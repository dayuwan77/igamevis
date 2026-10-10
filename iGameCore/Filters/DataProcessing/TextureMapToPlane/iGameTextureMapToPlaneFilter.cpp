#include "iGameTextureMapToPlaneFilter.h"

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

IGAME_NAMESPACE_BEGIN

namespace {

DataObject::Pointer ClonePointSet(const DataObject::Pointer& input) {
    auto copyPoints = [](PointSet* source) {
        auto points = Points::New();
        points->DeepCopy(source->GetPoints());
        return points;
    };
    auto copyAttributes = [](DataObject* source) {
        auto attributes = AttributeSet::New();
        attributes->DeepCopy(source->GetAttributeSet());
        return attributes;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto source = DynamicCast<SurfaceMesh>(input);
            auto output = SurfaceMesh::New();
            output->SetPoints(copyPoints(source));
            if (source->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(source->GetFaces());
                output->SetFaces(faces);
            }
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto source = DynamicCast<UnstructuredMesh>(input);
            auto output = UnstructuredMesh::New();
            output->SetPoints(copyPoints(source));
            auto cells = CellArray::New();
            cells->DeepCopy(source->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(source->GetCellTypes());
            output->SetCells(cells, types);
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_VOLUME_MESH: {
            auto source = DynamicCast<VolumeMesh>(input);
            auto output = VolumeMesh::New();
            output->SetPoints(copyPoints(source));
            auto volumes = CellArray::New();
            volumes->DeepCopy(source->GetVolumes());
            output->SetVolumes(volumes);
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_STRUCTURED_MESH: {
            auto source = DynamicCast<StructuredMesh>(input);
            auto output = StructuredMesh::New();
            output->SetPoints(copyPoints(source));
            output->SetDimensionSize(source->GetDimensionSize());
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        case IG_POINT_SET: {
            auto source = DynamicCast<PointSet>(input);
            auto output = PointSet::New();
            output->SetPoints(copyPoints(source));
            output->SetAttributeSet(copyAttributes(source));
            output->SetName(source->GetName());
            return output;
        }
        default:
            return nullptr;
    }
}

bool FirstSignificantComponentIsNegative(const Eigen::Vector3d& vector, double tolerance) {
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(vector[axis]) > tolerance) return vector[axis] < 0.0;
    }
    return false;
}

} // namespace

TextureMapToPlaneFilter::TextureMapToPlaneFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool TextureMapToPlaneFilter::Execute() {
    m_LastError.clear();
    m_TextureCoordinates = nullptr;

    const auto fail = [this](std::string message) {
        m_LastError = std::move(message);
        return false;
    };

    auto input = GetInput(0);
    auto inputPointSet = DynamicCast<PointSet>(input);
    if (inputPointSet.IsNull()) {
        return fail("Texture Map to Plane 仅支持 PointSet 及其派生网格类型；当前输入不包含可映射的点数据。");
    }
    const IGsize pointCount = inputPointSet->GetNumberOfPoints();
    if (pointCount < 2) return fail("输入至少需要两个点才能生成平面纹理坐标。");

    std::vector<std::array<double, 2>> coordinates(pointCount);
    constexpr double relativeTolerance = 1.0e-12;

    if (m_AutomaticPlaneGeneration) {
        if (pointCount < 3) return fail("自动平面拟合至少需要三个非共线点。");

        Eigen::Vector3d center = Eigen::Vector3d::Zero();
        double coordinateScale = 0.0;
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            center += Eigen::Vector3d(point[0], point[1], point[2]);
            coordinateScale = std::max({coordinateScale, std::abs(static_cast<double>(point[0])),
                                        std::abs(static_cast<double>(point[1])),
                                        std::abs(static_cast<double>(point[2]))});
        }
        center /= static_cast<double>(pointCount);

        Eigen::Matrix3d covariance = Eigen::Matrix3d::Zero();
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            const Eigen::Vector3d offset(point[0] - center[0], point[1] - center[1], point[2] - center[2]);
            covariance.noalias() += offset * offset.transpose();
        }
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(covariance);
        if (solver.info() != Eigen::Success) return fail("自动平面拟合失败：无法求解输入点集的主方向。");

        const auto eigenvalues = solver.eigenvalues();
        const double scaleSquared = std::max(1.0, coordinateScale * coordinateScale);
        if (eigenvalues[1] <= scaleSquared * relativeTolerance) {
            return fail("自动平面拟合失败：输入点全部重合或近似共线，无法确定二维纹理平面。");
        }

