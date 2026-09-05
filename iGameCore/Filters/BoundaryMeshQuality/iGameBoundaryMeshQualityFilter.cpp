#include "iGameBoundaryMeshQualityFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"

#include <cfloat>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

BoundaryMeshQualityFilter::BoundaryMeshQualityFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

BoundaryMeshQualityFilter::~BoundaryMeshQualityFilter() = default;

bool BoundaryMeshQualityFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) {
        m_Message = "No input";
        return false;
    }

    auto input = m_Inputs->GetElement(0);
    if (!input) {
        m_Message = "Empty input";
        return false;
    }

    // 将输入统一转换为体网格
    m_VolumeMesh = nullptr;
    switch (input->GetDataObjectType()) {
        case IG_VOLUME_MESH:
            m_VolumeMesh = DynamicCast<VolumeMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            auto converter = ConvertToVolumeMeshFilter::New();
            converter->SetInput(mesh);
            if (converter->Execute()) {
                m_VolumeMesh = converter->GetVolumeMesh();
            }
            break;
        }
        default:
            break;
    }

    if (!m_VolumeMesh) {
        m_Message = "Boundary mesh quality requires a volume mesh as input. "
                    "Please load a volume mesh or extract the surface mesh first.";
        return false;
    }

    // 确保面表、面-体邻接等拓扑已构建（对从 UnstructuredMesh 转换得到的网格尤其必要）
    m_VolumeMesh->RequestEditStatus();

    igIndex faceNum = m_VolumeMesh->GetNumberOfFaces();
    if (faceNum <= 0) {
        m_Message = "Volume mesh has no faces.";
        return false;
    }

    // 收集所有边界面
    std::vector<igIndex> boundaryFaceIds;
    boundaryFaceIds.reserve(faceNum);
    for (igIndex fid = 0; fid < faceNum; ++fid) {
        if (m_VolumeMesh->IsBoundaryFace(fid)) {
            boundaryFaceIds.push_back(fid);
        }
    }

    if (boundaryFaceIds.empty()) {
        m_Message = "No boundary faces found in the volume mesh.";
        return false;
    }

    DoubleArray::Pointer metricArray = DoubleArray::New();
    switch (m_Metric) {
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER:
            metricArray->SetName("DistanceFromCellCenterToFaceCenter");
            break;
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE:
            metricArray->SetName("DistanceFromCellCenterToFacePlane");
            break;
        case ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR:
            metricArray->SetName("AngleFaceNormalAndCellCenterToFaceCenterVector");
            break;
        default:
            metricArray->SetName("BoundaryMetric");
            break;
    }

    metricArray->SetDimension(1);
    // metric 数 = 输入 mesh 的 cell 数（边界面对应的 cell 写入计算值，其余置 0）
    IGsize inputCellNum = 0;
    if (input->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        inputCellNum = DynamicCast<UnstructuredMesh>(input)->GetNumberOfCells();
    } else if (input->GetDataObjectType() == IG_VOLUME_MESH) {
        inputCellNum = DynamicCast<VolumeMesh>(input)->GetNumberOfVolumes();
    } else if (input->GetDataObjectType() == IG_SURFACE_MESH) {
        inputCellNum = DynamicCast<SurfaceMesh>(input)->GetNumberOfFaces();
    }
    metricArray->Reserve(static_cast<int>(inputCellNum));
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    for (igIndex i = 0; i < inputCellNum; ++i) {
        metricArray->AddValue(NaN);
    }

    int progressCount = 0;
    int totalBoundaryFaces = static_cast<int>(boundaryFaceIds.size());
    int reportBlock = std::max(1, totalBoundaryFaces / 50);

    // 收集 boundary face 的 metric，并写回与之相邻的原始 cell
    for (size_t i = 0; i < boundaryFaceIds.size(); ++i) {
        if (static_cast<int>(i) > reportBlock * progressCount) {
            progressCount++;
            UpdateProgress(progressCount * 0.02);
        }
        double metric = ComputeMetricForBoundaryFace(boundaryFaceIds[i]);

        igIndex volIds[64];
        int volSize = m_VolumeMesh->GetFaceToNeighborVolumes(boundaryFaceIds[i], volIds);
        if (volSize <= 0) continue;
        igIndex ownerCell = volIds[0];
        if (inputCellNum > 0 && ownerCell >= 0 &&
            ownerCell < static_cast<igIndex>(inputCellNum)) {
            metricArray->SetValue(static_cast<int>(ownerCell), metric);
        }
    }

    // 当输出为角度指标时，需要将弧度转换为度数，并归一化到 [0, 90]
    if (m_Metric == ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR) {
        DoubleArray::Pointer degArray = DoubleArray::New();
        degArray->SetName(metricArray->GetName());
        degArray->SetDimension(1);
        degArray->Reserve(static_cast<int>(inputCellNum));
        int n = static_cast<int>(metricArray->GetNumberOfValues());
        for (int i = 0; i < n; ++i) {
            double v = metricArray->GetValue(i);
            degArray->AddValue(NormalizeAngle(v));
        }
        metricArray = degArray;
    }

    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr) {
        m_Message = "Input has no AttributeSet.";
        return false;
    }
    attrs->AddAttribute(IG_SCALAR, IG_CELL, metricArray);

    this->SetOutput(input);
    return true;
}

