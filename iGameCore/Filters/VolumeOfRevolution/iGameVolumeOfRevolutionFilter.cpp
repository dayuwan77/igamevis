#include "iGameVolumeOfRevolutionFilter.h"
#include "iGameArrayObject.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>
IGAME_NAMESPACE_BEGIN

// ---------- 辅助函数 ----------
static void BuildAdjacency(const std::vector<RevolutionEdge>& edges, std::map<IGsize, std::set<IGsize>>& adj) {
    for (const auto& e: edges) {
        adj[e.v0].insert(e.v1);
        adj[e.v1].insert(e.v0);
    }
}

// 本过滤器只接受「2D 轮廓」：1D 曲线或 2D 面片的边界。
// 只有线段/折线才是轮廓本身，面片类型本身不是轮廓，因此下面这些类型是允许出现的。
static bool IsContourCurveCellType(IGenum cellType) {
    return cellType == IG_LINE || cellType == IG_POLY_LINE || cellType == IG_QUADRATIC_EDGE ||
           cellType == IG_LAGRANGE_CURVE;
}

static bool IsSurfaceFaceCellType(IGenum cellType) {
    return cellType == IG_TRIANGLE || cellType == IG_QUAD || cellType == IG_POLYGON ||
           cellType == IG_FACE || cellType == IG_QUADRATIC_TRIANGLE || cellType == IG_QUADRATIC_QUAD ||
           cellType == IG_QUADRATIC_POLYGON || cellType == IG_LAGRANGE_TRIANGLE ||
           cellType == IG_LAGRANGE_QUADRILATERAL;
}

// 体单元（三维实体）不是轮廓，输入只含这些单元时拒绝。
static bool IsVolumeCellType(IGenum cellType) {
    return cellType == IG_TETRA || cellType == IG_HEXAHEDRON || cellType == IG_PYRAMID ||
           cellType == IG_PRISM || cellType == IG_VOLUME || cellType == IG_POLYHEDRON ||
           cellType == IG_QUADRATIC_TETRA || cellType == IG_QUADRATIC_HEXAHEDRON ||
           cellType == IG_QUADRATIC_PRISM || cellType == IG_QUADRATIC_PYRAMID ||
           cellType == IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON ||
           cellType == IG_TRIQUADRATIC_HEXAHEDRON || cellType == IG_LAGRANGE_TETRAHEDRON ||
           cellType == IG_LAGRANGE_HEXAHEDRON || cellType == IG_LAGRANGE_PRISM ||
           cellType == IG_LAGRANGE_PYRAMID;
}

