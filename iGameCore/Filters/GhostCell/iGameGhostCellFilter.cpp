#include "iGameGhostCellFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <vector>

IGAME_NAMESPACE_BEGIN

GhostCellFilter::GhostCellFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void GhostCellFilter::SetGhostArrayName(const std::string& name) {
    if (!name.empty()) { m_GhostArrayName = name; }
}

void GhostCellFilter::SetOutputArrayName(const std::string& name) {
    if (!name.empty()) { m_OutputArrayName = name; }
}

unsigned int GhostCellFilter::GetGhostTypeMask() const {
    unsigned int mask = 0;
    if (m_CheckDuplicateCell) { mask |= DUPLICATE_CELL; }
    if (m_CheckRefinedCell) { mask |= REFINED_CELL; }
    if (m_CheckExteriorCell) { mask |= EXTERIOR_CELL; }
    if (m_CheckHiddenCell) { mask |= HIDDEN_CELL; }
    return mask;
}

int GhostCellFilter::FindGhostArrayIndex(DataObject::Pointer input) const {
    if (input.IsNull() || m_GhostArrayName.empty()) { return -1; }
    auto attrs = input->GetAttributeSet();
    if (attrs == nullptr) { return -1; }

    const int idx = attrs->GetAttributeIndex(m_GhostArrayName);
    if (idx < 0) { return -1; }
    auto& attribute = attrs->GetAttribute(static_cast<IGsize>(idx));
    if (attribute.isDeleted || attribute.pointer.IsNull()) { return -1; }
    if (attribute.attachmentType != IG_CELL) { return -1; }    // 必须是单元数组
    if (attribute.pointer->GetDimension() != 1) { return -1; } // 必须单分量
    return idx;
}

ArrayObject::Pointer GhostCellFilter::CloneArray(ArrayObject::Pointer source) {
    if (source.IsNull()) { return nullptr; }

    ArrayObject::Pointer clone{};
    if (auto p = DynamicCast<FloatArray>(source)) {
        auto q = FloatArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<DoubleArray>(source)) {
        auto q = DoubleArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<IntArray>(source)) {
        auto q = IntArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<UnsignedIntArray>(source)) {
        auto q = UnsignedIntArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<CharArray>(source)) {
        auto q = CharArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<UnsignedCharArray>(source)) {
        auto q = UnsignedCharArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<ShortArray>(source)) {
        auto q = ShortArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<UnsignedShortArray>(source)) {
        auto q = UnsignedShortArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<LongLongArray>(source)) {
        auto q = LongLongArray::New();
        q->DeepCopy(p);
        clone = q;
    } else if (auto p = DynamicCast<UnsignedLongLongArray>(source)) {
        auto q = UnsignedLongLongArray::New();
        q->DeepCopy(p);
        clone = q;
    }
    if (clone.IsNull()) { return nullptr; }

    clone->SetName(source->GetName());
    clone->SetDimension(source->GetDimension());
    return clone;
}

bool GhostCellFilter::CopyAttributes(DataObject::Pointer output, DataObject::Pointer input) {
    auto outputAttrs = output->GetAttributeSet();
    if (outputAttrs == nullptr) { return false; }
    if (input.IsNull()) { return true; }
    auto inputAttrs = input->GetAttributeSet();
    if (inputAttrs == nullptr) { return true; }

    const IGsize count = static_cast<IGsize>(inputAttrs->GetNumberOfAttributes());
    for (IGsize i = 0; i < count; i++) {
        auto& attribute = inputAttrs->GetAttribute(i);
        // 跳过"已删除"和空指针：这两种都要跳过，否则输出里会留下空指针占位
        // （遍历 / 渲染属性时会读空指针，历史上就因此崩过）
        if (attribute.isDeleted || attribute.pointer.IsNull()) { continue; }

        auto cloned = CloneArray(attribute.pointer);
        if (cloned.IsNull()) { continue; } // 未知数组类型：跳过，不制造空占位

        const IGsize index = outputAttrs->AddAttribute(attribute.type, attribute.attachmentType, cloned);
        if (index < 0) { return false; }
        auto& added = outputAttrs->GetAttribute(index);
        added.isDeleted = false;
        added.UpdateAllDataRange();
    }
    return true;
}