Point BoundaryMeshQualityFilter::ComputeCellCenter(Volume* cell) {
    Point center(0, 0, 0);
    int n = cell->GetNumberOfPoints();
    if (n <= 0) return center;
    for (int i = 0; i < n; ++i) {
        Point p = cell->GetPoint(i);
        center[0] += p[0];
        center[1] += p[1];
        center[2] += p[2];
    }
    center[0] /= n;
    center[1] /= n;
    center[2] /= n;
    return center;
}

double BoundaryMeshQualityFilter::AngleInDegrees(const Vector3f& a, const Vector3f& b) {
    double la = a.length();
    double lb = b.length();
    if (la < 1e-12 || lb < 1e-12) return 0.0;
    double cosTheta = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (la * lb);
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta));
    return std::acos(cosTheta) * 180.0 / PI;
}

double BoundaryMeshQualityFilter::NormalizeAngle(double angleDeg) {
    // 保留在一个有意义的范围内[0, 90]
    double a = std::fabs(angleDeg);
    if (a > 90.0) a = 180.0 - a;
    if (a < 0.0) a = 0.0;
    if (a > 90.0) a = 90.0;
    return a;
}

double BoundaryMeshQualityFilter::ComputeMetricForBoundaryFace(igIndex faceId) {
    if (!m_VolumeMesh) return 0.0;

    // 获取与该边界面相邻的体单元
    igIndex volIds[64];
    int volSize = m_VolumeMesh->GetFaceToNeighborVolumes(faceId, volIds);
    if (volSize <= 0) return 0.0;
    igIndex volId = volIds[0];

    // 获取体单元、面及对应的几何信息
    Volume* vol = m_VolumeMesh->GetVolume(volId);
    if (vol == nullptr) return 0.0;

    igIndex facePids[IGAME_CELL_MAX_SIZE];
    int facePcnt = m_VolumeMesh->GetFacePointIds(faceId, facePids);
    if (facePcnt <= 0) return 0.0;

    Point cellCenter = ComputeCellCenter(vol);

    // 手动计算面中心
    Point faceCenter(0, 0, 0);
    for (int i = 0; i < facePcnt; ++i) {
        Point p = m_VolumeMesh->GetPoint(facePids[i]);
        faceCenter[0] += p[0];
        faceCenter[1] += p[1];
        faceCenter[2] += p[2];
    }
    faceCenter[0] /= facePcnt;
    faceCenter[1] /= facePcnt;
    faceCenter[2] /= facePcnt;

    // 手动计算面法线（按相同顺序，避免复用 Vol*::GetFace 中转）
    Point fp0 = m_VolumeMesh->GetPoint(facePids[0]);
    Point fp1 = m_VolumeMesh->GetPoint(facePids[1]);
    Point fp2 = m_VolumeMesh->GetPoint(facePids[2]);
    Vector3f v01 = fp1 - fp0;
    Vector3f v02 = fp2 - fp0;
    Vector3f rawNormal = v01.cross(v02);
    double nlen = rawNormal.length();
    Vector3f faceNormal(0, 0, 0);
    if (nlen > 1e-12) {
        faceNormal[0] = rawNormal[0] / nlen;
        faceNormal[1] = rawNormal[1] / nlen;
        faceNormal[2] = rawNormal[2] / nlen;
    }

    // 体单元中心 -> 面中心向量
    Vector3f centerVec(
        faceCenter[0] - cellCenter[0],
        faceCenter[1] - cellCenter[1],
        faceCenter[2] - cellCenter[2]);

    switch (m_Metric) {
        case DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER:
            return centerVec.length();

        case DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE: {
            // 平面方程: n . (x - fp0) = 0; 距离 = |n . (cellCenter - fp0)|
            // 与 DistanceFromCellCenterToFaceCenter 类似，也取绝对值使颜色映射对称
            if (nlen < 1e-12) return 0.0;
            double d = faceNormal[0] * (cellCenter[0] - fp0[0]) +
                       faceNormal[1] * (cellCenter[1] - fp0[1]) +
                       faceNormal[2] * (cellCenter[2] - fp0[2]);
            return std::fabs(d);
        }

        case ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR: {
            // 与 ParaView 一致：计算归一化的 (faceCenter - cellCenter) 与 faceNormal 的夹角
            double lv = centerVec.length();
            if (lv < 1e-12) return 0.0;
            Vector3f normVec(
                centerVec[0] / lv,
                centerVec[1] / lv,
                centerVec[2] / lv);
            return AngleInDegrees(faceNormal, normVec);
        }

        default:
            return 0.0;
    }
}

IGAME_NAMESPACE_END