// 只取轮廓实际用到的点：体单元网格里多余的顶点不应影响有限性检查。
static bool ValidateReferencedPoints(const std::vector<Vector3d>& contourPts,
                                     const std::vector<RevolutionEdge>& edges, IGsize& badIndex,
                                     std::string& badValue) {
    std::vector<bool> used(contourPts.size(), false);
    for (const auto& edge: edges) {
        used[edge.v0] = true;
        used[edge.v1] = true;
    }
    for (IGsize i = 0; i < contourPts.size(); ++i) {
        if (!used[i]) continue;
        const Vector3d& p = contourPts[i];
        for (int c = 0; c < 3; ++c) {
            if (!std::isfinite(p[c])) {
                badIndex = i;
                badValue = std::to_string(p[c]);
                return false;
            }
        }
    }
    return true;
}
// 根据源数组类型创建一个同类型的空数组，指定元组数量
static ArrayObject::Pointer CreateEmptyArrayOfSameType(ArrayObject::Pointer src, IGsize numTuples) {
    if (!src) return nullptr;
    int dim = src->GetDimension();
    IGsize numValues = numTuples * dim;

    if (auto farr = DynamicCast<FloatArray>(src)) {
        auto copy = FloatArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto darr = DynamicCast<DoubleArray>(src)) {
        auto copy = DoubleArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto uarr = DynamicCast<UnsignedIntArray>(src)) {
        auto copy = UnsignedIntArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto iarr = DynamicCast<IntArray>(src)) {
        auto copy = IntArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto sarr = DynamicCast<ShortArray>(src)) {
        auto copy = ShortArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto usarr = DynamicCast<UnsignedShortArray>(src)) {
        auto copy = UnsignedShortArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto carr = DynamicCast<CharArray>(src)) {
        auto copy = CharArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto ucarr = DynamicCast<UnsignedCharArray>(src)) {
        auto copy = UnsignedCharArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto llarr = DynamicCast<LongLongArray>(src)) {
        auto copy = LongLongArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }
    if (auto ullarr = DynamicCast<UnsignedLongLongArray>(src)) {
        auto copy = UnsignedLongLongArray::New();
        copy->SetDimension(dim);
        copy->Resize(numValues);
        return copy;
    }

    igError("CreateEmptyArrayOfSameType: unsupported array type.");
    return nullptr;
}
// ---------- 主执行函数 ----------
// 读取输入并提取轮廓。失败时返回 false 。
static bool BuildContour(const DataObject::Pointer& input, std::vector<Vector3d>& contourPts,
                         std::vector<RevolutionEdge>& edges, std::string& buildMessage) {
    const IGenum type = input->GetDataObjectType();

    if (type == IG_UNSTRUCTURED_MESH) {
        auto inMesh = DynamicCast<UnstructuredMesh>(input);
        if (!inMesh) {
            buildMessage = "无法将输入转换为 UnstructuredMesh。";
            return false;
        }
        auto inPoints = inMesh->GetPoints();
        if (!inPoints || inPoints->GetNumberOfPoints() < 2) {
            buildMessage = "UnstructuredMesh 输入的点少于 2 个；旋转轮廓至少需要 2 个点。";
            return false;
        }
        auto cells = inMesh->GetCells();
        auto types = inMesh->GetCellTypes();
        if (!cells || !types || cells->GetNumberOfCells() == 0) {
            buildMessage = "UnstructuredMesh 输入没有单元；点云不能作为旋转轮廓。";
            return false;
        }

        const IGsize numPts = inPoints->GetNumberOfPoints();
        contourPts.resize(numPts);
        for (IGsize i = 0; i < numPts; ++i) { inPoints->GetPoint(i, contourPts[i]); }

        const IGsize numCells = cells->GetNumberOfCells();
        IGsize volumeCells = 0;
        IGsize unsupportedCells = 0;
        IGenum firstUnsupported = IG_EMPTY_CELL;
        IGenum firstDegenerate = IG_EMPTY_CELL; // 曲线单元但点数不足 2
        std::vector<bool> unsupportedTypes(IG_LAGRANGE_PYRAMID + 1, false);
        for (IGsize i = 0; i < numCells; ++i) {
            const IGenum cellType = static_cast<IGenum>(types->GetValue(i));
            if (IsContourCurveCellType(cellType)) {
                // IG_LINE 为 2 点；IG_POLY_LINE 与高阶曲线由若干段组成，逐段展开。
                const igIndex* pointIds = nullptr;
                const int count = cells->GetCellIds(i, pointIds);
                if (count < 2 || pointIds == nullptr) {
                    if (firstDegenerate == IG_EMPTY_CELL) firstDegenerate = cellType;
                    continue;
                }
                for (int k = 0; k + 1 < count; ++k) {
                    const igIndex id0 = pointIds[k];
                    const igIndex id1 = pointIds[k + 1];
                    if (id0 < 0 || id1 < 0 || static_cast<IGsize>(id0) >= numPts ||
                        static_cast<IGsize>(id1) >= numPts) {
                        buildMessage = "UnstructuredMesh 的曲线单元 " + std::to_string(i) +
                                       " 引用了点数组之外的点。";
                        return false;
                    }
                    edges.push_back({static_cast<IGsize>(id0), static_cast<IGsize>(id1), i});
                }
            } else if (IsSurfaceFaceCellType(cellType)) {
                // 面片只用来说明单元属性的来源，轮廓仍由线段/折线单元给出。
                continue;
            } else {
                if (IsVolumeCellType(cellType)) ++volumeCells;
                ++unsupportedCells;
                if (cellType >= 0 && static_cast<size_t>(cellType) < unsupportedTypes.size()) {
                    unsupportedTypes[static_cast<size_t>(cellType)] = true;
                }
                if (firstUnsupported == IG_EMPTY_CELL) firstUnsupported = cellType;
            }
        }

        if (edges.empty()) {
            const IGsize kindCount =
                    static_cast<IGsize>(std::count(unsupportedTypes.begin(), unsupportedTypes.end(), true));
            if (unsupportedCells > 0 && volumeCells == unsupportedCells) {
                buildMessage = "UnstructuredMesh 输入只包含体单元（共 " +
                               std::to_string(volumeCells) +
                               " 个，如四面体/六面体）；三维实体无法旋转成曲面，请提供二维轮廓曲线。";
            } else if (firstDegenerate != IG_EMPTY_CELL) {
                buildMessage = "UnstructuredMesh 中存在曲线单元，但没有一个单元有至少 2 个点；轮廓连接无效。";
            } else {
                buildMessage = "UnstructuredMesh 输入中没有可用作轮廓的线段单元（共 " +
                               std::to_string(numCells) + " 个单元、" + std::to_string(kindCount) +
                               " 种单元类型，例如类型 " + std::to_string(firstUnsupported) + "（" +
                               GetCellTypeAsString(firstUnsupported) + "））；需要由 Line/PolyLine 单元构成的二维轮廓。";
            }
            return false;
        }
    } else if (type == IG_SURFACE_MESH) {
        auto surf = DynamicCast<SurfaceMesh>(input);
        if (!surf) {
            buildMessage = "无法将输入转换为 SurfaceMesh。";
            return false;
        }
        auto pts = surf->GetPoints();
        if (!pts || pts->GetNumberOfPoints() < 3) {
            buildMessage = "SurfaceMesh 输入的点少于 3 个；其边界无法构成二维轮廓。";
            return false;
        }
        if (surf->GetNumberOfFaces() == 0) {
            buildMessage = "SurfaceMesh 输入没有面片；其边界无法构成二维轮廓。";
            return false;
        }
        // 仅在确实要读取拓扑信息时才申请编辑状态（原先在类型转换检查之前调用，空指针会崩溃）。
        surf->RequestEditStatus();

        const IGsize numPts = pts->GetNumberOfPoints();
        contourPts.resize(numPts);
        for (IGsize i = 0; i < numPts; ++i) { pts->GetPoint(i, contourPts[i]); }

        const IGsize numEdges = surf->GetNumberOfEdges();
        for (IGsize eid = 0; eid < numEdges; ++eid) {
            if (!surf->IsBoundaryEdge(eid)) continue;
            igIndex ids[2];
            if (surf->GetEdgePointIds(eid, ids) != 2) continue;
            igIndex faceIds[1];
            const int numFaces = surf->GetEdgeToNeighborFaces(eid, faceIds);
            const IGsize cellId = (numFaces == 1) ? static_cast<IGsize>(faceIds[0]) : static_cast<IGsize>(-1);
            edges.push_back({static_cast<IGsize>(ids[0]), static_cast<IGsize>(ids[1]), cellId});
        }
        if (edges.empty()) {
            buildMessage = "SurfaceMesh 输入没有边界边：封闭曲面（如球面）不能作为二维旋转轮廓。";
            return false;
        }
    } else if (type == IG_STRUCTURED_MESH) {
        auto sMesh = DynamicCast<StructuredMesh>(input);
        if (!sMesh) {
            buildMessage = "无法将输入转换为 StructuredMesh。";
            return false;
        }
        auto pts = sMesh->GetPoints();
        if (!pts || pts->GetNumberOfPoints() < 2) {
            buildMessage = "StructuredMesh 输入的点少于 2 个；旋转轮廓至少需要 2 个点。";
            return false;
        }
        const igIndex* dim = sMesh->GetDimensionSize();
        const IGsize ni = dim[0], nj = dim[1], nk = dim[2];
        if (ni < 1 || nj < 1 || nk < 1) {
            buildMessage = "StructuredMesh 输入存在退化维度（" + std::to_string(ni) + "x" +
                           std::to_string(nj) + "x" + std::to_string(nk) + "）。";
            return false;
        }
        if (nk > 1) {
            buildMessage = "StructuredMesh 输入是三维体网格（" + std::to_string(ni) + "x" +
                           std::to_string(nj) + "x" + std::to_string(nk) +
                           "）；需要一维曲线或二维曲面（nk==1）。";
            return false;
        }
        const IGsize expectedPts = ni * nj * nk;
        const IGsize actualPts = pts->GetNumberOfPoints();
        if (actualPts != expectedPts) {
            buildMessage = "StructuredMesh 的点数（" + std::to_string(actualPts) +
                           "）与其维度（" + std::to_string(ni) + "x" + std::to_string(nj) +
                           "x" + std::to_string(nk) + "）不一致。";
            return false;
        }
        contourPts.resize(actualPts);
        for (IGsize idx = 0; idx < actualPts; ++idx) { pts->GetPoint(idx, contourPts[idx]); }

        auto idxOf = [&](IGsize i, IGsize j, IGsize k) -> IGsize { return i + j * ni + k * ni * nj; };
        if (nj == 1) {
            // 1D 曲线，无单元概念，cellId 设为 -1（不复制单元属性）
            for (IGsize i = 0; i + 1 < ni; ++i) {
                edges.push_back({idxOf(i, 0, 0), idxOf(i + 1, 0, 0), static_cast<IGsize>(-1)});
            }
        } else {
            // 2D 曲面，获取所有面片
            auto faces = sMesh->GetFaces();
            if (!faces || faces->GetNumberOfCells() == 0) {
                buildMessage = "StructuredMesh 输入没有面片；需要二维曲面。";
                return false;
            }
            // 构建边 -> 面片ID列表的映射
            std::map<std::pair<IGsize, IGsize>, std::vector<IGsize>> edgeFaceMap;
            const IGsize numFaces = faces->GetNumberOfCells();
            for (IGsize f = 0; f < numFaces; ++f) {
                igIndex facePts[4];
                const int npts = faces->GetCellIds(f, facePts); // 应为四边形
                if (npts < 3) continue;
                for (int e = 0; e < npts; ++e) {
                    IGsize a = static_cast<IGsize>(facePts[e]);
                    IGsize b = static_cast<IGsize>(facePts[(e + 1) % npts]);
                    if (a > b) std::swap(a, b);
                    edgeFaceMap[{a, b}].push_back(f);
                }
            }
            // 筛选边界边
            for (auto& kv: edgeFaceMap) {
                if (kv.second.size() == 1) {
                    edges.push_back({kv.first.first, kv.first.second, kv.second[0]});
                }
            }
            if (edges.empty()) {
                buildMessage = "StructuredMesh 边界提取失败：该结构曲面没有边界边（封闭曲面）。";
                return false;
            }
        }
    } else {
        buildMessage = "iGameVolumeOfRevolution 不支持该数据类型（" +
                       std::to_string(static_cast<int>(type)) + "（" + GetCellTypeAsString(type) +
                       "））；支持的输入为 UnstructuredMesh（Line/PolyLine 单元）、SurfaceMesh 和 "
                       "StructuredMesh。";
        return false;
    }

    if (contourPts.size() < 2 || edges.empty()) {
        buildMessage = "输入中没有找到有效的轮廓点或轮廓边。";
        return false;
    }
    for (IGsize i = 0; i < edges.size(); ++i) {
        if (edges[i].v0 == edges[i].v1) {
            buildMessage = "轮廓边 " + std::to_string(i) + " 已退化（两端都是点 " +
                           std::to_string(edges[i].v0) + "）。";
            return false;
        }
    }
    return true;
}

