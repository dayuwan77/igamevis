#include "iGameIsoVolumeFilter.h"
#include "iGameThreadPool.h"
#include "iGameHexClipCases.h"    // 六面体裁剪 case 表
#include "iGameCellClipCases.h"   // 楔形/金字塔/四面体裁剪 case 表

#include <unordered_map>
#include <cstring>
#include <cstdint>

IGAME_NAMESPACE_BEGIN

namespace {
// 按输入数组类型创建同类型数组，避免统一转成 FloatArray 造成类型/精度丢失
ArrayObject::Pointer CreateSameTypeArray(ArrayObject::Pointer in) {
    ArrayObject::Pointer out;
    switch (in->GetArrayType()) {
        case IG_DoubleArray:           out = DoubleArray::New();           break;
        case IG_IntArray:              out = IntArray::New();              break;
        case IG_UnsignedIntArray:      out = UnsignedIntArray::New();      break;
        case IG_CharArray:             out = CharArray::New();             break;
        case IG_UnsignedCharArray:     out = UnsignedCharArray::New();     break;
        case IG_ShortArray:            out = ShortArray::New();            break;
        case IG_UnsignedShortArray:    out = UnsignedShortArray::New();    break;
        case IG_LongLongArray:         out = LongLongArray::New();         break;
        case IG_UnsignedLongLongArray: out = UnsignedLongLongArray::New(); break;
        case IG_FloatArray:
        default:                       out = FloatArray::New();            break;
    }
    out->SetName(in->GetName());
    out->SetDimension(in->GetDimension());
    return out;
}
}  // namespace

IsoVolumeFilter::IsoVolumeFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

IsoVolumeFilter::~IsoVolumeFilter() {}

void IsoVolumeFilter::SetIsoScalarData(ArrayObject::Pointer array, double lower, double upper, int dimension) {
    this->m_SelectedScalar = array;
    if (array) {
        this->m_SelectedScalarName = array->GetName();
    }
    this->m_LowerValue = lower;
    this->m_UpperValue = upper;
    this->m_SelectDimension = static_cast<double>(dimension);
}

