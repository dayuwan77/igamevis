#include "iGameValidateCellsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kAbsoluteTolerance = 1e-9;
constexpr double kRelativeTolerance = 1e-7;

inline Vector3d ToDouble(const Point& p) {
    return Vector3d(static_cast<double>(p[0]), static_cast<double>(p[1]), static_cast<double>(p[2]));
}

double ComputeTolerance(const std::vector<Point>& pts) {
    if (pts.empty()) {
        return kAbsoluteTolerance;
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double minZ = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    double maxZ = std::numeric_limits<double>::lowest();

    for (const auto& p : pts) {
        minX = std::min(minX, static_cast<double>(p[0]));
        minY = std::min(minY, static_cast<double>(p[1]));
        minZ = std::min(minZ, static_cast<double>(p[2]));
        maxX = std::max(maxX, static_cast<double>(p[0]));
        maxY = std::max(maxY, static_cast<double>(p[1]));
        maxZ = std::max(maxZ, static_cast<double>(p[2]));
    }

    const double diag = std::sqrt((maxX - minX) * (maxX - minX) +
                                  (maxY - minY) * (maxY - minY) +
                                  (maxZ - minZ) * (maxZ - minZ));
    return std::max(kAbsoluteTolerance, diag * kRelativeTolerance);
}

inline bool PointsAreCoincident(const Point& a, const Point& b, double tol) {
    return (ToDouble(a) - ToDouble(b)).norm() <= tol;
}

inline bool HasDuplicatePoints(const std::vector<Point>& pts, double tol) {
    for (size_t i = 0; i < pts.size(); ++i) {
        for (size_t j = i + 1; j < pts.size(); ++j) {
            if ((ToDouble(pts[i]) - ToDouble(pts[j])).norm() <= tol) {
                return true;
            }
        }
    }
    return false;
}

double PointSegmentDistanceSq(const Vector3d& p, const Vector3d& a, const Vector3d& b,
                              double abLenSq) {
    if (abLenSq <= 0.0) {
        return (p - a).squaredNorm();
    }

    const double t = std::clamp((p - a).dot(b - a) / abLenSq, 0.0, 1.0);
    const Vector3d closest = a + (b - a) * t;
    return (p - closest).squaredNorm();
}

double PointLineDistanceSq(const Vector3d& p, const Vector3d& linePoint,
                           const Vector3d& lineDir, double lineLenSq) {
    if (lineLenSq <= 0.0) {
        return (p - linePoint).squaredNorm();
    }
    const Vector3d cross = (p - linePoint).cross(lineDir);
    return cross.squaredNorm() / lineLenSq;
}

// 3D 线段相交判断：
// 1) 非平行线段先求最近点参数，再检查三维空间距离；
// 2) 共线/近共线时检查是否发生重叠，而不是只看线性方程是否退化。
bool SegmentsIntersect(const Point& p1, const Point& p2, const Point& q1, const Point& q2,
                       double tol) {
    const Vector3d a = ToDouble(p1);
    const Vector3d b = ToDouble(p2);
    const Vector3d c = ToDouble(q1);
    const Vector3d d = ToDouble(q2);

    const Vector3d u = b - a;
    const Vector3d v = d - c;
    const Vector3d w = a - c;

    const double uu = u.squaredNorm();
    const double vv = v.squaredNorm();
    const double uv = u.dot(v);
    const double uw = u.dot(w);
    const double vw = v.dot(w);
    const double tol2 = tol * tol;

    double scale = std::max(std::max(uu, vv), tol2);
    scale = std::max(scale, 1e-30);
    const double denom = uu * vv - uv * uv;

    if (denom <= 1e-14 * scale) {
        if (uu <= tol2 && vv <= tol2) {
            return (a - c).squaredNorm() <= tol2;
        }
        if (uu <= tol2) {
            return PointSegmentDistanceSq(a, c, d, vv) <= tol2;
        }
        if (vv <= tol2) {
            return PointSegmentDistanceSq(c, a, b, uu) <= tol2;
        }

        // 先确认两条线段是否落在同一条三维直线上。
        if (PointLineDistanceSq(c, a, u, uu) > tol2 ||
            PointLineDistanceSq(d, a, u, uu) > tol2) {
            return false;
        }

        // 共线时按沿 u 方向的投影区间计算重叠长度。
        const double invUu = 1.0 / uu;
        const double s0 = u.dot(c - a) * invUu;
        const double s1 = u.dot(d - a) * invUu;
        const double overlapStart = std::max(0.0, std::min(s0, s1));
        const double overlapEnd = std::min(1.0, std::max(s0, s1));
        return (overlapEnd - overlapStart) * std::sqrt(uu) > tol;
    }

    const double s = (uv * vw - vv * uw) / denom;
    const double t = (uu * vw - uv * uw) / denom;

    constexpr double kParamTolerance = 1e-9;
    if (s < -kParamTolerance || s > 1.0 + kParamTolerance ||
        t < -kParamTolerance || t > 1.0 + kParamTolerance) {
        return false;
    }

    const Vector3d closestOnFirst = a + u * std::clamp(s, 0.0, 1.0);
    const Vector3d closestOnSecond = c + v * std::clamp(t, 0.0, 1.0);
    return (closestOnFirst - closestOnSecond).squaredNorm() <= tol2;
}

bool HasEdgeIntersections(const std::vector<Point>& pts, double tol, bool closed) {
    const int n = static_cast<int>(pts.size());
    if (n < 2) {
        return false;
    }

    const int edgeCount = closed ? n : n - 1;
    for (int i = 0; i < edgeCount; ++i) {
        const int i1 = (i + 1) % n;
        for (int j = i + 1; j < edgeCount; ++j) {
            if (closed) {
                if (j == i + 1 || (i == 0 && j == edgeCount - 1)) {
                    continue;
                }
            } else if (j == i + 1) {
                continue;
            }

            const int j1 = (j + 1) % n;
            if (SegmentsIntersect(pts[i], pts[i1], pts[j], pts[j1], tol)) {
                return true;
            }
        }
    }
    return false;
}

bool TriangleIsDegenerate(const std::vector<Point>& pts, double tol) {
    const Vector3d a = ToDouble(pts[0]);
    const Vector3d b = ToDouble(pts[1]);
    const Vector3d c = ToDouble(pts[2]);
    const double ab = (b - a).norm();
    const double ac = (c - a).norm();
    const double bc = (c - b).norm();
    const double maxEdge = std::max(std::max(ab, ac), bc);
    if (maxEdge <= tol) {
        return true;
    }

    const double crossNorm = (b - a).cross(c - a).norm();
    return crossNorm <= tol * maxEdge;
}

bool PolygonIsNonconvex(const std::vector<Point>& pts, double tol) {
    const int n = static_cast<int>(pts.size());
    if (n < 4) {
        return false;
    }

    // Newell 法向量，避免把近平面多边形直接按任意轴向投影造成误判。
    Vector3d normal(0.0, 0.0, 0.0);
    for (int i = 0; i < n; ++i) {
        const Vector3d current = ToDouble(pts[i]);
        const Vector3d next = ToDouble(pts[(i + 1) % n]);
        normal[0] += (current[1] - next[1]) * (current[2] + next[2]);
        normal[1] += (current[2] - next[2]) * (current[0] + next[0]);
        normal[2] += (current[0] - next[0]) * (current[1] + next[1]);
    }
    if (normal.squaredNorm() <= tol * tol) {
        return false;
    }

    int dominantAxis = 0;
    if (std::abs(normal[1]) > std::abs(normal[dominantAxis])) {
        dominantAxis = 1;
    }
    if (std::abs(normal[2]) > std::abs(normal[dominantAxis])) {
        dominantAxis = 2;
    }
    const int uAxis = (dominantAxis + 1) % 3;
    const int vAxis = (dominantAxis + 2) % 3;

    auto cross2D = [&](const Point& a, const Point& b, const Point& c) {
        return (static_cast<double>(b[uAxis]) - static_cast<double>(a[uAxis])) *
                       (static_cast<double>(c[vAxis]) - static_cast<double>(b[vAxis])) -
               (static_cast<double>(b[vAxis]) - static_cast<double>(a[vAxis])) *
                       (static_cast<double>(c[uAxis]) - static_cast<double>(b[uAxis]));
    };

    int orientation = 0;
    for (int i = 0; i < n; ++i) {
        const double turn = cross2D(pts[i], pts[(i + 1) % n], pts[(i + 2) % n]);
        if (std::abs(turn) <= tol * tol) {
            continue;
        }
        const int sign = turn > 0.0 ? 1 : -1;
        if (orientation == 0) {
            orientation = sign;
        } else if (orientation != sign) {
            return true;
        }
    }
    return false;
}

bool TetraIsDegenerate(const std::vector<Point>& pts, double tol) {
    const Vector3d a = ToDouble(pts[0]);
    const Vector3d b = ToDouble(pts[1]);
    const Vector3d c = ToDouble(pts[2]);
    const Vector3d d = ToDouble(pts[3]);

    double maxEdge = 0.0;
    const std::pair<int, int> edges[] = {{0, 1}, {0, 2}, {0, 3},
                                         {1, 2}, {1, 3}, {2, 3}};
    for (const auto& edge : edges) {
        maxEdge = std::max(maxEdge, (ToDouble(pts[edge.first]) - ToDouble(pts[edge.second])).norm());
    }
    if (maxEdge <= tol) {
        return true;
    }

    const double volume = std::abs((b - a).dot((c - a).cross(d - a))) / 6.0;
    return volume <= tol * tol * maxEdge;
}

unsigned short ValidateLine(const std::vector<Point>& pts, double tol) {
    if (pts.size() != 2) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (PointsAreCoincident(pts[0], pts[1], tol)) {
        state |= Validity_Nonconvex;
    }
    return state;
}

unsigned short ValidatePolyline(const std::vector<Point>& pts, double tol) {
    if (pts.size() < 2) {
        return Validity_WrongNumberOfPoints;
    }
    if (pts.size() == 2) {
        return ValidateLine(pts, tol);
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, tol)) {
        state |= Validity_Nonconvex;
    }
    for (size_t i = 1; i < pts.size(); ++i) {
        if (PointsAreCoincident(pts[i - 1], pts[i], tol)) {
            state |= Validity_NoncontiguousEdges;
        }
    }
    if (HasEdgeIntersections(pts, tol, false)) {
        state |= Validity_IntersectingEdges;
    }
    return state;
}