// ---------- 主执行函数 ----------
bool VolumeOfRevolutionFilter::Execute() {
    m_Message.clear();
    // 与 iGameLinearExtrusionFilter 一致：执行失败时不保留上一次的结果
    this->SetOutput(0, nullptr);

    DataObject::Pointer input = GetInput(0);
    if (!input) {
        m_Message = "旋转体生成：没有输入数据对象。";
        igError("{}", m_Message);
        return false;
    }

    std::vector<Vector3d> contourPts;
    std::vector<RevolutionEdge> edges;

    std::string buildMessage;
    if (!BuildContour(input, contourPts, edges, buildMessage)) {
        m_Message = buildMessage;
        igError("{}", m_Message);
        return false;
    }

    // -------- 旋转轴与角度合法性（参数不符合条件同样要拒绝，而不是产出错误网格） --------
    Vector3d axisDir = m_AxisDirection;
    double len = axisDir.norm();
    if (!std::isfinite(len) || len < 1e-12) {
        m_Message = "旋转轴方向必须是非零且有限的向量。";
        igError("{}", m_Message);
        return false;
    }
    axisDir /= len;
    Vector3d axisPt = m_AxisPoint;
    for (int c = 0; c < 3; ++c) {
        if (!std::isfinite(axisPt[c])) {
            m_Message = "轴上一点的坐标必须是有限值。";
            igError("{}", m_Message);
            return false;
        }
    }
    const double angle = m_Angle;
    if (!std::isfinite(angle)) {
        m_Message = "旋转角度必须是有限值。";
        igError("{}", m_Message);
        return false;
    }
    if (std::fabs(angle) < 1e-12) {
        m_Message = "旋转角度为 0；结果会退化为零面积曲面。";
        igError("{}", m_Message);
        return false;
    }
    if (std::fabs(angle) > 2.0 * 3.141592653589793 + 1e-9) {
        m_Message = "旋转角度不能超过一整圈（360 度）。";
        igError("{}", m_Message);
        return false;
    }
    if (m_Resolution < 3) {
        m_Message = "分段数至少为 3。";
        igError("{}", m_Message);
        return false;
    }

    // -------- 轮廓几何合法性 --------
    IGsize numPts = contourPts.size();
    // 只检查轮廓实际用到的点（体单元网格可能带有未被轮廓引用的多余顶点）。
    IGsize badIndex = 0;
    std::string badValue;
    if (!ValidateReferencedPoints(contourPts, edges, badIndex, badValue)) {
        m_Message = "轮廓点 " + std::to_string(badIndex) + " 不是有限值（" + badValue +
                    "）；轮廓无效。";
        igError("{}", m_Message);
        return false;
    }

    double minWithAxis1 = axisPt[0], maxWithAxis1 = axisPt[0];
    double minWithAxis2 = axisPt[1], maxWithAxis2 = axisPt[1];
    double minAlongAxis = axisPt[2], maxAlongAxis = axisPt[2];
    for (IGsize i = 0; i < numPts; ++i) {
        minWithAxis1 = std::min(minWithAxis1, contourPts[i][0]);
        maxWithAxis1 = std::max(maxWithAxis1, contourPts[i][0]);
        minWithAxis2 = std::min(minWithAxis2, contourPts[i][1]);
        maxWithAxis2 = std::max(maxWithAxis2, contourPts[i][1]);
        minAlongAxis = std::min(minAlongAxis, contourPts[i][2]);
        maxAlongAxis = std::max(maxAlongAxis, contourPts[i][2]);
    }
    const double withAxisExtent = std::max(maxWithAxis1 - minWithAxis1, maxWithAxis2 - minWithAxis2);
    const double alongAxisExtent = maxAlongAxis - minAlongAxis;
    const double tolerance =
            1e-9 * std::max(1.0, std::max(static_cast<double>(withAxisExtent), alongAxisExtent));

    // -------- 预计算轮廓点投影 --------
    std::vector<PointProjection> proj(numPts);
    double maxRadius = 0.0;
    for (IGsize i = 0; i < numPts; ++i) {
        Vector3d v = contourPts[i] - axisPt;
        double h = v.dot(axisDir);
        Vector3d v_par = h * axisDir;
        Vector3d v_perp = v - v_par;
        proj[i].r = v_perp.norm();
        proj[i].h = h;
        proj[i].v_perp = v_perp;
        maxRadius = std::max(maxRadius, proj[i].r);
    }

    // 所有轮廓点都落在旋转轴上：旋转退化，只会得到零面积三角形。
    if (maxRadius <= tolerance) {
        m_Message = "轮廓的所有点都落在旋转轴上；旋转结果会退化。";
        igError("{}", m_Message);
        return false;
    }

    // 轮廓退化成一条直线段（严格共线），并且旋转轴与这条直线平行：
    // 旋转结果的所有三角形都退化为零面积，必须拒绝。
    // 注意：这里用三个点构成的最小面积来判断严格共线；只"恰好前两点共线"不算。
    {
        double minCross = 0.0;
        bool hasCross = false;
        for (IGsize i = 0; i + 2 < numPts; ++i) {
            for (IGsize j = i + 1; j + 1 < numPts; ++j) {
                const Vector3d edge1 = contourPts[j] - contourPts[i];
                for (IGsize k = j + 1; k < numPts; ++k) {
                    const double crossNorm = edge1.cross(contourPts[k] - contourPts[i]).norm();
                    if (!hasCross || crossNorm < minCross) {
                        minCross = crossNorm;
                        hasCross = true;
                    }
                }
            }
        }
        if (hasCross && minCross <= tolerance) {
            // 严格共线：轮廓方向就是首尾两点连线方向
            Vector3d lineDir = contourPts[numPts - 1] - contourPts[0];
            const double lineLength = lineDir.norm();
            if (lineLength > tolerance) {
                lineDir /= lineLength;
                if (std::fabs(lineDir.dot(axisDir)) > 1.0 - 1e-9) {
                    m_Message = "轮廓是一条直线段（所有点严格共线），而旋转轴与它平行；"
                                "生成的三角形面积全为零。请提供非共线的二维轮廓，或改用不与线段平行的旋转轴。";
                    igError("{}", m_Message);
                    return false;
                }
            }
        }
    }

    // -------- 角度参数 --------
    double angleStep = m_Angle / m_Resolution;
    const double PI = 3.141592653589793;
    int numTheta = m_Resolution + 1;

    // -------- 判断是否为完整圆周旋转，处理+/-360的情况 --------
    bool isFull = (fabs(fabs(m_Angle) - 2.0 * PI) < 1e-12);

    // -------- 生成旋转点云 --------
    auto newPoints = Points::New();
    std::vector<std::vector<IGsize>> pointIndices(numPts);
    for (auto& vec: pointIndices) vec.resize(numTheta);

    for (IGsize iPt = 0; iPt < numPts; ++iPt) {
        double r = proj[iPt].r;
        double h = proj[iPt].h;
        Vector3d v_par = h * axisDir;
        Vector3d v_perp = proj[iPt].v_perp;
        for (int j = 0; j < numTheta; ++j) {
            // -------- 若为全周且为最后一层，复用第一层点索引 --------
            if (isFull && j == m_Resolution) {
                pointIndices[iPt][j] = pointIndices[iPt][0];
                continue;
            }
            double theta = j * angleStep;
            Vector3d v_rot;
            if (r < 1e-12) {
                v_rot = Vector3d(0, 0, 0);
            } else {
                Vector3d cross = axisDir.cross(v_perp);
                v_rot = v_perp * std::cos(theta) + cross * std::sin(theta);
            }
            Vector3d newP = axisPt + v_par + v_rot;
            IGsize idx = newPoints->AddPoint(newP[0], newP[1], newP[2]);
            pointIndices[iPt][j] = idx;
        }
    }

    // -------- 生成侧面三角形 --------
    auto newCells = CellArray::New();
    auto newTypes = UnsignedIntArray::New();
    std::vector<IGsize> triCellIds;
    for (const auto& edge: edges) {
        IGsize i0 = edge.v0, i1 = edge.v1;
        IGsize cellId = edge.cellId;
        for (int j = 0; j < m_Resolution; ++j) {
            int j_next = j + 1;
            IGsize ids[4] = {pointIndices[i0][j], pointIndices[i1][j], pointIndices[i1][j_next],
                             pointIndices[i0][j_next]};
            bool deg0 = (proj[i0].r < 1e-9);
            bool deg1 = (proj[i1].r < 1e-9);
            if (deg0 && deg1) continue;
            else if (deg0) {
                igIndex tri[3] = {static_cast<igIndex>(ids[1]), static_cast<igIndex>(ids[2]),
                                  static_cast<igIndex>(ids[3])};
                newCells->AddCellIds(tri, 3);
                triCellIds.push_back(cellId);
                newTypes->AddValue(IG_TRIANGLE);
            } else if (deg1) {
                igIndex tri[3] = {static_cast<igIndex>(ids[0]), static_cast<igIndex>(ids[1]),
                                  static_cast<igIndex>(ids[3])};
                newCells->AddCellIds(tri, 3);
                triCellIds.push_back(cellId);
                newTypes->AddValue(IG_TRIANGLE);
            } else {
                igIndex tri1[3] = {static_cast<igIndex>(ids[0]), static_cast<igIndex>(ids[1]),
                                   static_cast<igIndex>(ids[2])};
                newCells->AddCellIds(tri1, 3);
                triCellIds.push_back(cellId);
                newTypes->AddValue(IG_TRIANGLE);
                igIndex tri2[3] = {static_cast<igIndex>(ids[0]), static_cast<igIndex>(ids[2]),
                                   static_cast<igIndex>(ids[3])};
                newCells->AddCellIds(tri2, 3);
                triCellIds.push_back(cellId);
                newTypes->AddValue(IG_TRIANGLE);
            }
        }
    }

    // -------- 输出 --------
    auto outputMesh = UnstructuredMesh::New();
    outputMesh->SetPoints(newPoints);
    outputMesh->SetCells(newCells, newTypes);

    // -------- 复制点属性 --------
    auto inputAttrs = input->GetAttributeSet();
    if (inputAttrs) {
        auto pointAttrs = inputAttrs->GetAllPointAttributes();
        if (pointAttrs && pointAttrs->GetNumberOfElements() > 0) {
            const IGsize numOutPts = newPoints->GetNumberOfPoints();
            auto outputAttrs = outputMesh->GetAttributeSet();
            if (!outputAttrs) {
                outputAttrs = AttributeSet::New();
                outputMesh->SetAttributeSet(outputAttrs);
            }

            for (int a = 0; a < pointAttrs->GetNumberOfElements(); ++a) {
                auto& attr = pointAttrs->GetElement(a);
                if (attr.IsDeleted() || !attr.pointer) continue;

                auto inArray = attr.pointer;
                int dim = inArray->GetDimension();
                auto outArray = CreateEmptyArrayOfSameType(inArray, numOutPts);
                if (!outArray) continue;

                outArray->SetName(inArray->GetName());

                // 填充数据：每个原始点 i 复制到所有层 j (j=0..m_Resolution-1)
                for (IGsize i = 0; i < numPts; ++i) {
                    for (int j = 0; j < m_Resolution; ++j) {
                        IGsize outIdx = pointIndices[i][j];
                        for (int d = 0; d < dim; ++d) {
                            double val = inArray->GetValue(i * dim + d);
                            outArray->SetValue(outIdx * dim + d, val);
                        }
                    }
                }

                // 按类型添加，保留数据范围
                DoubleArray::Pointer range = attr.GetDataRange();
                if (attr.type == IG_SCALAR) {
                    if (range) outputAttrs->AddScalar(IG_POINT, outArray, range);
                    else outputAttrs->AddScalar(IG_POINT, outArray);
                } else if (attr.type == IG_VECTOR) {
                    if (range) outputAttrs->AddVector(IG_POINT, outArray, range);
                    else outputAttrs->AddVector(IG_POINT, outArray);
                } else {
                    if (range) outputAttrs->AddAttribute(attr.type, IG_POINT, outArray, range);
                    else outputAttrs->AddAttribute(attr.type, IG_POINT, outArray);
                }
            }
        }
    }

    // -------- 复制单元属性 --------
    // 只有轮廓边全部对应到真实的输入单元时才复制：cellId 为 -1（顺序连接的曲线、StructuredMesh 的 1D 曲线）没有可继承的单元属性。
    bool hasValidCellIds = true;
    for (auto& e: edges) {
        if (e.cellId == static_cast<IGsize>(-1)) {
            hasValidCellIds = false;
            break;
        }
    }

    if (inputAttrs && hasValidCellIds && !triCellIds.empty()) {
        auto cellAttrs = inputAttrs->GetAllCellAttributes();
        if (cellAttrs && cellAttrs->GetNumberOfElements() > 0) {
            IGsize numOutCells = triCellIds.size();
            auto outputAttrs = outputMesh->GetAttributeSet();
            if (!outputAttrs) {
                outputAttrs = AttributeSet::New();
                outputMesh->SetAttributeSet(outputAttrs);
            }

            for (int a = 0; a < cellAttrs->GetNumberOfElements(); ++a) {
                auto& attr = cellAttrs->GetElement(a);
                if (attr.IsDeleted() || !attr.pointer) continue;

                auto inArray = attr.pointer;
                int dim = inArray->GetDimension();
                auto outArray = CreateEmptyArrayOfSameType(inArray, numOutCells);
                if (!outArray) continue;

                outArray->SetName(inArray->GetName());

                // 按 triCellIds 顺序从输入单元属性中取值
                for (IGsize t = 0; t < numOutCells; ++t) {
                    IGsize cellId = triCellIds[t];
                    if (cellId < inArray->GetNumberOfValues() / dim) {
                        for (int d = 0; d < dim; ++d) {
                            double val = inArray->GetValue(cellId * dim + d);
                            outArray->SetValue(t * dim + d, val);
                        }
                    } else {
                        // 若 cellId 无效，置 0
                        for (int d = 0; d < dim; ++d) outArray->SetValue(t * dim + d, 0.0);
                    }
                }

                // 添加到输出
                DoubleArray::Pointer range = attr.GetDataRange();
                if (attr.type == IG_SCALAR) {
                    if (range) outputAttrs->AddScalar(IG_CELL, outArray, range);
                    else outputAttrs->AddScalar(IG_CELL, outArray);
                } else if (attr.type == IG_VECTOR) {
                    if (range) outputAttrs->AddVector(IG_CELL, outArray, range);
                    else outputAttrs->AddVector(IG_CELL, outArray);
                } else {
                    if (range) outputAttrs->AddAttribute(attr.type, IG_CELL, outArray, range);
                    else outputAttrs->AddAttribute(attr.type, IG_CELL, outArray);
                }
            }
        }
    }

    if (newCells->GetNumberOfCells() == 0) {
        m_Message = "生成的旋转曲面没有单元；请检查轮廓是否全部落在旋转轴上。";
        igError("{}", m_Message);
        return false;
    }

    SetOutput(0, outputMesh);
    return true;
}

IGAME_NAMESPACE_END