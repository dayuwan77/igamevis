#include "iGameConnectivityFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

IGAME_NAMESPACE_BEGIN

namespace {

// 按输入数组的底层类型创建输出数组，避免属性复制时被统一转成 float。
ArrayObject::Pointer CreateArrayOfSameType(ArrayObject* inArray) {
    switch (inArray->GetArrayType()) {
        case IG_DoubleArray: return DoubleArray::New();
        case IG_IntArray:
        case IG_INTARRAY: return IntArray::New();
        case IG_UnsignedIntArray: return UnsignedIntArray::New();
        case IG_CharArray: return CharArray::New();
        case IG_UnsignedCharArray: return UnsignedCharArray::New();
        case IG_ShortArray: return ShortArray::New();
        case IG_UnsignedShortArray: return UnsignedShortArray::New();
        case IG_LongLongArray: return LongLongArray::New();
        case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
        case IG_FloatArray:
        default: return FloatArray::New();
    }
}

} // namespace

ConnectivityFilter::ConnectivityFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ConnectivityFilter::BuildPointFaceAdjacency() {
    const IGsize nPts = m_Mesh->GetNumberOfPoints();
    const IGsize nCells = m_Mesh->GetNumberOfFaces();
    auto faces = m_Mesh->GetFaces();

    std::vector<int> counts(nPts, 0);
    const igIndex* ids = nullptr;
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        const int n = faces->GetCellIds(cellId, ids);
        for (int j = 0; j < n; ++j) { ++counts[ids[j]]; }
    }

    m_PointFaces.assign(nPts, {});
    for (IGsize p = 0; p < nPts; ++p) { m_PointFaces[p].reserve(counts[p]); }
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        const int n = faces->GetCellIds(cellId, ids);
        for (int j = 0; j < n; ++j) { m_PointFaces[ids[j]].push_back(cellId); }
    }
}