unsigned short ValidateTriangle(const std::vector<Point>& pts, double tol) {
    if (pts.size() != 3) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, tol)) {
        state |= Validity_Nonconvex;
    }
    if (TriangleIsDegenerate(pts, tol)) {
        state |= Validity_Nonconvex;
    }
    return state;
}

unsigned short ValidatePolygon(const std::vector<Point>& pts, double tol) {
    if (pts.size() < 3) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, tol)) {
        state |= Validity_Nonconvex;
    }

    const int n = static_cast<int>(pts.size());
    for (int i = 0; i < n; ++i) {
        if (PointsAreCoincident(pts[i], pts[(i + 1) % n], tol)) {
            state |= Validity_NoncontiguousEdges;
        }
    }
    if (HasEdgeIntersections(pts, tol, true)) {
        state |= Validity_IntersectingEdges;
    }
    if (PolygonIsNonconvex(pts, tol)) {
        state |= Validity_Nonconvex;
    }
    return state;
}

unsigned short ValidateTetra(const std::vector<Point>& pts, double tol) {
    if (pts.size() != 4) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, tol)) {
        state |= Validity_Nonconvex;
    }
    if (TetraIsDegenerate(pts, tol)) {
        state |= Validity_Nonconvex;
    }

    const Vector3d a = ToDouble(pts[0]);
    const Vector3d b = ToDouble(pts[1]);
    const Vector3d c = ToDouble(pts[2]);
    const Vector3d d = ToDouble(pts[3]);
    const double signedVolume = (b - a).dot((c - a).cross(d - a)) / 6.0;
    if (signedVolume < 0.0) {
        state |= Validity_FacesAreOrientedIncorrectly;
    }
    return state;
}

