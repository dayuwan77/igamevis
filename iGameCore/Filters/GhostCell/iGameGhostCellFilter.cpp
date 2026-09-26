#include "iGameGhostCellFilter.h"
#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameFlatArray.h"

IGAME_NAMESPACE_BEGIN

GhostCellFilter::GhostCellFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool GhostCellFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) return false;

    std::vector<char> pointGhosts;
    bool hasPointGhosts = LoadPointGhostArray(input, pointGhosts);

    std::vector<char> cellGhosts;
    if (!ComputeCellGhosts(input, pointGhosts, hasPointGhosts, cellGhosts)) return false;

    if (!AttachCellGhostArray(input, cellGhosts)) return false;

    SetOutput(0, input);
    return true;
}

bool GhostCellFilter::LoadPointGhostArray(DataObject::Pointer input, std::vector<char>& pointGhosts) {
    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr) return false;

    int idx = attrs->GetAttributeIndex(m_PointGhostArrayName);
    if (idx < 0) return false;

    auto& attr = attrs->GetAttribute(idx);
    if (attr.attachmentType != IG_POINT) return false;
    if (attr.pointer.IsNull()) return false;

    IGsize n = attr.pointer->GetNumberOfElements();
    pointGhosts.assign(n, 0);
    for (IGsize i = 0; i < n; i++) {
        if (attr.pointer->GetValue(i) != 0.0) pointGhosts[i] = 1;
    }
    return n > 0;
}

bool GhostCellFilter::ComputeCellGhosts(DataObject::Pointer input, const std::vector<char>& pointGhosts,
                                        bool hasPointGhosts, std::vector<char>& cellGhosts) {

    auto markRange = [&](IGsize count, auto getCellPointIds) -> bool {
        cellGhosts.assign(count, 0);
        if (!hasPointGhosts) return true;
        if (count == 0) return true;

        igIndex ids[IGAME_CELL_MAX_SIZE];
        for (IGsize c = 0; c < count; c++) {
            int n = getCellPointIds(c, ids);
            char ghost = 0;
            for (int k = 0; k < n; k++) {
                if (ids[k] >= 0 && (IGsize) ids[k] < pointGhosts.size() && pointGhosts[ids[k]]) {
                    ghost = 1;
                    break;
                }
            }
            cellGhosts[c] = ghost;
            if ((c & 0x3FF) == 0) UpdateProgress((double) c / (double) count);
        }
        return true;
    };

    // 体网格
    if (auto mesh = DynamicCast<VolumeMesh>(input)) {
        return markRange(mesh->GetNumberOfVolumes(),
                         [mesh](IGsize c, igIndex* ids) { return mesh->GetVolumePointIds(c, ids); });
    }
    // 表面网格
    if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        return markRange(mesh->GetNumberOfFaces(),
                         [mesh](IGsize c, igIndex* ids) { return mesh->GetFacePointIds(c, ids); });
    }
    // 非结构化网格
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        return markRange(mesh->GetNumberOfCells(),
                         [mesh](IGsize c, igIndex* ids) { return mesh->GetCellPointIds(c, ids); });
    }
    return false;
}

bool GhostCellFilter::AttachCellGhostArray(DataObject::Pointer input, const std::vector<char>& cellGhosts) {
    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr) return false;

    CharArray::Pointer marker = nullptr;
    int idx = attrs->GetAttributeIndex("GhostCells");
    if (idx >= 0) { marker = DynamicCast<CharArray>(attrs->GetAttribute(idx).pointer); }
    if (marker.IsNull()) {
        marker = CharArray::New();
        marker->SetName("GhostCells");
        attrs->AddScalar(IG_CELL, marker);
    }

    // 0 = 正常单元，1 = ghost 单元
    marker->Resize((IGsize) cellGhosts.size());
    for (IGsize i = 0; i < (IGsize) cellGhosts.size(); i++) { marker->SetValue(i, cellGhosts[i] ? 1.0 : 0.0); }
    return true;
}

IGAME_NAMESPACE_END
