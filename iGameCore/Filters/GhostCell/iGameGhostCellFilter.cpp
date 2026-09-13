#include "iGameGhostCellFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

GhostCellFilter::GhostCellFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

int GhostCellFilter::FindGhostArrayIndex(DataObject::Pointer input) const {
    if (input.IsNull() || m_GhostArrayName.empty()) { return -1; }
    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr) { return -1; }

    const int idx = attrs->GetAttributeIndex(m_GhostArrayName);
    if (idx < 0) { return -1; }
    auto& a = attrs->GetAttribute(idx);
    if (a.isDeleted || a.pointer.IsNull()) { return -1; }
    if (a.attachmentType != IG_CELL) { return -1; }    // 必须是单元数组
    if (a.pointer->GetDimension() != 1) { return -1; } // 必须单分量
    return idx;
}

bool GhostCellFilter::AttachMask(DataObject::Pointer output, IGsize cellCount, ArrayObject::Pointer src) const {
    auto attrs = output->GetAttributeSet();
    if (attrs == nullptr) { return false; }

    auto mask = CharArray::New();
    mask->SetName(m_OutputArrayName);
    mask->Resize(cellCount);
    for (IGsize c = 0; c < cellCount; c++) {
        const long value = static_cast<long>(src->GetValue(c));
        bool hit = false;
        if (m_CheckAny && value != 0) { hit = true; } // 任意 ghost 标记
        if (!hit && m_CheckDuplicateCell && (value & DUPLICATE_CELL)) { hit = true; }
        if (!hit && m_CheckHiddenCell && (value & HIDDEN_CELL)) { hit = true; }
        mask->SetValue(c, hit ? 1.0 : 0.0);
    }

    // 已有同名数组则原地替换，避免产生"已删除"的空占位
    const int exist = attrs->GetAttributeIndex(m_OutputArrayName);
    if (exist >= 0) {
        auto& a = attrs->GetAttribute(exist);
        a.pointer = mask;
        a.isDeleted = false;
        a.type = IG_CharArray;
        a.attachmentType = IG_CELL;
        a.UpdateAllDataRange();
        return true;
    }
    const IGsize idx = attrs->AddAttribute(IG_CharArray, IG_CELL, mask);
    if (idx < 0) { return false; }
    attrs->GetAttribute(idx).UpdateAllDataRange();
    return true;
}

bool GhostCellFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) {
        igError("GhostCellFilter: 输入为空。");
        return false;
    }

    // 1) 优先读取已有单元标记（默认 vtkGhostType），找不到就报错，不能静默输出全 0
    const int inIdx = FindGhostArrayIndex(input);
    if (inIdx < 0) {
        igError("GhostCellFilter: Cell Data 中找不到可用的单元标记数组 \"{}\"（要求单分量单元数组）。",
                m_GhostArrayName);
        return false;
    }
    auto srcArray = input->GetAttributeSet()->GetAttribute(inIdx).pointer;
    igDebug("GhostCellFilter: 使用单元标记数组 {}", srcArray->GetName());

    // 2) 未选择任何类型时，默认"任意 ghost 标记"
    if (!m_CheckAny && !m_CheckDuplicateCell && !m_CheckHiddenCell) { m_CheckAny = true; }

    // 3) 复制输入（独立输出），在副本上写入掩码
    if (auto mesh = DynamicCast<VolumeMesh>(input)) {
        auto out = VolumeMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto volumes = CellArray::New();
        volumes->DeepCopy(mesh->GetVolumes());
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(input->GetAttributeSet());
        out->SetPoints(points);
        out->SetVolumes(volumes);
        out->SetAttributeSet(attrs);
        if (!AttachMask(out, out->GetNumberOfVolumes(), srcArray)) { return false; }
        out->SetName(input->GetName() + "_ghostmask");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }
    if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        auto out = SurfaceMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto faces = CellArray::New();
        faces->DeepCopy(mesh->GetFaces());
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(input->GetAttributeSet());
        out->SetPoints(points);
        out->SetFaces(faces);
        out->SetAttributeSet(attrs);
        if (!AttachMask(out, out->GetNumberOfFaces(), srcArray)) { return false; }
        out->SetName(input->GetName() + "_ghostmask");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }
    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        auto out = UnstructuredMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto cells = CellArray::New();
        cells->DeepCopy(mesh->GetCells());
        auto types = UnsignedIntArray::New();
        types->Resize(mesh->GetNumberOfCells());
        auto inTypes = mesh->GetCellTypes();
        for (IGsize i = 0; i < mesh->GetNumberOfCells(); i++) { types->SetValue(i, inTypes->GetValue(i)); }
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(input->GetAttributeSet());
        out->SetPoints(points);
        out->SetCells(cells, types);
        out->SetAttributeSet(attrs);
        if (!AttachMask(out, out->GetNumberOfCells(), srcArray)) { return false; }
        out->SetName(input->GetName() + "_ghostmask");
        UpdateProgress(1.0);
        SetOutput(0, out);
        return true;
    }

    igError("GhostCellFilter: 不支持的网格类型。");
    return false;
}

IGAME_NAMESPACE_END