unsigned short ValidateCellState(IGenum cellType, const std::vector<Point>& pts, double tol) {
    if (!ValidateCellsFilter::IsSupportedCellType(cellType)) {
        return Validity_UnsupportedCellType;
    }

    switch (cellType) {
        case IG_VERTEX:
            return pts.size() == 1 ? Validity_Valid : Validity_WrongNumberOfPoints;

        case IG_LINE:
            return ValidateLine(pts, tol);

        case IG_POLY_LINE:
            return ValidatePolyline(pts, tol);

        case IG_TRIANGLE:
            return ValidateTriangle(pts, tol);

        case IG_QUAD:
        case IG_POLYGON:
        case IG_FACE:
            return ValidatePolygon(pts, tol);

        case IG_TETRA:
            return ValidateTetra(pts, tol);

        default:
            return Validity_UnsupportedCellType;
    }
}

std::vector<Point> GetCellPoints(const UnstructuredMesh::Pointer& mesh, IGsize cellId,
                                 const IdArray::Pointer& ids) {
    ids->Reset();
    mesh->GetCellPointIds(cellId, ids);

    std::vector<Point> pts;
    pts.reserve(static_cast<size_t>(ids->GetNumberOfIds()));
    for (int i = 0; i < ids->GetNumberOfIds(); ++i) {
        pts.push_back(mesh->GetPoint(ids->GetId(i)));
    }
    return pts;
}

