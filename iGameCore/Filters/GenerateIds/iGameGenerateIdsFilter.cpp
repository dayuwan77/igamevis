#include "iGameGenerateIdsFilter.h"
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <iGamePointSet.h>
#include <iostream>

IGAME_NAMESPACE_BEGIN

iGameGenerateIdsFilter::iGameGenerateIdsFilter(IGenum dataType) {
    m_DataType = dataType;
    m_ArrayName = "Ids";
    m_StartId = 0;
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool iGameGenerateIdsFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) { return false; }

    if (m_DataType != IG_POINT && m_DataType != IG_CELL) { return false; }

    SetOutput(input);
    return Run();
}

bool iGameGenerateIdsFilter::Run() {
    auto output = GetOutput();
    IGsize count = 0;
    if (m_DataType == IG_POINT) {
        auto points = output->GetPoints();
        if (points == nullptr) { return false; }
        count = points->GetNumberOfPoints();
    } else {
        auto cells = output->GetCellArray();
        if (cells == nullptr) { return false; }
        count = cells->GetNumberOfCells();
    }

    if (count == 0) { return true; }

    LongLongArray::Pointer arr = LongLongArray::New();
    arr->SetName(m_ArrayName);
    arr->SetDimension(1);
    arr->Resize(count);

    for (IGsize i = 0; i < count; ++i) {
        arr->ValueAt(i) = m_StartId + static_cast<long long>(i);
    }

    auto attrs = output->GetAttributeSet();
    if (attrs == nullptr) {
        output->SetAttributeSet(AttributeSet::New());
        attrs = output->GetAttributeSet();
    }

    // Overwrite must match BOTH the array name and the attachment type
    // (IG_POINT / IG_CELL). Looking up by name alone would either keep adding
    // duplicate attributes or replace the attribute attached to the other type.
    int existing = -1;
    for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.pointer && attr.attachmentType == m_DataType &&
            attr.pointer->GetName() == m_ArrayName) {
            existing = static_cast<int>(i);
            break;
        }
    }

    if (existing >= 0) {
        auto& attr = attrs->GetAttribute(existing);
        attr.pointer = arr;
        attr.UpdateAllDataRange();
    } else {
        attrs->AddAttribute(IG_SCALAR, m_DataType, arr);
    }

    std::cout << "[INFO] Added array '" << m_ArrayName << "' with " << count << " elements to "
              << (m_DataType == IG_POINT ? "Point" : "Cell") << " data." << std::endl;
    std::cout << "[INFO] Total attributes now: " << attrs->GetNumberOfAttributes() << std::endl;

    if (auto draw = DynamicCast<DrawObject>(output)) { draw->ForceReConvertToDrawableData(); }
    return true;
}

IGAME_NAMESPACE_END