bool IsoVolumeFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }

    // 若输入是 DrawObject(.pvd / 时序等容器对象):
    //   1) 含时间帧时, 取第 m_TimeStep 帧的网格; 走 StreamingData::GetTargetTimeFrameData,
    //      命中帧缓存时直接复用(反复切帧/播放时不再重复读盘), 未命中则读盘并写回缓存;
    //   2) 否则退化为使用容器内第一个子网格。
    if (input->GetDataObjectType() == IG_DRAW_OBJECT) {
        DataObject::Pointer resolved = nullptr;

        auto frames = input->PeekTimeFrames();
        if (frames && frames->GetTimeNum() > 0) {
            int idx = m_TimeStep;
            if (idx < 0) { idx = 0; }
            if (idx >= (int)frames->GetTimeNum()) { idx = (int)frames->GetTimeNum() - 1; }
            auto frameData = frames->GetTargetTimeFrameData(idx);
            // 收集该帧的各个分块(并行分区可能一帧多块)
            std::vector<UnstructuredMesh::Pointer> parts;
            for (auto& obj : frameData) {
                auto mesh = DynamicCast<DataObject>(obj);
                if (mesh && mesh->GetDataObjectType() != IG_DRAW_OBJECT) {
                    auto um = DynamicCast<UnstructuredMesh>(mesh);
                    if (um) {
                        parts.push_back(um);
                        if (!resolved) { resolved = mesh; }
                    }
                }
            }
            // 有时序但该帧取不到 -> 直接失败, 避免静默返回其它帧的结果
            if (parts.empty() || !resolved) { return false; }

            // 多分块帧: 逐块提取后合并成一个网格。
            // 各分块互不重叠, 因此逐块处理在几何上等价于整体处理。
            if (parts.size() > 1) {
                std::vector<UnstructuredMesh::Pointer> partResults;
                for (auto& p : parts) {
                    auto one = UnstructuredMesh::New();
                    if (this->ExtractToMesh(p, one) && one->GetNumberOfCells() > 0) {
                        partResults.push_back(one);
                    }
                }
                if (partResults.empty()) { return false; }
                if (partResults.size() == 1) {
                    this->SetOutput(0, partResults[0]);
                } else {
                    auto merged = this->MergeParts(partResults);
                    if (!merged) { return false; }
                    this->SetOutput(0, merged);
                }
                return true;
            }
        }
        else if (input->HasSubDataObject()) {
            resolved = input->SubDataObjectIteratorBegin()->second;
        }
        if (resolved) { input = resolved; }

        // 容器上的同名数组可能只是元数据(无数据), 需在网格上按名称重新解析出
        // 真正带数据的点标量数组, 否则按网格点数取值会越界/空指针崩溃
        if (input && !m_SelectedScalarName.empty()) {
            auto subAttrs = input->GetAttributeSet();
            if (subAttrs) {
                auto pts = subAttrs->GetAllPointAttributes();
                if (pts) {
                    for (int i = 0; i < (int)pts->GetNumberOfElements(); ++i) {
                        auto& e = pts->GetElement(i);
                        if (e.pointer && e.pointer->GetName() == m_SelectedScalarName) {
                            m_SelectedScalar = e.pointer;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (!input) { return false; }

    switch (input->GetDataObjectType()) {
        case IG_NONE:
            return true;
        case IG_VOLUME_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        case IG_SURFACE_MESH:
            return this->ExecuteWithSurfaceMesh(DynamicCast<SurfaceMesh>(input));
        case IG_UNSTRUCTURED_MESH:
            return this->ExecuteWithUnstructuredMesh(DynamicCast<UnstructuredMesh>(input));
        case IG_STRUCTURED_MESH:
            return this->ExecuteWithVolumeMesh(DynamicCast<VolumeMesh>(input));
        default:
            return false;
    }
}

bool IsoVolumeFilter::ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer input) {
    if (!input) return false;
    auto output = UnstructuredMesh::New();
    if (!this->ExtractToMesh(input, output)) { return false; }
    this->SetOutput(0, output);
    return true;
}

// 在单个网格上完成提取: 解析点标量 -> 保留 >= lower -> 再保留 <= upper
bool IsoVolumeFilter::ExtractToMesh(UnstructuredMesh::Pointer input, UnstructuredMesh::Pointer output) {
    if (!input || !output) return false;

    // 各分块(以及独立模型)的属性是独立的, 按名称在当前网格上解析出带数据的点标量数组
    if (!m_SelectedScalarName.empty()) {
        auto attrs = input->GetAttributeSet();
        if (attrs) {
            auto pts = attrs->GetAllPointAttributes();
            if (pts) {
                for (int i = 0; i < (int)pts->GetNumberOfElements(); ++i) {
                    auto& e = pts->GetElement(i);
                    if (e.pointer && e.pointer->GetName() == m_SelectedScalarName) {
                        m_SelectedScalar = e.pointer;
                        break;
                    }
                }
            }
        }
    }
    if (!m_SelectedScalar) return false;

    // 防御: 标量数组元素数应不少于网格点数, 否则(例如拿到空数组)直接失败而非越界崩溃
    if (m_SelectedScalar->GetNumberOfElements() < (IGsize)input->GetNumberOfPoints()) {
        return false;
    }

    // 第一步：保留标量值 >= LowerValue 的部分
    auto lowerClipped = UnstructuredMesh::New();
    if (!ClipMeshByScalar(input, m_SelectedScalar, m_LowerValue, true, lowerClipped)) {
        return false;
    }

    // 第二步：在第一步结果上保留标量值 <= UpperValue 的部分
    // 需要从中间网格的 AttributeSet 中找到同名标量数组
    ArrayObject::Pointer scalarArray = m_SelectedScalar;
    auto attrSet = lowerClipped->GetAttributeSet();
    if (attrSet && !m_SelectedScalarName.empty()) {
        auto attr = attrSet->GetAttribute(m_SelectedScalarName);
        if (!attr.IsNone() && attr.pointer) {
            scalarArray = attr.pointer;
        }
    }

    if (!ClipMeshByScalar(lowerClipped, scalarArray, m_UpperValue, false, output)) {
        return false;
    }
    return true;
}

// 合并多个分块的提取结果: 点/单元按偏移拼接, 属性按相同布局依次拼接
UnstructuredMesh::Pointer IsoVolumeFilter::MergeParts(const std::vector<UnstructuredMesh::Pointer>& parts) {
    if (parts.empty()) return nullptr;

    auto out = UnstructuredMesh::New();
    auto outPts = Points::New();
    auto outCells = CellArray::New();
    auto outTypes = UnsignedIntArray::New();

    igIndex ptOffset = 0;
    for (auto& part : parts) {
        if (!part) continue;
        auto pts = part->GetPoints();
        if (pts) {
            for (igIndex i = 0; i < pts->GetNumberOfPoints(); ++i) {
                outPts->AddPoint(pts->GetPoint(i));
            }
        }
        auto cells = part->GetCells();
        auto types = part->GetCellTypes();
        const igIndex* ids = nullptr;
        for (igIndex c = 0; cells && c < part->GetNumberOfCells(); ++c) {
            igIndex n = cells->GetCellIds(c, ids);
            std::vector<igIndex> tmp(ids, ids + n);
            for (auto& v : tmp) { v += ptOffset; }
            outCells->AddCellIds(tmp.data(), (int)n);
            outTypes->AddValue(types ? types->GetValue(c) : (igIndex)IG_TETRA);
        }
        if (pts) { ptOffset += pts->GetNumberOfPoints(); }
    }
    out->SetPoints(outPts);
    out->SetCells(outCells, outTypes);

    // 属性: 以第一个结果的布局为准, 依次拼接各分块的值
    auto srcData = parts[0] ? parts[0]->GetAttributeSet() : nullptr;
    auto outData = AttributeSet::New();
    if (srcData) {
        auto list0 = srcData->GetAllAttributes();
        for (igIndex ai = 0; list0 && ai < list0->GetNumberOfElements(); ++ai) {
            auto& e0 = list0->GetElement(ai);
            if (!e0.pointer) continue;

            int dim = e0.pointer->GetDimension();
            if (dim <= 0) { dim = 1; }
            std::vector<double> vals(dim, 0.0);

            igIndex total = 0;
            for (auto& p : parts) {
                auto s = p ? p->GetAttributeSet() : nullptr;
                auto a = s ? s->GetAllAttributes() : nullptr;
                if (a && ai < a->GetNumberOfElements() && a->GetElement(ai).pointer) {
                    total += a->GetElement(ai).pointer->GetNumberOfElements();
                }
            }

            auto merged = CreateSameTypeArray(e0.pointer);
            merged->Resize(total);
            igIndex w = 0;
            for (auto& p : parts) {
                auto s = p ? p->GetAttributeSet() : nullptr;
                auto a = s ? s->GetAllAttributes() : nullptr;
                if (!a || ai >= a->GetNumberOfElements()) continue;
                auto arr = a->GetElement(ai).pointer;
                if (!arr) continue;
                for (igIndex k = 0; k < arr->GetNumberOfElements(); ++k) {
                    arr->GetElement(k, vals.data());
                    merged->SetElement(w++, vals.data());
                }
            }
            outData->AddAttribute(e0.type, e0.attachmentType, merged, e0.GetDataRange());
        }
    }
    out->SetAttributeSet(outData);
    out->SetName("isovolume_merged");
    return out;
}

// 表驱动的六面体单阈值裁剪: 查自研 case 表 → 直接输出原生单元(tet/pyramid/wedge/hex)
// keepAbove=true 时"保留侧"为 values[i] >= 0; false 时为 values[i] <= 0
void IsoVolumeFilter::ClipCellByTable(Cell::Pointer cell, int cellType, const double* values, bool keepAbove,
                                      Points::Pointer points, CellArray::Pointer connectivity,
                                      UnsignedIntArray::Pointer types, igIndex cellId,
                                      std::vector<CellClip::InterpolateEdge>& OriginEdge,
                                      std::vector<igIndex>& originCell,
                                      std::vector<InteriorInterp>& interiorPts) {
    if (!cell || !values || !points || !connectivity || !types) { return; }

    // 按单元类型选择"case 表 + 顶点数 + 边表"
    static const int kTetEdges[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    static const int kWedgeEdges[9][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 4}, {2, 5}, {3, 4}, {3, 5}, {4, 5}};
    static const int kPyramidEdges[8][2] = {{0, 1}, {0, 3}, {0, 4}, {1, 2}, {1, 4}, {2, 3}, {2, 4}, {3, 4}};
    static const int kHexEdges[12][2] = {{0, 1}, {0, 3}, {0, 4}, {1, 2}, {1, 5}, {2, 3},
                                         {2, 6}, {3, 7}, {4, 5}, {4, 7}, {5, 6}, {6, 7}};
    const int* caseStart = nullptr;
    const int* cellData = nullptr;          // 每项 10 个 int: {type, n, src[8]}
    // 保留 "value <= iso" 一侧时使用的表(反向侧表)
    const int* caseStartIn = nullptr;
    const int* cellDataIn = nullptr;
    int nVerts = 0, nEdges = 0;
    const int (*edgeVerts)[2] = nullptr;
    switch (cellType) {
        case IG_TETRA:
            nVerts = 4; nEdges = 6; edgeVerts = kTetEdges;
            caseStart = kIsoTetCaseStart;
            cellData = reinterpret_cast<const int*>(kIsoTetCells);
            caseStartIn = kIsoTetCaseStartIn;
            cellDataIn = reinterpret_cast<const int*>(kIsoTetCellsIn);
            break;
        case IG_PRISM:
            nVerts = 6; nEdges = 9; edgeVerts = kWedgeEdges;
            caseStart = kIsoWedgeCaseStart;
            cellData = reinterpret_cast<const int*>(kIsoWedgeCells);
            caseStartIn = kIsoWedgeCaseStartIn;
            cellDataIn = reinterpret_cast<const int*>(kIsoWedgeCellsIn);
            break;
        case IG_PYRAMID:
            nVerts = 5; nEdges = 8; edgeVerts = kPyramidEdges;
            caseStart = kIsoPyramidCaseStart;
            cellData = reinterpret_cast<const int*>(kIsoPyramidCells);
            caseStartIn = kIsoPyramidCaseStartIn;
            cellDataIn = reinterpret_cast<const int*>(kIsoPyramidCellsIn);
            break;
        case IG_HEXAHEDRON:
            nVerts = 8; nEdges = 12; edgeVerts = kHexEdges;
            caseStart = kIsoHexCaseStart;
            cellData = reinterpret_cast<const int*>(kIsoHexCells);
            caseStartIn = kIsoHexCaseStartIn;
            cellDataIn = reinterpret_cast<const int*>(kIsoHexCellsIn);
            break;
        default:
            return;
    }
    // 心点定义表(按 case 索引) —— 心点 = 指定点子集的等权平均
    const int* centStart = nullptr;
    const int* centSrc = nullptr;
    // 心点表长度, 用于 case 索引的上界检查(避免越界读 centStart[mask + 1])
    int centStartSize = 0;
    // 反向侧的心点定义(两侧的 mask 集合与子集都可能不同)
    const int* centStartIn = nullptr;
    const int* centSrcIn = nullptr;
    int centStartSizeIn = 0;
    switch (cellType) {
        case IG_PRISM:
            centStart = kIsoWedgeCentroidStart; centSrc = kIsoWedgeCentroidSrc;
            centStartSize = static_cast<int>(sizeof(kIsoWedgeCentroidStart) / sizeof(kIsoWedgeCentroidStart[0]));
            centStartIn = kIsoWedgeCentroidStartIn; centSrcIn = kIsoWedgeCentroidSrcIn;
            centStartSizeIn = static_cast<int>(sizeof(kIsoWedgeCentroidStartIn) / sizeof(kIsoWedgeCentroidStartIn[0]));
            break;
        case IG_PYRAMID:
            centStart = kIsoPyramidCentroidStart; centSrc = kIsoPyramidCentroidSrc;
            centStartSize = static_cast<int>(sizeof(kIsoPyramidCentroidStart) / sizeof(kIsoPyramidCentroidStart[0]));
            centStartIn = kIsoPyramidCentroidStartIn; centSrcIn = kIsoPyramidCentroidSrcIn;
            centStartSizeIn = static_cast<int>(sizeof(kIsoPyramidCentroidStartIn) / sizeof(kIsoPyramidCentroidStartIn[0]));
            break;
        case IG_HEXAHEDRON:
            centStart = kIsoHexCentroidStart; centSrc = kIsoHexCentroidSrc;
            centStartSize = static_cast<int>(sizeof(kIsoHexCentroidStart) / sizeof(kIsoHexCentroidStart[0]));
            centStartIn = kIsoHexCentroidStartIn; centSrcIn = kIsoHexCentroidSrcIn;
            centStartSizeIn = static_cast<int>(sizeof(kIsoHexCentroidStartIn) / sizeof(kIsoHexCentroidStartIn[0]));
            break;
        default: break;   // 四面体无内部点
    }
    // 保留 "value <= iso" 一侧时改用反向侧表。
    // 不能靠取反索引复用正向表: 两趟的保留侧互为补集, 但同一掩码下两者的
    // 输出分解并不互为镜像, 取反索引会在若干 case 上得到错误分解(少输出单元)。
    if (!keepAbove) {
        if (caseStartIn) { caseStart = caseStartIn; }
        if (cellDataIn) { cellData = cellDataIn; }
        centStart = centStartIn; centSrc = centSrcIn; centStartSize = centStartSizeIn;
    }
    // 1) 顶点位掩码: 位 i = (标量值 >= 等值), 与保留方向无关。
    //    传入的 values 已按方向处理: keepAbove=true 时为 iso - s, 否则为 s - iso,
    //    两种情形下上式都等价于 s >= iso。
    int mask = 0;
    for (int i = 0; i < nVerts; ++i) {
        const bool above = keepAbove ? (values[i] <= 0.0) : (values[i] >= 0.0);
        if (above) { mask |= (1 << i); }
    }
    if (mask == 0) { return; }                 // 全部在保留侧之外 → 无输出

    const int start = caseStart[mask];
    const int end = caseStart[mask + 1];
    if (start >= end) { return; }

    // 类型码(表: 4/5/6/8 = 顶点数) → iGameVis 单元类型
    auto typeOf = [](int code) -> igIndex {
        switch (code) {
            case 4:  return IG_TETRA;
            case 5:  return IG_PYRAMID;
            case 6:  return IG_PRISM;          // 楔形/棱柱
            case 8:  return IG_HEXAHEDRON;
            default: return IG_NONE;
        }
    };

    std::vector<igIndex> edgePointId(static_cast<size_t>(nEdges), -1);

    // ===== 预扫描: 若本 case 含"内部点", 先用本 case 的边界点平均算出其位置 =====
    // 说明: 心扇分解需要一个片段内部的顶点; 用边界点的平均可保证它落在片段内部,
    //       且不影响外表面。
    bool hasInterior = false;
    std::vector<char> useVertex(static_cast<size_t>(nVerts), 0);
    std::vector<char> useEdge(static_cast<size_t>(nEdges), 0);
    int maxInteriorId = -1;
    for (int c = start; c < end; ++c) {
        const int* cs = cellData + c * 10;
        const int n = cs[1];
        for (int k = 0; k < n; ++k) {
            const int src = cs[2 + k];
            if (src < 8) { if (src < nVerts) { useVertex[src] = 1; } }
            else if (src < 20) { const int e = src - 8; if (e >= 0 && e < nEdges) { useEdge[e] = 1; } }
            else { hasInterior = true; if (src - 20 > maxInteriorId) { maxInteriorId = src - 20; } }
        }
    }
    std::vector<igIndex> interiorPointId(static_cast<size_t>(maxInteriorId + 1), -1);
    if (hasInterior) {
        InteriorInterp rec;
        double acc[3] = {0.0, 0.0, 0.0};
        int cnt = 0;
        for (int i = 0; i < nVerts; ++i) {
            if (!useVertex[i]) { continue; }
            auto p = cell->GetPoint(i);
            acc[0] += p[0]; acc[1] += p[1]; acc[2] += p[2];
            ++cnt;
            rec.v.push_back(cell->GetPointId(i));
            rec.w.push_back(1.0);                       // 先记"计数", 之后再除
        }
        for (int e = 0; e < nEdges; ++e) {
            if (!useEdge[e]) { continue; }
            int vh1 = edgeVerts[e][0];
            int vh2 = edgeVerts[e][1];
            double delta = values[vh2] - values[vh1];
            if (delta <= 0.0) { int tmp = vh1; vh1 = vh2; vh2 = tmp; delta = -delta; }
            const double t = (delta == 0.0 ? 0.0 : -values[vh1] / delta);
            auto p1 = cell->GetPoint(vh1);
            auto p2 = cell->GetPoint(vh2);
            auto pe = p1 + (p2 - p1) * t;
            acc[0] += pe[0]; acc[1] += pe[1]; acc[2] += pe[2];
            ++cnt;
            rec.v.push_back(cell->GetPointId(vh1));
            rec.v.push_back(cell->GetPointId(vh2));
            rec.w.push_back(1.0 - t);                   // 边点 = (1-t)*vh1 + t*vh2
            rec.w.push_back(t);
        }
        if (cnt > 0) {
            auto pi = cell->GetPoint(0) * 0.0;          // 零向量
            // 界内检查: 必须先确认 mask + 1 在表内, 否则 centStart[mask + 1] 会越界读
            const bool centIdxOk = (centStart != nullptr) && (centSrc != nullptr) &&
                                   (mask >= 0) && (mask + 1 < centStartSize);
            if (centIdxOk && centStart[mask + 1] > centStart[mask]) {
                // === 心点 = 指定点子集的等权平均(属性同样 1/n 加权) ===
                const int a0 = centStart[mask];
                const int b0 = centStart[mask + 1];
                const int n0 = b0 - a0;
                const double w0 = 1.0 / static_cast<double>(n0);
                pi = cell->GetPoint(0) * 0.0;
                rec.v.clear(); rec.w.clear();
                for (int k = a0; k < b0; ++k) {
                    const int s = centSrc[k];
                    if (s < 8) {
                        if (s >= nVerts) { continue; }
                        pi = pi + cell->GetPoint(s) * w0;
                        rec.v.push_back(cell->GetPointId(s));
                        rec.w.push_back(w0);
                    } else if (s < 20) {
                        const int e = s - 8;
                        if (e < 0 || e >= nEdges) { continue; }
                        int vh1 = edgeVerts[e][0], vh2 = edgeVerts[e][1];
                        double delta = values[vh2] - values[vh1];
                        if (delta <= 0.0) { int t = vh1; vh1 = vh2; vh2 = t; delta = -delta; }
                        double t = (delta == 0.0 ? 0.0 : -values[vh1] / delta);
                        if (t < 0.0) { t = 0.0; } else if (t > 1.0) { t = 1.0; }   // 安全钳位
                        auto p1 = cell->GetPoint(vh1), p2 = cell->GetPoint(vh2);
                        auto pe = p1 + (p2 - p1) * t;
                        pi = pi + pe * w0;
                        rec.v.push_back(cell->GetPointId(vh1)); rec.w.push_back((1.0 - t) * w0);
                        rec.v.push_back(cell->GetPointId(vh2)); rec.w.push_back(t * w0);
                    }
                };
            } else {
                pi = pi + Vector<float, 3>(static_cast<float>(acc[0] / cnt),
                                           static_cast<float>(acc[1] / cnt),
                                           static_cast<float>(acc[2] / cnt));
                for (size_t m = 0; m < rec.w.size(); ++m) { rec.w[m] /= static_cast<double>(cnt); }
            }
            points->AddPoint(pi);
            OriginEdge.emplace_back(CellClip::InterpolateEdge(cell->GetPointId(0)));
            const igIndex pid = static_cast<igIndex>(points->GetNumberOfPoints() - 1);
            rec.pointId = pid;
            interiorPts.push_back(rec);
            for (size_t iid = 0; iid < interiorPointId.size(); ++iid) { interiorPointId[iid] = pid; }
        }
    }

    igIndex cellIds[8];
    for (int c = start; c < end; ++c) {
        const int* cs = cellData + c * 10;
        const int n = cs[1];
        if (n < 3 || n > 8) { continue; }
        bool ok = true;
        for (int k = 0; k < n; ++k) {
            const int src = cs[2 + k];
            if (src < 8) {
                // 原顶点: 直接复用(与 CellClip 相同的记录方式)
                if (src >= nVerts) { ok = false; break; }
                points->AddPoint(cell->GetPoint(src));
                OriginEdge.emplace_back(CellClip::InterpolateEdge(cell->GetPointId(src)));
                cellIds[k] = static_cast<igIndex>(points->GetNumberOfPoints() - 1);
            } else if (src < 20) {
                const int e = src - 8;
                if (e < 0 || e >= nEdges) { ok = false; break; }
                if (edgePointId[e] < 0) {
                    int vh1 = edgeVerts[e][0];
                    int vh2 = edgeVerts[e][1];
                    double delta = values[vh2] - values[vh1];
                    if (delta <= 0.0) {
                        int tmp = vh1; vh1 = vh2; vh2 = tmp; delta = -delta;
                    }
                    const double t = (delta == 0.0 ? 0.0 : -values[vh1] / delta);
                    auto p1 = cell->GetPoint(vh1);
                    auto p2 = cell->GetPoint(vh2);
                    points->AddPoint(p1 + (p2 - p1) * t);
                    OriginEdge.emplace_back(CellClip::InterpolateEdge(
                            cell->GetPointId(vh1), cell->GetPointId(vh2), t));
                    edgePointId[e] = static_cast<igIndex>(points->GetNumberOfPoints() - 1);
                }
                cellIds[k] = edgePointId[e];
            } else {
                // 单元内部点(已在预扫描中创建)
                const int iid = src - 20;
                if (iid < 0 || iid >= static_cast<int>(interiorPointId.size()) ||
                    interiorPointId[iid] < 0) {
                    ok = false; break;
                }
                cellIds[k] = interiorPointId[iid];
            }
        }
        if (!ok) { continue; }                      // 表项异常 → 跳过, 不输出坏单元
        connectivity->AddCellIds(cellIds, n);
        types->AddValue(typeOf(cs[0]));
        originCell.emplace_back(cellId);
    }
}

bool IsoVolumeFilter::ExecuteWithVolumeMesh(VolumeMesh::Pointer vm) {
    if (!vm) return false;
    if (vm->GetIsPolyhedronType()) {
        return this->ExecuteWithVolumeMeshWithPolyhedronType(vm);
    }
    auto um = UnstructuredMesh::New();
    um->GenerateFromVolumeMesh(vm);
    return this->ExecuteWithUnstructuredMesh(um);
}

bool IsoVolumeFilter::ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm) {
    if (!sm) return false;
    auto um = UnstructuredMesh::New();
    um->GenerateFromSurfaceMesh(sm);
    return this->ExecuteWithUnstructuredMesh(um);
}

bool IsoVolumeFilter::ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm) {
    if (!vm || !vm->GetIsPolyhedronType()) { return false; }
    auto um = UnstructuredMesh::New();
    um->GenerateFromVolumeMesh(vm);
    return this->ExecuteWithUnstructuredMesh(um);
}

bool IsoVolumeFilter::ClipMeshByScalar(UnstructuredMesh::Pointer input, ArrayObject::Pointer scalarArray,
                                       double isoValue, bool keepAbove, UnstructuredMesh::Pointer output) {
    if (!input || !output || !scalarArray) return false;

    AttributeSet::Pointer inData = input->GetAttributeSet();
    AttributeSet::Pointer outData = AttributeSet::New();

    CellArray::Pointer OutConn = CellArray::New();
    UnsignedIntArray::Pointer OutType = UnsignedIntArray::New();
    Points::Pointer OutPoints = Points::New();
    std::vector<CellClip::InterpolateEdge> OriginEdge;
    std::vector<InteriorInterp> InteriorPts;   // 六面体内部点(三线性插值)记录
    std::vector<igIndex> OriginCell;

    auto inPoints = input->GetPoints();
    auto inPointNum = input->GetNumberOfPoints();
    auto inCells = input->GetCells();
    auto inTypes = input->GetCellTypes();
    igIndex inCellNum = input->GetNumberOfCells();

    DoubleArray::Pointer PointIsoArray = DoubleArray::New();
    CharArray::Pointer CellVisible = CharArray::New();
    ComputePointValueAndCellVisible(inPoints, inCells, PointIsoArray, CellVisible, scalarArray, isoValue, keepAbove);
    auto PointIsoValue = PointIsoArray->RawPointer();
    auto cellVisible = CellVisible->RawPointer();

    // 复制完全在内的单元
    igIndex vcnt = 0;
    const igIndex* vhs = nullptr;
    for (igIndex cellId = 0; cellId < inCellNum; cellId++) {
        if (cellVisible[cellId] == 1) {
            vcnt = static_cast<igIndex>(inCells->GetCellIds(cellId, vhs));
            OutConn->AddCellIds(vhs, vcnt);
            OutType->AddValue(inTypes->GetValue(cellId));
            OriginCell.emplace_back(cellId);
        }
    }

    // 复制原始点，并建立 OriginEdge 映射
    OutPoints->Resize(inPointNum);
    std::copy(inPoints->RawPointer(), inPoints->RawPointer() + inPointNum * 3, OutPoints->RawPointer());
    OriginEdge.reserve(inPointNum * 2);
    for (int pointId = 0; pointId < inPointNum; pointId++) {
        OriginEdge.emplace_back(CellClip::InterpolateEdge(pointId));
    }

    // 裁剪与边界相交的单元
    igIndex* vhs2 = nullptr;
    igIndex CellId = 0;
    igIndex i = 0;
    Cell::Pointer cell = nullptr;
    std::vector<double> CellIsoValue;
    for (CellId = 0; CellId < inCellNum; CellId++) {
        if (cellVisible[CellId]) { continue; }
        cell = input->GetCell(CellId);
        vhs2 = cell->m_PointIds->RawPointer();
        vcnt = cell->GetNumberOfPoints();
        if (CellIsoValue.size() < (size_t)vcnt) { CellIsoValue.resize(vcnt); }
        for (i = 0; i < vcnt; i++) { CellIsoValue[i] = PointIsoValue[vhs2[i]]; }
        switch (cell->GetCellType()) {
            case IG_TRIANGLE:
                CellClip::Clip(DynamicCast<Triangle>(cell), CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            case IG_QUAD:
                CellClip::Clip(DynamicCast<Quad>(cell), CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            case IG_POLYGON:
                CellClip::Clip(DynamicCast<Polygon>(cell), CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr, nullptr,
                               CellId, OriginEdge, OriginCell);
                break;
            case IG_TETRA:
            case IG_PRISM:
            case IG_PYRAMID:
            case IG_HEXAHEDRON:
                // 表驱动原生裁剪: 直接输出原生单元(tet/wedge/pyramid/hex)，不再四面体化
                this->ClipCellByTable(cell, cell->GetCellType(), CellIsoValue.data(), keepAbove,
                                      OutPoints, OutConn, OutType, CellId,
                                      OriginEdge, OriginCell, InteriorPts);
                break;
            case IG_QUADRATIC_TETRA:
                CellClip::Clip(DynamicCast<QuadraticTetra>(cell), CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            case IG_POLYHEDRON:
                CellClip::Clip(DynamicCast<Polyhedron>(cell), CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr,
                               nullptr, CellId, OriginEdge, OriginCell);
                break;
            default: {
                if (Cell::GetCellDimension(cell->GetCellType()) == 3) {
                    auto vol = DynamicCast<Volume>(cell);
                    // Quadratic / Lagrange 体单元并不是 Volume 子类，DynamicCast 可能为 nullptr，
                    // 此时无法裁剪且会崩溃，选择跳过该相交单元以保证健壮性。
                    if (vol) {
                        CellClip::Clip(vol, CellIsoValue.data(), OutPoints, OutConn, OutType, nullptr,
                                       nullptr, CellId, OriginEdge, OriginCell, PointIsoValue);
                    }
                }
                break;
            }
        }
    }

    this->CopyAttributeSetData(OutPoints->GetNumberOfPoints(), OutConn->GetNumberOfCells(), inData, outData,
                               OriginEdge, OriginCell, InteriorPts);

    // ===== 点合并 + 紧凑化：消除裁剪边界的重复点=====
    {
        igIndex outP = OutPoints->GetNumberOfPoints();
        igIndex outC = OutConn->GetNumberOfCells();

        // 1) 按坐标去重（重复点坐标精确相等）
        //    这里点数可达数十万, 用 std::map<tuple<float,float,float>> 会退化成
        //    红黑树 + 元组逐项比较, 实测是最大热点; 改为哈希表 + 按位哈希(保持"精确相等"语义)。
        struct CoordKey {
            float x, y, z;
            bool operator==(const CoordKey& o) const { return x == o.x && y == o.y && z == o.z; }
        };
        struct CoordHash {
            size_t operator()(const CoordKey& k) const noexcept {
                uint32_t bx, by, bz;
                // -0.0 与 +0.0 在位模式上不同但浮点相等, 先归一化再取位
                const float x = k.x + 0.0f, y = k.y + 0.0f, z = k.z + 0.0f;
                std::memcpy(&bx, &x, sizeof(bx));
                std::memcpy(&by, &y, sizeof(by));
                std::memcpy(&bz, &z, sizeof(bz));
                uint64_t h = 1469598103934665603ull;
                h = (h ^ bx) * 1099511628211ull;
                h = (h ^ by) * 1099511628211ull;
                h = (h ^ bz) * 1099511628211ull;
                return static_cast<size_t>(h);
            }
        };
        std::unordered_map<CoordKey, igIndex, CoordHash> coordMap;
        coordMap.reserve(static_cast<size_t>(outP) + 1);   // 预留, 避免反复 rehash
        std::vector<igIndex> oldToNew(outP);
        igIndex preCnt = 0;
        float* rawPts = OutPoints->RawPointer();
        CoordKey key;
        for (igIndex old = 0; old < outP; ++old) {
            key.x = rawPts[old * 3];
            key.y = rawPts[old * 3 + 1];
            key.z = rawPts[old * 3 + 2];
            auto it = coordMap.find(key);
            if (it == coordMap.end()) {
                coordMap.emplace(key, old);
                oldToNew[old] = preCnt++;
            } else {
                oldToNew[old] = oldToNew[it->second];
            }
        }
        coordMap.clear();

        // 2) 重映射连接关系，并仅保留被单元引用的新点（紧凑化）
        //    直接写入扁平缓冲, 避免"每个单元一个 vector"(数十万次小分配)。
        std::vector<igIndex> preToCompact(preCnt, -1);
        std::vector<igIndex> repOfPre(preCnt, -1);   // 每个新点对应的代表输出点索引
        igIndex compactCnt = 0;
        std::vector<igIndex> flatConn;
        flatConn.reserve(static_cast<size_t>(outC) * 4 + 64);
        std::vector<igIndex> cellSizes;
        cellSizes.reserve(static_cast<size_t>(outC));
        const igIndex* cellIds = nullptr;
        for (igIndex c = 0; c < outC; ++c) {
            igIndex vcnt = OutConn->GetCellIds(c, cellIds);
            cellSizes.push_back(vcnt);
            for (igIndex k = 0; k < vcnt; ++k) {
                igIndex pre = oldToNew[cellIds[k]];
                if (preToCompact[pre] == -1) {
                    preToCompact[pre] = compactCnt++;
                    repOfPre[pre] = cellIds[k];
                }
                flatConn.push_back(preToCompact[pre]);
            }
        }

        // 3) 重建紧凑点集
        Points::Pointer newPts = Points::New();
        newPts->Resize(compactCnt);
        float* np = newPts->RawPointer();
        for (igIndex p = 0; p < preCnt; ++p) {
            igIndex rep = repOfPre[p];
            if (rep < 0) { continue; }
            igIndex ci = preToCompact[p];
            np[ci * 3]     = rawPts[rep * 3];
            np[ci * 3 + 1] = rawPts[rep * 3 + 1];
            np[ci * 3 + 2] = rawPts[rep * 3 + 2];
        }

        // 4) 重建紧凑连接关系(从扁平缓冲切分回每个单元)
        CellArray::Pointer finalConn = CellArray::New();
        {
            size_t off = 0;
            for (igIndex c = 0; c < outC; ++c) {
                const igIndex n = cellSizes[c];
                finalConn->AddCellIds(flatConn.data() + off, static_cast<int>(n));
                off += static_cast<size_t>(n);
            }
        }

        // 5) 重建点属性（取代表点的属性）；单元属性不变
        AttributeSet::Pointer newOutData = AttributeSet::New();
        auto outAllAttr = outData->GetAllAttributes();
        if (outAllAttr) {
            for (igIndex i = 0; i < outAllAttr->GetNumberOfElements(); ++i) {
                auto attr = outAllAttr->GetElement(i);
                auto inArray = attr.pointer;
                if (!inArray) { continue; }
                if (attr.attachmentType == IG_POINT) {
                    auto newArr = CreateSameTypeArray(inArray);
                    newArr->Resize(compactCnt);
                    double vals[IGAME_CELL_MAX_SIZE] = {0};
                    for (igIndex p = 0; p < preCnt; ++p) {
                        igIndex rep = repOfPre[p];
                        if (rep < 0) { continue; }
                        igIndex ci = preToCompact[p];
                        inArray->GetElement(rep, vals);
                        newArr->SetElement(ci, vals);
                    }
                    newOutData->AddAttribute(attr.type, attr.attachmentType, newArr, attr.GetDataRange());
                } else if (attr.attachmentType == IG_CELL) {
                    newOutData->AddAttribute(attr.type, attr.attachmentType, inArray, attr.GetDataRange());
                }
            }
        }

        OutPoints = newPts;
        OutConn = finalConn;
        outData = newOutData;
    }

    output->SetCells(OutConn, OutType);
    output->SetPoints(OutPoints);
    output->SetAttributeSet(outData);

    std::vector<igIndex>().swap(OriginCell);
    std::vector<CellClip::InterpolateEdge>().swap(OriginEdge);
    return true;
}

void IsoVolumeFilter::ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells,
                                                      DoubleArray::Pointer PointIsoArray, CharArray::Pointer CellVisible,
                                                      ArrayObject::Pointer scalarArray, double isoValue,
                                                      bool keepAbove) {
    igIndex PointId = 0;
    igIndex inPointNum = inPoints->GetNumberOfPoints();
    PointIsoArray->Resize(inPointNum);
    double* PointIsoValue = PointIsoArray->RawPointer();

    int dim = static_cast<int>(m_SelectDimension);
    for (PointId = 0; PointId < inPointNum; PointId++) {
        double s = scalarArray->GetElementValue(PointId, dim);
        if (keepAbove) {
            // s >= isoValue 等价于 isoValue - s <= 0
            PointIsoValue[PointId] = isoValue - s;
        } else {
            // s <= isoValue 等价于 s - isoValue <= 0
            PointIsoValue[PointId] = s - isoValue;
        }
    }

    igIndex CellId = 0;
    IGsize CellNum = inCells->GetNumberOfCells();
    CellVisible->Resize(CellNum);
    auto cellVisible = CellVisible->RawPointer();
    std::fill(cellVisible, cellVisible + CellNum, 0);

    auto func = [&](igIndex start, igIndex end) -> void {
        igIndex cellId = 0;
        // 多面体的 id 列表(打包面结构)可能超过 IGAME_CELL_MAX_SIZE, 用指针版避免栈溢出
        const igIndex* vhs = nullptr;
        igIndex vcnt = 0;
        igIndex allIn = 1, allOut = 1;
        double value = 0;
        igIndex i = 0;
        for (cellId = start; cellId < end; cellId++) {
            vcnt = inCells->GetCellIds(cellId, vhs);
            allIn = 1;
            allOut = 1;
            for (i = 0; i < vcnt; i++) {
                value = PointIsoValue[vhs[i]];
                if (value < 0.0) {
                    allOut = 0;
                } else if (value > 0.0) {
                    allIn = 0;
                } else {
                    allIn = 0;
                    allOut = 0;
                }
            }
            if (allIn) {
                cellVisible[cellId] = 1;
            } else if (allOut) {
                cellVisible[cellId] = 2;
            }
        }
    };
    ThreadPool::parallelFor(0, CellNum, func);
    PointIsoValue = nullptr;
}

void IsoVolumeFilter::CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                                           AttributeSet::Pointer outData,
                                           std::vector<CellClip::InterpolateEdge> OriginEdge,
                                           std::vector<igIndex> OriginCell,
                                           const std::vector<InteriorInterp>& interiorPts) {
    igIndex i = 0, j = 0, k = 0;
    int dimension = 0;
    if (!inData) return;
    auto inAllAttr = inData->GetAllAttributes();
    if (!inAllAttr) return;
    // 内部点查找(interiorPts 按 pointId 升序)
    auto findInterior = [&interiorPts](igIndex pid) -> const InteriorInterp* {
        if (interiorPts.empty()) { return nullptr; }
        size_t lo = 0, hi = interiorPts.size();
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            if (interiorPts[mid].pointId < pid) { lo = mid + 1; } else { hi = mid; }
        }
        if (lo < interiorPts.size() && interiorPts[lo].pointId == pid) { return &interiorPts[lo]; }
        return nullptr;
    };
    double values[IGAME_CELL_MAX_SIZE] = {0};
    double values_1[IGAME_CELL_MAX_SIZE] = {0};
    double values_2[IGAME_CELL_MAX_SIZE] = {0};
    for (i = 0; i < inAllAttr->GetNumberOfElements(); i++) {
        auto attr = inAllAttr->GetElement(i);
        auto inArray = attr.pointer;
        if (!inArray) continue;
        auto outArray = CreateSameTypeArray(inArray);
        if (attr.attachmentType == IG_CELL) {
            outArray->Resize(outCellNum);
            for (j = 0; j < outCellNum; j++) {
                inArray->GetElement(OriginCell[j], values);
                outArray->SetElement(j, values);
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        } else if (attr.attachmentType == IG_POINT) {
            outArray->Resize(outPointNum);
            dimension = inArray->GetDimension();
            for (j = 0; j < outPointNum; j++) {
                const InteriorInterp* ip = findInterior(j);
                if (ip) {
                    // 单元内部点: 按权重对参与的边界点加权
                    for (k = 0; k < dimension; k++) { values[k] = 0.0; }
                    for (size_t m = 0; m < ip->v.size(); ++m) {
                        inArray->GetElement(ip->v[m], values_1);
                        const double wm = ip->w[m];
                        for (k = 0; k < dimension; k++) { values[k] += wm * values_1[k]; }
                    }
                    outArray->SetElement(j, values);
                    continue;
                }
                inArray->GetElement(OriginEdge[j].vh1, values_1);
                if (OriginEdge[j].vh2 == -1) {
                    outArray->SetElement(j, values_1);
                } else {
                    inArray->GetElement(OriginEdge[j].vh2, values_2);
                    for (k = 0; k < dimension; k++) {
                        values[k] = values_1[k] + OriginEdge[j].t * (values_2[k] - values_1[k]);
                    }
                    outArray->SetElement(j, values);
                }
            }
            outData->AddAttribute(attr.type, attr.attachmentType, outArray, attr.GetDataRange());
        }
    }
}
IGAME_NAMESPACE_END