std::vector<Point> GetFacePoints(const SurfaceMesh::Pointer& mesh, IGsize faceId) {
    const int nPts = mesh->GetNumberOfPoints();
    if (nPts <= 0) {
        return {};
    }

    std::vector<igIndex> ids(static_cast<size_t>(nPts));
    const int nFacePts = mesh->GetFacePointIds(faceId, ids.data());
    if (nFacePts <= 0) {
        return {};
    }

    ids.resize(static_cast<size_t>(nFacePts));
    std::vector<Point> pts;
    pts.reserve(ids.size());
    for (igIndex id : ids) {
        pts.push_back(mesh->GetPoint(id));
    }
    return pts;
}

int FlagIndex(unsigned short flag) {
    switch (flag) {
        case Validity_WrongNumberOfPoints: return 0;
        case Validity_IntersectingEdges: return 1;
        case Validity_IntersectingFaces: return 2;
        case Validity_NoncontiguousEdges: return 3;
        case Validity_Nonconvex: return 4;
        case Validity_FacesAreOrientedIncorrectly: return 5;
        case Validity_UnsupportedCellType: return 6;
        default: return -1;
    }
}

DataObject::Pointer CloneMeshForValidation(DataObject::Pointer input) {
    if (input == nullptr) {
        return nullptr;
    }

    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto points = Points::New();
        points->DeepCopy(src->GetPoints());
        return points;
    };
    auto copyAttributes = [](DataObject* src) -> AttributeSet::Pointer {
        auto attributes = AttributeSet::New();
        attributes->DeepCopy(src->GetAttributeSet());
        return attributes;
    };

    if (auto src = DynamicCast<UnstructuredMesh>(input)) {
        auto dst = UnstructuredMesh::New();
        dst->SetPoints(copyPoints(src.GetPointer()));

        auto cells = CellArray::New();
        cells->DeepCopy(src->GetCells());
        auto types = UnsignedIntArray::New();
        types->DeepCopy(src->GetCellTypes());
        dst->SetCells(cells, types);

        dst->SetAttributeSet(copyAttributes(src.GetPointer()));
        dst->SetName(src->GetName() + "_ValidateCells");
        return dst;
    }

    if (auto src = DynamicCast<SurfaceMesh>(input)) {
        auto dst = SurfaceMesh::New();
        dst->SetPoints(copyPoints(src.GetPointer()));
        if (src->GetFaces() != nullptr) {
            auto faces = CellArray::New();
            faces->DeepCopy(src->GetFaces());
            dst->SetFaces(faces);
        }
        dst->SetAttributeSet(copyAttributes(src.GetPointer()));
        dst->SetName(src->GetName() + "_ValidateCells");
        return dst;
    }

    return nullptr;
}

}  // namespace

bool ValidateCellsFilter::IsSupportedCellType(IGenum cellType) {
    switch (cellType) {
        case IG_VERTEX:
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_FACE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_TETRA:
            return true;
        default:
            return false;
    }
}

std::string ValidateCellsFilter::GetSupportedCellTypesDescription() {
    return "当前支持：UnstructuredMesh 中的点、线、折线、三角形、四边形、多边形、四面体；"
           "SurfaceMesh 中的三角形、四边形和多边形面。"
           "暂不支持高阶单元、六面体、三棱柱、金字塔、多面体及其他复合类型。";
}

std::string ValidateCellsFilter::GetValidityFlagName(unsigned short flag) {
    switch (flag) {
        case Validity_WrongNumberOfPoints: return "点数量错误";
        case Validity_IntersectingEdges: return "边相交";
        case Validity_IntersectingFaces: return "面相交";
        case Validity_NoncontiguousEdges: return "边不连续";
        case Validity_Nonconvex: return "非凸";
        case Validity_FacesAreOrientedIncorrectly: return "面朝向错误";
        case Validity_UnsupportedCellType: return "未支持的单元类型";
        default: return "未知错误";
    }
}

std::string ValidateCellsFilter::GetValidityStateText(unsigned short state) {
    if (state == Validity_Valid) {
        return "有效";
    }

    const unsigned short flags[] = {
            Validity_WrongNumberOfPoints,
            Validity_IntersectingEdges,
            Validity_IntersectingFaces,
            Validity_NoncontiguousEdges,
            Validity_Nonconvex,
            Validity_FacesAreOrientedIncorrectly,
            Validity_UnsupportedCellType};

    std::string text;
    for (unsigned short flag : flags) {
        if ((state & flag) != 0) {
            if (!text.empty()) {
                text += "、";
            }
            text += GetValidityFlagName(flag);
        }
    }
    return text.empty() ? "无效" : text;
}