bool ConnectivityFilter::CellIsConnected(igIndex cellId) const {
    if (!m_ScalarConnectivity || m_ScalarArray.IsNull()) { return true; }
    const igIndex* ids = nullptr;
    const int n = m_Mesh->GetFaces()->GetCellIds(cellId, ids);
    if (n <= 0) { return false; }

    double lo = std::numeric_limits<double>::max();
    double hi = -std::numeric_limits<double>::max();
    for (int j = 0; j < n; ++j) {
        const double v = m_ScalarArray->GetElementValue(ids[j], 0);
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    const double r0 = std::min(m_ScalarRange[0], m_ScalarRange[1]);
    const double r1 = std::max(m_ScalarRange[0], m_ScalarRange[1]);
    if (m_FullScalarConnectivity) { return lo >= r0 && hi <= r1; }
    return hi >= r0 && lo <= r1;
}

bool ConnectivityFilter::ResolveScalarArray() {
    auto attrSet = m_Mesh->GetAttributeSet();
    if (attrSet == nullptr) {
        m_Message = "ScalarConnectivity requires a point scalar array";
        return false;
    }
    if (!m_ScalarArrayName.empty()) {
        const int idx = attrSet->GetAttributeIndex(m_ScalarArrayName);
        if (idx < 0) {
            m_Message = "ScalarConnectivity: array not found: " + m_ScalarArrayName;
            return false;
        }
        auto& attr = attrSet->GetAttribute(idx);
        if (attr.attachmentType != IG_POINT || attr.pointer.IsNull()) {
            m_Message = "ScalarConnectivity: array is not a point attribute";
            return false;
        }
        m_ScalarArray = attr.pointer;
        return true;
    }

    auto all = attrSet->GetAllAttributes();
    if (all) {
        const IGsize n = all->GetNumberOfElements();
        // 优先使用当前显示属性（若为点属性）。
        const int cur = m_Mesh->GetAttributeIndex();
        if (cur >= 0 && cur < static_cast<int>(n)) {
            auto& attr = all->GetElement(cur);
            if (!attr.isDeleted && attr.attachmentType == IG_POINT && !attr.pointer.IsNull()) {
                m_ScalarArray = attr.pointer;
                return true;
            }
        }
        for (IGsize i = 0; i < n; ++i) {
            auto& attr = all->GetElement(i);
            if (!attr.isDeleted && attr.attachmentType == IG_POINT && !attr.pointer.IsNull()) {
                m_ScalarArray = attr.pointer;
                return true;
            }
        }
    }
    m_Message = "ScalarConnectivity: no point attribute available";
    return false;
}

IGsize ConnectivityFilter::TraverseAndMark(const std::vector<igIndex>& seedCells, int regionNumber,
                                           igIndex& pointNumber) {
    IGsize count = 0;
    std::vector<igIndex> wave = seedCells;
    std::vector<igIndex> wave2;
    const igIndex* ids = nullptr;

    while (!wave.empty()) {
        wave2.clear();
        for (igIndex cellId : wave) {
            if (m_Visited[cellId] >= 0) { continue; }
            m_Visited[cellId] = regionNumber;
            ++count;

            const int n = m_Mesh->GetFaces()->GetCellIds(cellId, ids);
            for (int j = 0; j < n; ++j) {
                const igIndex pt = ids[j];
                if (m_PointMap[pt] < 0) { m_PointMap[pt] = pointNumber++; }
                for (igIndex nb : m_PointFaces[pt]) {
                    if (m_Visited[nb] < 0 && CellIsConnected(nb)) { wave2.push_back(nb); }
                }
            }
        }
        wave.swap(wave2);
    }
    return count;
}

bool ConnectivityFilter::Execute() {
    m_Message.clear();
    m_Mesh = DynamicCast<SurfaceMesh>(GetInput(0));
    if (m_Mesh == nullptr) {
        m_Message = "ConnectivityFilter only supports SurfaceMesh input";
        return false;
    }
    const IGsize nPts = m_Mesh->GetNumberOfPoints();
    const IGsize nCells = m_Mesh->GetNumberOfFaces();
    if (nPts == 0 || nCells == 0) {
        m_Message = "Input surface mesh is empty";
        return false;
    }
    if (m_ExtractionMode < POINT_SEEDED_REGIONS || m_ExtractionMode > CLOSEST_POINT_REGION) {
        m_Message = "Invalid extraction mode";
        return false;
    }

    m_ScalarArray = nullptr;
    if (m_ScalarConnectivity && !ResolveScalarArray()) { return false; }

    BuildPointFaceAdjacency();
    m_Visited.assign(nCells, -1);
    m_PointMap.assign(nPts, -1);
    m_RegionSizes.clear();

    igIndex pointNumber = 0;
    igIndex largestRegionId = 0;
    IGsize maxCellsInRegion = 0;

    const bool seededMode = (m_ExtractionMode == POINT_SEEDED_REGIONS ||
                             m_ExtractionMode == CELL_SEEDED_REGIONS ||
                             m_ExtractionMode == CLOSEST_POINT_REGION);

    if (!seededMode) {
        int regionNumber = 0;
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] >= 0) { continue; }
            const IGsize marked =
                TraverseAndMark(std::vector<igIndex>{static_cast<igIndex>(cellId)}, regionNumber, pointNumber);
            m_RegionSizes.push_back(marked);
            if (marked > maxCellsInRegion) {
                maxCellsInRegion = marked;
                largestRegionId = regionNumber;
            }
            ++regionNumber;
            UpdateProgress(0.7 * static_cast<double>(cellId + 1) / nCells);
        }
    } else {
        std::vector<igIndex> seeds;
        if (m_ExtractionMode == POINT_SEEDED_REGIONS) {
            for (igIndex pt : m_Seeds) {
                if (pt < 0 || pt >= static_cast<igIndex>(nPts)) { continue; }
                for (igIndex face : m_PointFaces[pt]) { seeds.push_back(face); }
            }
        } else if (m_ExtractionMode == CELL_SEEDED_REGIONS) {
            for (igIndex cell : m_Seeds) {
                if (cell >= 0 && cell < static_cast<igIndex>(nCells)) { seeds.push_back(cell); }
            }
        } else { // CLOSEST_POINT_REGION
            igIndex minId = 0;
            double minDist = std::numeric_limits<double>::max();
            const double* c = m_ClosestPoint;
            for (IGsize p = 0; p < nPts; ++p) {
                const Point& q = m_Mesh->GetPoint(p);
                const double dx = q[0] - c[0], dy = q[1] - c[1], dz = q[2] - c[2];
                const double d = dx * dx + dy * dy + dz * dz;
                if (d < minDist) { minDist = d; minId = static_cast<igIndex>(p); }
            }
            for (igIndex face : m_PointFaces[minId]) { seeds.push_back(face); }
        }
        if (seeds.empty()) {
            m_Message = "No seed cells specified for the seeded extraction mode";
            return false;
        }
        const IGsize marked = TraverseAndMark(seeds, 0, pointNumber);
        m_RegionSizes.push_back(marked);
        largestRegionId = 0;
    }

    if (m_RegionSizes.empty()) {
        m_Message = "No connected region found";
        return false;
    }

    // 决定保留哪些面。
    std::vector<char> keep(nCells, 0);
    if (m_ExtractionMode == ALL_REGIONS) {
        std::fill(keep.begin(), keep.end(), 1);
    } else if (m_ExtractionMode == LARGEST_REGION) {
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] == static_cast<int>(largestRegionId)) { keep[cellId] = 1; }
        }
    } else if (m_ExtractionMode == SPECIFIED_REGIONS) {
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            const int r = m_Visited[cellId];
            if (r >= 0 && std::find(m_SpecifiedRegionIds.begin(), m_SpecifiedRegionIds.end(), r) !=
                              m_SpecifiedRegionIds.end()) {
                keep[cellId] = 1;
            }
        }
    } else { // 种子类模式：保留全部被访问到的面
        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            if (m_Visited[cellId] >= 0) { keep[cellId] = 1; }
        }
    }

    // RegionId 重编号映射（按区域单元数排序）。
    std::vector<int> regionMap(m_RegionSizes.size());
    std::iota(regionMap.begin(), regionMap.end(), 0);
    if (m_RegionIdAssignmentMode == CELL_COUNT_ASCENDING ||
        m_RegionIdAssignmentMode == CELL_COUNT_DESCENDING) {
        std::stable_sort(regionMap.begin(), regionMap.end(), [&](int a, int b) {
            return m_RegionSizes[a] < m_RegionSizes[b];
        });
        if (m_RegionIdAssignmentMode == CELL_COUNT_DESCENDING) {
            std::reverse(regionMap.begin(), regionMap.end());
        }
        std::vector<igIndex> reordered(m_RegionSizes.size());
        std::vector<int> oldToNew(m_RegionSizes.size());
        for (size_t newId = 0; newId < regionMap.size(); ++newId) {
            oldToNew[regionMap[newId]] = static_cast<int>(newId);
            reordered[newId] = m_RegionSizes[regionMap[newId]];
        }
        m_RegionSizes = reordered;
        regionMap = oldToNew;
    }

    // 重建输出网格。
    auto output = SurfaceMesh::New();
    output->SetName(m_Mesh->GetName());
    auto outPoints = Points::New();
    auto outFaces = CellArray::New();

    std::vector<igIndex> oldToNewPoint(nPts, -1);
    std::vector<int> pointRegion; // 输出点 -> 区域号
    std::vector<int> cellRegion;  // 输出面 -> 区域号
    std::vector<igIndex> keptCells;

    std::vector<igIndex> newIds;
    const igIndex* ids = nullptr;
    for (IGsize cellId = 0; cellId < nCells; ++cellId) {
        if (!keep[cellId]) { continue; }
        const int n = m_Mesh->GetFaces()->GetCellIds(cellId, ids);
        if (n <= 0) { continue; }
        newIds.resize(n);
        for (int j = 0; j < n; ++j) {
            const igIndex old = ids[j];
            if (oldToNewPoint[old] < 0) {
                oldToNewPoint[old] = static_cast<igIndex>(outPoints->GetNumberOfPoints());
                outPoints->AddPoint(m_Mesh->GetPoint(old));
                pointRegion.push_back(m_Visited[cellId]);
            }
            newIds[j] = oldToNewPoint[old];
        }
        outFaces->AddCellIds(newIds.data(), static_cast<int>(n));
        cellRegion.push_back(m_Visited[cellId]);
        keptCells.push_back(cellId);
    }

    if (keptCells.empty()) {
        m_Message = "Extraction produced an empty mesh";
        return false;
    }
    output->SetPoints(outPoints);
    output->SetFaces(outFaces);

    // 搬运原有点/单元属性（保持原类型与数组长度）。
    auto inAttrs = m_Mesh->GetAttributeSet();
    auto outAttrs = AttributeSet::New();
    const IGsize outPointNum = outPoints->GetNumberOfPoints();
    const IGsize outCellNum = static_cast<IGsize>(keptCells.size());
    if (inAttrs != nullptr) {
        auto all = inAttrs->GetAllAttributes();
        if (all) {
            const IGsize attrNum = all->GetNumberOfElements();
            for (IGsize a = 0; a < attrNum; ++a) {
                auto& attr = all->GetElement(a);
                if (attr.isDeleted || attr.pointer.IsNull()) { continue; }
                if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) { continue; }

                auto inArray = attr.pointer;
                auto outArray = CreateArrayOfSameType(inArray);
                outArray->SetName(inArray->GetName());
                const int dim = inArray->GetDimension();
                outArray->SetDimension(dim);

                std::vector<double> values(dim > 0 ? dim : 1);
                if (attr.attachmentType == IG_POINT) {
                    // 输出点 -> 输入点
                    std::vector<igIndex> newToOld(outPointNum, -1);
                    for (IGsize old = 0; old < nPts; ++old) {
                        if (oldToNewPoint[old] >= 0) { newToOld[oldToNewPoint[old]] = old; }
                    }
                    outArray->Resize(outPointNum);
                    for (IGsize t = 0; t < outPointNum; ++t) {
                        inArray->GetElement(newToOld[t], values);
                        outArray->SetElement(t, values.data());
                    }
                } else {
                    outArray->Resize(outCellNum);
                    for (IGsize t = 0; t < outCellNum; ++t) {
                        inArray->GetElement(keptCells[t], values);
                        outArray->SetElement(t, values.data());
                    }
                }
                const IGsize index = outAttrs->AddAttribute(attr.type, attr.attachmentType, outArray);
                if (index != static_cast<IGsize>(-1)) {
                    outAttrs->GetAttribute(index).UpdateAllDataRange();
                }
            }
        }
    }

    // 写入 RegionId 标量（点 + 单元）。
    int regionIdAttrIndex = -1;
    if (m_ColorRegions) {
        auto pointRegionId = IntArray::New();
        pointRegionId->SetName(RegionIdName);
        pointRegionId->SetDimension(1);
        pointRegionId->Resize(outPointNum);
        for (IGsize t = 0; t < outPointNum; ++t) {
            pointRegionId->SetValue(t, regionMap[pointRegion[t]]);
        }
        regionIdAttrIndex = static_cast<int>(
            outAttrs->AddAttribute(IG_SCALAR, IG_POINT, pointRegionId));
        if (regionIdAttrIndex >= 0) {
            outAttrs->GetAttribute(regionIdAttrIndex).UpdateAllDataRange();
        }

        auto cellRegionId = IntArray::New();
        cellRegionId->SetName(RegionIdName);
        cellRegionId->SetDimension(1);
        cellRegionId->Resize(outCellNum);
        for (IGsize t = 0; t < outCellNum; ++t) {
            cellRegionId->SetValue(t, regionMap[cellRegion[t]]);
        }
        const IGsize cellIdx = outAttrs->AddAttribute(IG_SCALAR, IG_CELL, cellRegionId);
        if (cellIdx != static_cast<IGsize>(-1)) {
            outAttrs->GetAttribute(cellIdx).UpdateAllDataRange();
        }
    }

    output->SetAttributeSet(outAttrs);
    output->ConvertToDrawableData();

    UpdateProgress(1.0);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