bool GhostCellFilter::AttachMask(DataObject::Pointer output, IGsize cellCount, ArrayObject::Pointer ghostArray) const {
    if (output.IsNull() || ghostArray.IsNull()) { return false; }
    auto attrs = output->GetAttributeSet();
    if (attrs == nullptr) { return false; }

    // 一个类型都没勾时，按"任意 ghost 标记"处理
    const bool checkAny =
            m_CheckAny || (!m_CheckDuplicateCell && !m_CheckRefinedCell && !m_CheckExteriorCell && !m_CheckHiddenCell);
    const unsigned int typeMask = GetGhostTypeMask();

    auto mask = CharArray::New();
    mask->SetName(m_OutputArrayName);
    mask->SetDimension(1);
    mask->Resize(cellCount);
    for (IGsize c = 0; c < cellCount; c++) {
        const long value = static_cast<long>(ghostArray->GetValue(c));
        bool isGhost = false;
        if (checkAny && value != 0) {
            isGhost = true;
        } else if (typeMask != 0 && (static_cast<unsigned long>(value) & typeMask) != 0) {
            isGhost = true;
        }
        mask->SetValue(c, isGhost ? 1.0 : 0.0);
    }

    // 已经有"同名 + 同附着类型"的数组就原地替换，避免产生"已删除"的空占位
    const IGsize count = static_cast<IGsize>(attrs->GetNumberOfAttributes());
    for (IGsize i = 0; i < count; i++) {
        auto& attribute = attrs->GetAttribute(i);
        if (attribute.pointer.IsNull()) { continue; }
        if (attribute.attachmentType != IG_CELL) { continue; }
        if (attribute.pointer->GetName() != m_OutputArrayName) { continue; }
        attribute.pointer = mask;
        attribute.isDeleted = false;
        attribute.type = IG_SCALAR;
        attribute.attachmentType = IG_CELL;
        attribute.UpdateAllDataRange();
        return true;
    }

    const IGsize index = attrs->AddAttribute(IG_SCALAR, IG_CELL, mask);
    if (index < 0) { return false; }
    attrs->GetAttribute(index).UpdateAllDataRange();
    return true;
}

bool GhostCellFilter::Execute() {
    auto input = GetInput(0);
    if (input.IsNull()) {
        igError("GhostCellFilter: 输入为空。");
        return false;
    }

    // 1) 优先读现有单元标记（默认 vtkGhostType）；找不到就报错，不能静默输出全 0
    const int ghostIndex = FindGhostArrayIndex(input);
    if (ghostIndex < 0) {
        igError("GhostCellFilter: Cell Data 中找不到可用的单元标记数组 \"{}\"（要求单分量单元数组，"
                "例如标准的 vtkGhostType）。",
                m_GhostArrayName);
        return false;
    }
    auto ghostArray = input->GetAttributeSet()->GetAttribute(static_cast<IGsize>(ghostIndex)).pointer;
    igDebug("GhostCellFilter: 使用单元标记数组 {}（类型位掩码 = {}）", ghostArray->GetName(), GetGhostTypeMask());

    // 2) 独立输出：复制点、单元、所有属性，输入保持不变
    //    注意判断顺序：VolumeMesh 继承自 SurfaceMesh，必须先判断 VolumeMesh
    DataObject::Pointer output{};
    if (auto mesh = DynamicCast<VolumeMesh>(input)) {
        auto out = VolumeMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto volumes = CellArray::New();
        volumes->DeepCopy(mesh->GetVolumes());
        out->SetPoints(points);
        out->SetVolumes(volumes);
        output = out;
        const IGsize cellCount = out->GetNumberOfVolumes();
        if (!CopyAttributes(out, input) || !AttachMask(out, cellCount, ghostArray)) { return false; }
    } else if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        auto out = SurfaceMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto faces = CellArray::New();
        faces->DeepCopy(mesh->GetFaces());
        out->SetPoints(points);
        out->SetFaces(faces);
        output = out;
        const IGsize cellCount = out->GetNumberOfFaces();
        if (!CopyAttributes(out, input) || !AttachMask(out, cellCount, ghostArray)) { return false; }
    } else if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        auto out = UnstructuredMesh::New();
        auto points = Points::New();
        points->DeepCopy(mesh->GetPoints());
        auto cells = CellArray::New();
        cells->DeepCopy(mesh->GetCells());
        auto types = UnsignedIntArray::New();
        types->Resize(mesh->GetNumberOfCells());
        auto inputTypes = mesh->GetCellTypes();
        for (IGsize i = 0; i < mesh->GetNumberOfCells(); i++) {
            types->SetValue(i, inputTypes != nullptr ? inputTypes->GetValue(i) : 0);
        }
        out->SetPoints(points);
        out->SetCells(cells, types);
        output = out;
        const IGsize cellCount = out->GetNumberOfCells();
        if (!CopyAttributes(out, input) || !AttachMask(out, cellCount, ghostArray)) { return false; }
    } else {
        igError("GhostCellFilter: 不支持的网格类型（支持表面网格 / 体网格 / 非结构网格）。");
        return false;
    }
    if (output.IsNull()) {
        igError("GhostCellFilter: 生成输出失败。");
        return false;
    }
    output->SetName(input->GetName() + "_ghostmask");

    UpdateProgress(1.0);
    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END