const std::vector<igIndex>& ValidateCellsFilter::GetCellIdsWithFlag(unsigned short flag) const {
    static const std::vector<igIndex> empty;
    const int index = FlagIndex(flag);
    if (index < 0) {
        return empty;
    }
    return m_CellIdsByFlag[static_cast<size_t>(index)];
}

ValidateCellsFilter::ValidateCellsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ValidateCellsFilter::ResetResults(IGsize count) {
    m_ValidityStates.assign(static_cast<size_t>(count), Validity_Valid);
    m_InvalidCellIds.clear();
    m_UnsupportedCellIds.clear();
    for (auto& ids : m_CellIdsByFlag) {
        ids.clear();
    }
    m_LastError.clear();
}

void ValidateCellsFilter::RecordCell(IGsize cellId, IGenum cellType, unsigned short state) {
    (void)cellType;
    m_ValidityStates[static_cast<size_t>(cellId)] = state;
    if (state == Validity_Valid) {
        return;
    }

    if ((state & Validity_UnsupportedCellType) != 0) {
        m_UnsupportedCellIds.push_back(static_cast<igIndex>(cellId));
    } else {
        m_InvalidCellIds.push_back(static_cast<igIndex>(cellId));
    }

    const unsigned short flags[] = {
            Validity_WrongNumberOfPoints,
            Validity_IntersectingEdges,
            Validity_IntersectingFaces,
            Validity_NoncontiguousEdges,
            Validity_Nonconvex,
            Validity_FacesAreOrientedIncorrectly,
            Validity_UnsupportedCellType};
    for (unsigned short flag : flags) {
        if ((state & flag) == 0) {
            continue;
        }
        const int index = FlagIndex(flag);
        if (index >= 0) {
            m_CellIdsByFlag[static_cast<size_t>(index)].push_back(static_cast<igIndex>(cellId));
        }
    }
}

void ValidateCellsFilter::ApplyResultAttribute(DataObject* output) {
    if (output == nullptr) {
        return;
    }

    IntArray::Pointer stateArray = IntArray::New();
    stateArray->SetName(ValidityStateArrayName());
    stateArray->SetDimension(1);
    stateArray->Resize(static_cast<igIndex>(m_ValidityStates.size()));
    for (size_t i = 0; i < m_ValidityStates.size(); ++i) {
        stateArray->SetValue(static_cast<igIndex>(i), static_cast<int>(m_ValidityStates[i]));
    }

    auto* attributes = output->GetAttributeSet();
    if (attributes != nullptr) {
        while (true) {
            const int index = attributes->GetAttributeIndex(ValidityStateArrayName());
            if (index < 0) {
                break;
            }
            attributes->DeleteAttribute(index);
        }
        stateArray->Modified();
        attributes->AddAttribute(IG_SCALAR, IG_CELL, stateArray);
        attributes->Modified();
    }
}

bool ValidateCellsFilter::Execute() {
    ResetResults(0);

    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) {
        m_LastError = "没有输入数据对象。";
        return false;
    }

    if (DynamicCast<UnstructuredMesh>(input).IsNull() && DynamicCast<SurfaceMesh>(input).IsNull()) {
        m_LastError = GetSupportedCellTypesDescription() + " 当前输入数据类型不支持。";
        return false;
    }

    DataObject::Pointer output = CloneMeshForValidation(input);
    if (output == nullptr) {
        m_LastError = "无法复制输入网格，无法生成独立输出节点。";
        return false;
    }

    if (auto mesh = DynamicCast<UnstructuredMesh>(output)) {
        const IGsize nCells = mesh->GetNumberOfCells();
        ResetResults(nCells);

        IdArray::Pointer ids = IdArray::New();
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            const IGenum cellType = mesh->GetCellType(cellId);
            const std::vector<Point> pts = GetCellPoints(mesh, cellId, ids);
            const double tol = ComputeTolerance(pts);
            const unsigned short state = ValidateCellState(cellType, pts, tol);
            RecordCell(cellId, cellType, state);
        }

        ApplyResultAttribute(mesh.GetPointer());
        mesh->Modified();

    } else if (auto mesh = DynamicCast<SurfaceMesh>(output)) {
        const IGsize nFaces = mesh->GetNumberOfFaces();
        ResetResults(nFaces);

        for (IGsize faceId = 0; faceId < nFaces; ++faceId) {
            const std::vector<Point> pts = GetFacePoints(mesh, faceId);
            const double tol = ComputeTolerance(pts);
            const unsigned short state = ValidatePolygon(pts, tol);
            RecordCell(faceId, IG_POLYGON, state);
        }

        ApplyResultAttribute(mesh.GetPointer());
        mesh->Modified();

    } else {
        m_LastError = "内部错误：输出网格类型无效。";
        return false;
    }

    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END