        Eigen::Vector3d normal = solver.eigenvectors().col(0).normalized();
        const double minimumNormalComponent = std::min({std::abs(normal[0]), std::abs(normal[1]),
                                                        std::abs(normal[2])});
        int referenceAxis = 0;
        while (referenceAxis < 2 &&
               std::abs(normal[referenceAxis]) > minimumNormalComponent + 1.0e-10) ++referenceAxis;
        Eigen::Vector3d reference = Eigen::Vector3d::Zero();
        reference[referenceAxis] = 1.0;
        Eigen::Vector3d sAxis = (reference - normal * normal.dot(reference)).normalized();
        Eigen::Vector3d tAxis = normal.cross(sAxis).normalized();
        const double axisTolerance = std::numeric_limits<double>::epsilon() * 64.0;
        if (FirstSignificantComponentIsNegative(tAxis, axisTolerance)) tAxis = -tAxis;

        double sMin = std::numeric_limits<double>::max();
        double sMax = std::numeric_limits<double>::lowest();
        double tMin = std::numeric_limits<double>::max();
        double tMax = std::numeric_limits<double>::lowest();
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            const Eigen::Vector3d offset(point[0] - center[0], point[1] - center[1], point[2] - center[2]);
            coordinates[pointId] = {offset.dot(sAxis), offset.dot(tAxis)};
            sMin = std::min(sMin, coordinates[pointId][0]);
            sMax = std::max(sMax, coordinates[pointId][0]);
            tMin = std::min(tMin, coordinates[pointId][1]);
            tMax = std::max(tMax, coordinates[pointId][1]);
        }
        const double sLength = sMax - sMin;
        const double tLength = tMax - tMin;
        const double extentTolerance = std::max(1.0, coordinateScale) * 1.0e-10;
        if (sLength <= extentTolerance || tLength <= extentTolerance) {
            return fail("自动平面拟合失败：输入点在拟合平面上的二维范围退化。");
        }
        for (auto& coordinate : coordinates) {
            coordinate[0] = (coordinate[0] - sMin) / sLength;
            coordinate[1] = (coordinate[1] - tMin) / tLength;
        }
    } else {
        const Eigen::Vector3d origin(m_Origin[0], m_Origin[1], m_Origin[2]);
        const Eigen::Vector3d sAxis(m_Point1[0] - m_Origin[0], m_Point1[1] - m_Origin[1],
                                    m_Point1[2] - m_Origin[2]);
        const Eigen::Vector3d tAxis(m_Point2[0] - m_Origin[0], m_Point2[1] - m_Origin[1],
                                    m_Point2[2] - m_Origin[2]);
        const double sLengthSquared = sAxis.squaredNorm();
        const double tLengthSquared = tAxis.squaredNorm();
        if (sLengthSquared <= relativeTolerance || tLengthSquared <= relativeTolerance) {
            return fail("手动映射失败：Origin 到 Point1、Point2 的两条平面轴长度都必须大于零。");
        }
        if (sAxis.cross(tAxis).squaredNorm() <= relativeTolerance * sLengthSquared * tLengthSquared) {
            return fail("手动映射失败：Origin、Point1、Point2 不能共线。");
        }
        for (IGsize pointId = 0; pointId < pointCount; ++pointId) {
            const auto& point = inputPointSet->GetPoint(pointId);
            const Eigen::Vector3d offset(point[0] - origin[0], point[1] - origin[1], point[2] - origin[2]);
            coordinates[pointId] = {offset.dot(sAxis) / sLengthSquared, offset.dot(tAxis) / tLengthSquared};
        }
    }

    auto output = ClonePointSet(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (outputPointSet.IsNull()) return fail("无法为当前网格类型创建独立输出。");
    output->SetName(input->GetName() + "_TextureMapToPlane");

    m_TextureCoordinates = FloatArray::New();
    m_TextureCoordinates->SetName(TextureCoordinatesArrayName());
    m_TextureCoordinates->SetDimension(2);
    m_TextureCoordinates->Reserve(pointCount);
    for (const auto& coordinate : coordinates) {
        const float tuple[2]{static_cast<float>(coordinate[0]), static_cast<float>(coordinate[1])};
        m_TextureCoordinates->AddElement(tuple);
    }

    auto* attributes = outputPointSet->GetAttributeSet();
    for (IGsize attributeId = 0; attributeId < attributes->GetNumberOfAttributes(); ++attributeId) {
        auto& attribute = attributes->GetAttribute(attributeId);
        if (attribute.IsDeleted() || attribute.GetAttachmentType() != IG_POINT) continue;
        if (attribute.GetType() == IG_TCOORD ||
            (attribute.GetPointer() && attribute.GetPointer()->GetName() == TextureCoordinatesArrayName())) {
            attributes->DeleteAttribute(attributeId);
        }
    }
    attributes->AddAttribute(IG_TCOORD, IG_POINT, m_TextureCoordinates);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
