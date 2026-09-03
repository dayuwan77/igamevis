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
static void BuildAdjacency(const std::vector<Edge>& edges, std::map<IGsize, std::set<IGsize>>& adj) {
    for (const auto& e: edges) {
        adj[e.v0].insert(e.v1);
        adj[e.v1].insert(e.v0);
    }
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
bool VolumeOfRevolutionFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (!input) return false;

    std::vector<Vector3d> contourPts;
    std::vector<Edge> edges;

    IGenum type = input->GetDataObjectType();

    if (type == IG_UNSTRUCTURED_MESH) {
        auto inMesh = DynamicCast<UnstructuredMesh>(input);
        if (!inMesh) {
            igError("Failed to cast to UnstructuredMesh.");
            return false;
        }
        auto inPoints = inMesh->GetPoints();
        if (!inPoints || inPoints->GetNumberOfPoints() < 2) {
            igError("UnstructuredMesh has insufficient points.");
            return false;
        }
        IGsize numPts = inPoints->GetNumberOfPoints();
        contourPts.resize(numPts);
        for (IGsize i = 0; i < numPts; ++i) { inPoints->GetPoint(i, contourPts[i]); }
        auto cells = inMesh->GetCells();
        auto types = inMesh->GetCellTypes();
        if (cells && types) {
            IGsize numCells = cells->GetNumberOfCells();
            for (IGsize i = 0; i < numCells; ++i) {
                if (types->GetValue(i) == IG_LINE) {
                    igIndex ids[2];
                    if (cells->GetCellIds(i, ids) == 2) {
                        edges.push_back({static_cast<IGsize>(ids[0]), static_cast<IGsize>(ids[1]),i});
                    }
                }
            }
        }
        if (edges.empty() && numPts > 1) {
            for (IGsize i = 0; i < numPts - 1; ++i) { edges.push_back({i, i + 1}); }
        }
    } else if (type == IG_SURFACE_MESH) {
        auto surf = DynamicCast<SurfaceMesh>(input);
        surf->RequestEditStatus();
        if (!surf) {
            igError("Failed to cast to SurfaceMesh.");
            return false;
        }
        auto pts = surf->GetPoints();
        if (!pts || pts->GetNumberOfPoints() < 3) {
            igError("SurfaceMesh has insufficient points.");
            return false;
        }
        IGsize numPts = pts->GetNumberOfPoints();
        contourPts.resize(numPts);
        for (IGsize i = 0; i < numPts; ++i) { pts->GetPoint(i, contourPts[i]); }
        IGsize numEdges = surf->GetNumberOfEdges();
        for (IGsize eid = 0; eid < numEdges; ++eid) {
            if (surf->IsBoundaryEdge(eid)) {
                igIndex ids[2];
                if (surf->GetEdgePointIds(eid, ids) == 2) {
                    igIndex faceIds[1];
                    int numFaces = surf->GetEdgeToNeighborFaces(eid, faceIds);
                    IGsize cellId = (numFaces == 1) ? static_cast<IGsize>(faceIds[0]) : static_cast<IGsize>(-1);
                    edges.push_back({static_cast<IGsize>(ids[0]), static_cast<IGsize>(ids[1]),cellId});
                }
            }
        }
        if (edges.empty()) {
            igError("SurfaceMesh has no boundary edges (not a valid 2D contour).");
            return false;
        }
    } else if (type == IG_STRUCTURED_MESH) {
        auto sMesh = DynamicCast<StructuredMesh>(input);
        if (!sMesh) {
            igError("Failed to cast to StructuredMesh.");
            return false;
        }
        auto pts = sMesh->GetPoints();
        if (!pts || pts->GetNumberOfPoints() < 2) {
            igError("StructuredMesh has insufficient points.");
            return false;
        }
        igIndex* dim = sMesh->GetDimensionSize();
        IGsize ni = dim[0], nj = dim[1], nk = dim[2];
        IGsize expectedPts = ni * nj * nk;
        IGsize actualPts = pts->GetNumberOfPoints();
        if (actualPts != expectedPts) {
            igError("StructuredMesh point count does not match dimensions.");
            return false;
        }
        contourPts.resize(actualPts);
        for (IGsize idx = 0; idx < actualPts; ++idx) { pts->GetPoint(idx, contourPts[idx]); }

        auto idxOf = [&](IGsize i, IGsize j, IGsize k) -> IGsize { return i + j * ni + k * ni * nj; };
        std::set<std::pair<IGsize, IGsize>> edgeSet;
        auto addEdge = [&](IGsize a, IGsize b) {
            if (a > b) std::swap(a, b);
            edgeSet.insert({a, b});
        };
        if (nj == 1 && nk == 1) {
            for (IGsize i = 0; i < ni - 1; ++i) { addEdge(idxOf(i, 0, 0), idxOf(i + 1, 0, 0)); }
        } else if (nk == 1) {
            for (IGsize i = 0; i < ni - 1; ++i) addEdge(idxOf(i, 0, 0), idxOf(i + 1, 0, 0));
            for (IGsize i = 0; i < ni - 1; ++i) addEdge(idxOf(i, nj - 1, 0), idxOf(i + 1, nj - 1, 0));
            for (IGsize j = 0; j < nj - 1; ++j) addEdge(idxOf(0, j, 0), idxOf(0, j + 1, 0));
            for (IGsize j = 0; j < nj - 1; ++j) addEdge(idxOf(ni - 1, j, 0), idxOf(ni - 1, j + 1, 0));
        } else {
            igError("StructuredMesh must be 1D curve or 2D surface (nk==1).");
            return false;
        }
        if (nj == 1 && nk == 1) {
            // 1D 曲线，无单元概念，cellId 设为 -1
            for (IGsize i = 0; i < ni - 1; ++i) {
                edges.push_back({idxOf(i,0,0), idxOf(i+1,0,0), static_cast<IGsize>(-1)});
            }
        } else if (nk == 1) {
            // 2D 曲面，获取所有面片
            auto faces = sMesh->GetFaces();
            if (!faces || faces->GetNumberOfCells() == 0) {
                igError("StructuredMesh has no faces.");
                return false;
            }
            // 构建边 -> 面片ID列表的映射
            std::map<std::pair<IGsize, IGsize>, std::vector<IGsize>> edgeFaceMap;
            IGsize numFaces = faces->GetNumberOfCells();
            for (IGsize f = 0; f < numFaces; ++f) {
                igIndex pts[4];
                int npts = faces->GetCellIds(f, pts);  // 应为四边形
                if (npts != 4) continue;
                for (int e = 0; e < 4; ++e) {
                    IGsize a = pts[e], b = pts[(e+1)%4];
                    if (a > b) std::swap(a, b);
                    edgeFaceMap[{a, b}].push_back(f);
                }
            }
            // 筛选边界边
            for (auto& kv : edgeFaceMap) {
                if (kv.second.size() == 1) {
                    auto& edge = kv.first;
                    IGsize cellId = kv.second[0];
                    edges.push_back({edge.first, edge.second, cellId});
                }
            }
            if (edges.empty()) {
                igError("StructuredMesh boundary extraction failed.");
                return false;
            }
        }
        if (edges.empty()) {
            igError("StructuredMesh boundary extraction failed.");
            return false;
        }
    } else {
        igError("iGameVolumeOfRevolution does not support this data type.");
        return false;
    }

    if (contourPts.size() < 2 || edges.empty()) {
        igError("No valid contour points or edges found.");
        return false;
    }

    // -------- 旋转轴归一化 --------
    Vector3d axisDir = m_AxisDirection;
    double len = axisDir.norm();
    if (len < 1e-12) {
        igError("Axis direction is zero.");
        return false;
    }
    axisDir /= len;
    Vector3d axisPt = m_AxisPoint;

    // -------- 预计算轮廓点投影 --------
    IGsize numPts = contourPts.size();
    std::vector<PointProjection> proj(numPts);
    for (IGsize i = 0; i < numPts; ++i) {
        Vector3d v = contourPts[i] - axisPt;
        double h = v.dot(axisDir);
        Vector3d v_par = h * axisDir;
        Vector3d v_perp = v - v_par;
        proj[i].r = v_perp.norm();
        proj[i].h = h;
        proj[i].v_perp = v_perp;
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
    auto inputAttrs = input->GetAttributeSet();
if (inputAttrs) {
    auto pointAttrs = inputAttrs->GetAllPointAttributes();
    if (pointAttrs && pointAttrs->GetNumberOfElements() > 0) {
        IGsize numOutPts = newPoints->GetNumberOfPoints();  // = numPts * m_Resolution
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
bool hasValidCellIds = true;
for (auto& e : edges) {
    if (e.cellId == static_cast<IGsize>(-1)) {
        hasValidCellIds = false;
        break;
    }
}

if (hasValidCellIds && !triCellIds.empty()) {
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
    SetOutput(0, outputMesh);
    return true;
}

IGAME_NAMESPACE_END