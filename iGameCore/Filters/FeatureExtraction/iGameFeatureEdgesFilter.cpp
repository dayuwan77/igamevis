#include "iGameFeatureEdgesFilter.h"

#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <iostream>

IGAME_NAMESPACE_BEGIN

namespace {

    constexpr double PI =
        3.14159265358979323846;

    constexpr double EPSILON =
        1e-12;

    double VectorLength(
        const std::array<double, 3>& vector) {
        return std::sqrt(
            vector[0] * vector[0] +
            vector[1] * vector[1] +
            vector[2] * vector[2]);
    }

    /*
     * 计算两个单位法向量的点积。
     *
     * 不使用 abs(dot)，因为 ParaView/VTK
     * 使用原始法向量点积进行特征角判断。
     */
    double ComputeNormalDot(
        const std::array<double, 3>& normal1,
        const std::array<double, 3>& normal2) {
        double dot =
            normal1[0] * normal2[0] +
            normal1[1] * normal2[1] +
            normal1[2] * normal2[2];

        return std::clamp(
            dot,
            -1.0,
            1.0);
    }

    /*
     * 使用 Newell 方法计算 SurfaceMesh 面法向量。
     */
    std::array<double, 3>
        ComputeSurfaceFaceNormal(
            SurfaceMesh::Pointer surfaceMesh,
            IGsize faceId) {
        std::array<double, 3> normal{
            0.0,
            0.0,
            0.0
        };

        if (surfaceMesh == nullptr) {
            return normal;
        }

        igIndex pointIds[
            IGAME_CELL_MAX_SIZE
        ]{};

            const int numberOfPoints =
                surfaceMesh->GetFacePointIds(
                    faceId,
                    pointIds);

            if (numberOfPoints < 3) {
                return normal;
            }

            for (int i = 0;
                i < numberOfPoints;
                ++i) {
                const Point& point1 =
                    surfaceMesh->GetPoint(
                        pointIds[i]);

                const Point& point2 =
                    surfaceMesh->GetPoint(
                        pointIds[
                            (i + 1) %
                                numberOfPoints
                        ]);

                normal[0] +=
                    (point1[1] - point2[1]) *
                    (point1[2] + point2[2]);

                normal[1] +=
                    (point1[2] - point2[2]) *
                    (point1[0] + point2[0]);

                normal[2] +=
                    (point1[0] - point2[0]) *
                    (point1[1] + point2[1]);
            }

            const double length =
                VectorLength(normal);

            if (length < EPSILON) {
                return {
                    0.0,
                    0.0,
                    0.0
                };
            }

            normal[0] /= length;
            normal[1] /= length;
            normal[2] /= length;

            return normal;
    }

}  // namespace

std::array<double, 3>
FeatureEdgesFilter::ComputeFaceNormal(
    Face* face) {
    if (face == nullptr) {
        return {
            0.0,
            0.0,
            0.0
        };
    }

    const Vector3f normal =
        face->GetNormal();

    std::array<double, 3> result{
        static_cast<double>(normal[0]),
        static_cast<double>(normal[1]),
        static_cast<double>(normal[2])
    };

    const double length =
        VectorLength(result);

    if (length < EPSILON) {
        return {
            0.0,
            0.0,
            0.0
        };
    }

    result[0] /= length;
    result[1] /= length;
    result[2] /= length;

    return result;
}

