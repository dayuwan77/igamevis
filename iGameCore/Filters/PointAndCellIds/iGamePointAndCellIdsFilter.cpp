#include "iGamePointAndCellIdsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

namespace
{

int FindAttributeIndex(AttributeSet* attributes, const std::string& name, IGenum attachmentType) {
    auto allAttributes = attributes->GetAllAttributes();
    if (!allAttributes) {
        return -1;
    }

    int result = -1;

    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = allAttributes->GetElement(i);

        if (attribute.IsNone() ||
            attribute.GetAttachmentType() != attachmentType ||
            attribute.GetPointer()->GetName() != name) {
            continue;
        }

        if (result >= 0) {
            return -2;
        }

        result = static_cast<int>(i);
    }

    return result;
}

bool GetCellCount(const DataObject::Pointer& input, IGsize& count) {
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            count = DynamicCast<SurfaceMesh>(input)->GetNumberOfFaces();
            return true;

        case IG_VOLUME_MESH:
            count = DynamicCast<VolumeMesh>(input)->GetNumberOfVolumes();
            return true;

        case IG_UNSTRUCTURED_MESH:
            count = DynamicCast<UnstructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_STRUCTURED_MESH:
            count = DynamicCast<StructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_LAGRANGE_UNSTRUCTURED_MESH:
            count = DynamicCast<LagrangeUnstructuredMesh>(input)->GetNumberOfCells();
            return true;

        case IG_POINT_SET:
        default:
        return false;
    }
}

bool GenerateIds(AttributeSet* attributes,
                 IGenum attachmentType,
                 const std::string& name,
                 IGsize count,
                 LongLongArray::Pointer& output) {
    const int existingIndex = FindAttributeIndex(attributes, name, attachmentType);

    if (existingIndex == -2) {
        return false;
    }

    int attributeIndex = existingIndex;

    if (existingIndex >= 0) {
        auto& attribute = attributes->GetAttribute(existingIndex);

        if (attribute.GetType() != IG_SCALAR) {
            return false;
        }

        output = DynamicCast<LongLongArray>(attribute.GetPointer());
        if (!output) {
            return false;
        }
    } else {
        output = LongLongArray::New();
        output->SetName(name);

        const IGsize newIndex = attributes->AddScalar(attachmentType, output);
        if (newIndex == static_cast<IGsize>(-1)) {
            return false;
        }

        attributeIndex = static_cast<int>(newIndex);
    }

    output->SetName(name);
    output->SetDimension(1);
    output->Resize(count);

    for (IGsize i = 0; i < count; ++i) {
        output->SetValue(i, static_cast<long long>(i));
    }

    if (count > 0) {
        attributes->GetAttribute(attributeIndex).UpdateAllDataRange();
    }

    return true;
}

}

PointAndCellIdsFilter::PointAndCellIdsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void PointAndCellIdsFilter::SetGeneratePointIds(bool value) {
    if (m_GeneratePointIds != value) {
        m_GeneratePointIds = value;
        Modified();
    }
}

void PointAndCellIdsFilter::SetGenerateCellIds(bool value) {
    if (m_GenerateCellIds != value) {
        m_GenerateCellIds = value;
        Modified();
    }
}

void PointAndCellIdsFilter::SetPointIdsArrayName(const std::string& name) {
    if (m_PointIdsArrayName != name) {
        m_PointIdsArrayName = name;
        Modified();
    }
}

void PointAndCellIdsFilter::SetCellIdsArrayName(const std::string& name) {
    if (m_CellIdsArrayName != name) {
        m_CellIdsArrayName = name;
        Modified();
    }
}

bool PointAndCellIdsFilter::Execute() {
    m_Message.clear();
    m_PointIdsArray = nullptr;
    m_CellIdsArray = nullptr;
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input) {
        m_Message = "PointAndCellIdsFilter has no input DataObject.";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (!mesh) {
        m_Message = "PointAndCellIdsFilter requires a PointSet input.";
        return false;
    }

    auto attributes = input->GetAttributeSet();
    if (!attributes) {
        m_Message = "PointAndCellIdsFilter input has no AttributeSet.";
        return false;
    }

    if (m_GeneratePointIds) {
        if (m_PointIdsArrayName.empty()) {
            m_Message = "Point IDs array name cannot be empty.";
            return false;
        }

        if (!GenerateIds(attributes,
                         IG_POINT,
                         m_PointIdsArrayName,
                         mesh->GetNumberOfPoints(),
                         m_PointIdsArray)) {
            m_Message = "Failed to generate point IDs.";
            return false;
        }
    }

    if (m_GenerateCellIds) {
        if (m_CellIdsArrayName.empty()) {
            m_Message = "Cell IDs array name cannot be empty.";
            return false;
        }

        IGsize cellCount = 0;
        if (!GetCellCount(input, cellCount)) {
            m_Message = "Unsupported mesh type for cell IDs.";
            return false;
        }

        if (!GenerateIds(attributes,
                         IG_CELL,
                         m_CellIdsArrayName,
                         cellCount,
                         m_CellIdsArray)) {
            m_Message = "Failed to generate cell IDs.";
            return false;
        }
    }

    input->Modified();
    SetOutput(input);

    m_Message = "Point and cell IDs generated successfully.";
    return true;
}

IGAME_NAMESPACE_END