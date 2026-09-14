#include "iGamePointCoordinatesFilter.h"

#include "iGameAttributeSet.h"
#include "iGamePoints.h"

IGAME_NAMESPACE_BEGIN

PointCoordinatesFilter::PointCoordinatesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void PointCoordinatesFilter::SetArrayName(const std::string& name) {
    if (!name.empty()) { m_ArrayName = name; }
}

bool PointCoordinatesFilter::Execute() {
    m_CoordinatesArray = nullptr;
    SetOutput(nullptr);

    auto input = GetInput(0);
    if (!input) {
        igDebug("PointCoordinatesFilter requires a non-null input.");
        return false;
    }

    auto points = input->GetPoints();
    if (!points) {
        igDebug("PointCoordinatesFilter requires input data with points.");
        return false;
    }

    auto coordinates = points->ConvertToArray();
    if (!coordinates || coordinates->GetDimension() != 3) {
        igDebug("PointCoordinatesFilter requires three-component point coordinates.");
        return false;
    }

    auto attributes = input->GetAttributeSet();
    if (!attributes) {
        igDebug("PointCoordinatesFilter could not access the input attribute set.");
        return false;
    }

    const int existingIndex = attributes->GetAttributeIndex(m_ArrayName);
    if (existingIndex >= 0) {
        auto& existing = attributes->GetAttribute(existingIndex);
        if (existing.GetPointer().get() != coordinates.get() || existing.GetType() != IG_VECTOR ||
            existing.GetAttachmentType() != IG_POINT) {
            igDebug("PointCoordinatesFilter cannot create array '{}': the name is already in use.", m_ArrayName);
            return false;
        }

        // Point coordinates and the generated attribute share the same storage.
        // Re-advertise that storage as modified and rebuild its cached ranges so
        // repeated execution reflects edits made to the currently selected model.
        coordinates->Modified();
        existing.UpdateAllDataRange();
        input->Modified();
        m_CoordinatesArray = coordinates;
        SetOutput(input);
        return true;
    }

    coordinates->SetName(m_ArrayName);
    coordinates->Modified();
    const auto coordinatesIndex = attributes->AddAttribute(IG_VECTOR, IG_POINT, coordinates);
    if (coordinatesIndex < 0) {
        igDebug("PointCoordinatesFilter failed to add array '{}'.", m_ArrayName);
        return false;
    }

    attributes->GetAttribute(coordinatesIndex).UpdateAllDataRange();
    input->Modified();
    m_CoordinatesArray = coordinates;
    SetOutput(input);
    return true;
}

IGAME_NAMESPACE_END