bool FeatureEdgesFilter::Execute() {
    auto input =
        GetInput(0);

    if (input == nullptr) {
        std::cerr
            << "FeatureEdgesFilter: input is null."
            << std::endl;

        return false;
    }

    /*
     * 当前 FeatureEdgesFilter 要求输入为 SurfaceMesh。
     */
    auto surfaceMesh =
        DynamicCast<SurfaceMesh>(
            input);

    if (surfaceMesh == nullptr) {
        auto unstructuredInput =
            DynamicCast<UnstructuredMesh>(
                input);

        if (unstructuredInput != nullptr) {
            std::cerr
                << "FeatureEdgesFilter: input is an "
                << "UnstructuredMesh."
                << std::endl;

            std::cerr
                << "Please extract the surface mesh first, "
                << "then apply FeatureEdgesFilter."
                << std::endl;
        }
        else {
            std::cerr
                << "FeatureEdgesFilter: input must be a "
                << "SurfaceMesh."
                << std::endl;
        }

        return false;
    }

    if (surfaceMesh->GetNumberOfPoints() == 0 ||
        surfaceMesh->GetNumberOfFaces() == 0) {
        std::cerr
            << "FeatureEdgesFilter: input surface mesh "
            << "is empty."
            << std::endl;

        return false;
    }

    /*
     * 构建边以及面到边的邻接关系。
     */
    surfaceMesh->BuildEdges();
    surfaceMesh->BuildFaceEdgeLinks();

    const IGsize numberOfEdges =
        surfaceMesh->GetNumberOfEdges();

    if (numberOfEdges == 0) {
        std::cerr
            << "FeatureEdgesFilter: surface mesh has "
            << "no edges."
            << std::endl;

        return false;
    }

    /*
     * 特征角度余弦值。
     *
     * ParaView/VTK 的判断方式：
     *
     *     normalDot <= cos(featureAngle)
     *
     * 因此等于特征角度的边也会被判定为特征边。
     */
    const double featureCosine =
        std::cos(
            m_FeatureAngle *
            PI /
            180.0);

    auto output =
        UnstructuredMesh::New();

    auto outputCells =
        CellArray::New();

    auto outputTypes =
        UnsignedIntArray::New();

    /*
     * Cell Data:
     *
     * 0 - Boundary Edge
     * 1 - Non-Manifold Edge
     * 2 - Feature Edge
     * 3 - Manifold Edge
     */
    auto edgeTypes =
        FloatArray::New();

    edgeTypes->SetName(
        "Edge Types");

    edgeTypes->SetDimension(
        1);

    edgeTypes->Reserve(
        numberOfEdges);

    /*
     * Cell Data:
     *
     * 每个输出线单元对应的原始 SurfaceMesh Edge Id。
     */
    auto edgeIds =
        UnsignedIntArray::New();

    edgeIds->SetName(
        "Edge Ids");

    edgeIds->SetDimension(
        1);

    edgeIds->Reserve(
        numberOfEdges);

    /*
     * 当前实现保留原始 SurfaceMesh 的点。
     */
    output->SetPoints(
        surfaceMesh->GetPoints());

    IGsize boundaryEdgeCount = 0;
    IGsize featureEdgeCount = 0;
    IGsize nonManifoldEdgeCount = 0;
    IGsize manifoldEdgeCount = 0;
    IGsize outputEdgeCount = 0;

    for (IGsize edgeId = 0;
        edgeId < numberOfEdges;
        ++edgeId) {
        SurfaceMesh::ReturnContainer neighborFaces;

        const bool success =
            surfaceMesh->GetEdgeToNeighborFaces(
                edgeId,
                neighborFaces);

        if (!success ||
            neighborFaces.size() == 0) {
            continue;
        }

        const IGsize neighborFaceCount =
            neighborFaces.size();

        bool shouldOutput =
            false;

        int edgeTypeValue =
            -1;

        /*
         * 一张面共享的边：
         * Boundary Edge
         */
        if (neighborFaceCount == 1) {
            if (m_BoundaryEdges) {
                shouldOutput = true;
                edgeTypeValue = BOUNDARY_EDGE;
                ++boundaryEdgeCount;
            }
        }

        /*
         * 三张及以上面共享的边：
         * Non-Manifold Edge
         */
        else if (neighborFaceCount >= 3) {
            if (m_NonManifoldEdges) {
                shouldOutput = true;
                edgeTypeValue = NON_MANIFOLD_EDGE;
                ++nonManifoldEdgeCount;
            }
        }

        /*
         * 两张面共享的边：
         * 可能是 Feature Edge 或 Manifold Edge。
         */
        else if (neighborFaceCount == 2) {
            const auto normal1 =
                ComputeSurfaceFaceNormal(
                    surfaceMesh,
                    neighborFaces[0]);

            const auto normal2 =
                ComputeSurfaceFaceNormal(
                    surfaceMesh,
                    neighborFaces[1]);

            const double length1 =
                VectorLength(normal1);

            const double length2 =
                VectorLength(normal2);

            if (length1 > EPSILON &&
                length2 > EPSILON) {
                const double normalDot =
                    ComputeNormalDot(
                        normal1,
                        normal2);

                /*
                 * 与 ParaView/VTK 保持一致：
                 *
                 * normalDot <= cos(featureAngle)
                 */
                if (m_FeatureEdges &&
                    normalDot <= featureCosine) {
                    shouldOutput = true;
                    edgeTypeValue = FEATURE_EDGE;
                    ++featureEdgeCount;
                }

                /*
                 * 两个相邻面夹角小于特征角度，
                 * 属于普通流形边。
                 */
                else if (m_ManifoldEdges) {
                    shouldOutput = true;
                    edgeTypeValue = MANIFOLD_EDGE;
                    ++manifoldEdgeCount;
                }
            }
        }

        if (!shouldOutput ||
            edgeTypeValue < 0) {
            continue;
        }

        igIndex pointIds[2] = {
            0,
            0
        };

        const int pointCount =
            surfaceMesh->GetEdgePointIds(
                edgeId,
                pointIds);

        if (pointCount != 2) {
            continue;
        }

        /*
         * 输出一个线单元。
         */
        outputCells->AddCellId2(
            pointIds[0],
            pointIds[1]);

        outputTypes->AddValue(
            IG_LINE);

        /*
         * 写入 Edge Types Cell Data。
         */
        edgeTypes->AddValue(
            static_cast<float>(
                edgeTypeValue));

        /*
         * 写入原始 Edge Id Cell Data。
         */
        edgeIds->AddValue(
            static_cast<unsigned int>(
                edgeId));

        ++outputEdgeCount;
    }

    if (outputEdgeCount == 0) {
        std::cerr
            << "FeatureEdgesFilter: no edges matched "
            << "the selected criteria."
            << std::endl;

        return false;
    }

    output->SetCells(
        outputCells,
        outputTypes);

    auto attributeSet =
        output->GetAttributeSet();

    /*
     * 添加 Cell Data: Edge Types。
     */
    attributeSet->AddScalar(
        IG_CELL,
        edgeTypes);

    /*
     * 添加 Cell Data: Edge Ids。
     */
    attributeSet->AddScalar(
        IG_CELL,
        edgeIds);

    /*
     * 通知绘制模块重新转换属性数据，
     * 以便后续调用 ViewCloudPicture。
     */
    attributeSet->
        ForceReConvertToDrawableData();

    SetOutput(
        output);

    std::cout
        << "Boundary edges: "
        << boundaryEdgeCount
        << std::endl;

    std::cout
        << "Feature edges: "
        << featureEdgeCount
        << std::endl;

    std::cout
        << "Non-manifold edges: "
        << nonManifoldEdgeCount
        << std::endl;

    std::cout
        << "Manifold edges: "
        << manifoldEdgeCount
        << std::endl;

    std::cout
        << "Output edges: "
        << outputEdgeCount
        << std::endl;

    return true;
}

IGAME_NAMESPACE_END