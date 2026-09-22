#include "iGameForceStaticMeshFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDataObject.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

namespace {
// 深拷贝输入网格，生成一个独立的新网格作为静态缓存
iGame::DataObject::Pointer CloneMesh(iGame::DataObject::Pointer input) {
    using namespace iGame;
    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto pts = Points::New();
        pts->DeepCopy(src->GetPoints());
        return pts;
    };
    auto copyAttrs = [](DataObject* src) -> AttributeSet::Pointer {
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(src->GetAttributeSet());
        return attrs;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            auto dst = SurfaceMesh::New();
            dst->SetPoints(copyPoints(src));
            if (src->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(src->GetFaces());
                dst->SetFaces(faces);
            }
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            auto cells = CellArray::New();
            cells->DeepCopy(src->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(src->GetCellTypes());
            dst->SetCells(cells, types);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            auto dst = VolumeMesh::New();
            dst->SetPoints(copyPoints(src));
            auto vols = CellArray::New();
            vols->DeepCopy(src->GetVolumes());
            dst->SetVolumes(vols);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            auto dst = StructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_POINT_SET: {
            auto src = DynamicCast<PointSet>(input);
            auto dst = PointSet::New();
            dst->SetPoints(copyPoints(src));
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        default:
            return nullptr;
    }
}
} // namespace

IGAME_NAMESPACE_BEGIN

bool ForceStaticMeshFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) return false;

    if (m_ForceCacheComputation || !IsValidCache(input)) {
        // 首次执行或缓存失效：深拷贝输入，构建静态网格缓存（几何固定）
        m_Cache = CloneMesh(input);
        if (m_Cache == nullptr) return false;
        m_CacheInitialized = true;
    } else {
        // 缓存有效：仅更新属性数据（点/单元几何保持缓存不动）
        InputToCache(input);
    }

    if (auto attrSet = DynamicCast<PointSet>(m_Cache)->GetAttributeSet()) {
        attrSet->ForceReConvertToDrawableData();
    }

    SetOutput(m_Cache);
    return true;
}

bool ForceStaticMeshFilter::IsValidCache(const DataObject::Pointer& input) {
    if (!m_CacheInitialized || m_Cache == nullptr) return false;

    auto inPs = DynamicCast<PointSet>(input);
    auto cachePs = DynamicCast<PointSet>(m_Cache);
    if (inPs == nullptr || cachePs == nullptr) return false;
    if (inPs->GetNumberOfPoints() != cachePs->GetNumberOfPoints()) return false;

    // 按网格类型取真实单元数：StructuredMesh 单元由维度隐式决定且 GetCellArray 为空，
    // 直接用它的 GetNumberOfCells()，避免"始终为 0"导致判断失效。
    auto cellCount = [](const DataObject::Pointer& obj) -> IGsize {
        if (auto st = DynamicCast<StructuredMesh>(obj)) return st->GetNumberOfCells();
        if (auto vm = DynamicCast<VolumeMesh>(obj)) return vm->GetNumberOfVolumes();
        if (auto sm = DynamicCast<SurfaceMesh>(obj)) return sm->GetNumberOfFaces();
        if (auto um = DynamicCast<UnstructuredMesh>(obj)) return um->GetNumberOfCells();
        auto ca = obj ? obj->GetCellArray() : nullptr;
        return ca ? ca->GetNumberOfCells() : 0;
    };
    return cellCount(input) == cellCount(m_Cache);
}

void ForceStaticMeshFilter::InputToCache(const DataObject::Pointer& input) {
    if (m_Cache == nullptr || input == nullptr) return;

    auto cachePs = DynamicCast<PointSet>(m_Cache);
    auto inPs = DynamicCast<PointSet>(input);
    if (cachePs == nullptr || inPs == nullptr) return;

    // 用输入的属性数据替换缓存中的属性，几何保持缓存不动
    auto attrs = AttributeSet::New();
    attrs->DeepCopy(inPs->GetAttributeSet());
    m_Cache->SetAttributeSet(attrs);
}

void ForceStaticMeshFilter::SetForceCacheComputation(bool on) { m_ForceCacheComputation = on; }

bool ForceStaticMeshFilter::GetForceCacheComputation() const { return m_ForceCacheComputation; }

ForceStaticMeshFilter::ForceStaticMeshFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
