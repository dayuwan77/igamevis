#include "iGameElevationFilter.h"

IGAME_NAMESPACE_BEGIN

bool ElevationFilter::Execute() {
    // Guard clauses: valid input, valid mapping range, non-empty mesh.
    if (GetInput(0) == nullptr) {
        igError("ElevationFilter: null input");
        return false;
    }
    auto mesh = DynamicCast<PointSet>(GetInput(0));
    if (mesh == nullptr) {
        igError("ElevationFilter: input is not a PointSet");
        return false;
    }
    if (!(m_High > m_Low)) {
        igError("ElevationFilter: invalid output range [{}, {}]", m_Low, m_High);
        return false;
    }
    const IGsize nPoints = mesh->GetNumberOfPoints();
    if (nPoints == 0) {
        igError("ElevationFilter: empty mesh");
        return false;
    }

    IGAME_CORE_INFO("ElevationFilter: Execute() start (axis = {}, output range = [{}, {}])",
                    AxisIndex() == 0 ? "X" : (AxisIndex() == 1 ? "Y" : "Z"), m_Low, m_High);

    const int axis = AxisIndex();

    // Pass 1: source range [vMin, vMax] on the selected axis.
    float vMin = std::numeric_limits<float>::max();
    float vMax = std::numeric_limits<float>::lowest();
    for (IGsize i = 0; i < nPoints; ++i) {
        const float v = mesh->GetPoint(i)[axis];
        vMin = std::min(vMin, v);
        vMax = std::max(vMax, v);
    }
    IGAME_CORE_INFO("ElevationFilter: nPoints = {}, source range = [{}, {}]", nPoints, vMin, vMax);

    // Degenerate (flat) mesh: no elevation spread, fall back to constant Low.
    auto elevArray = FloatArray::New();
    elevArray->SetName(m_ArrayName);
    elevArray->SetDimension(1);
    elevArray->Resize(nPoints);

    const float span = vMax - vMin;
    if (span <= 0.0f) {
        IGAME_CORE_WARN("ElevationFilter: flat mesh on the selected axis, all values set to {}", m_Low);
        for (IGsize i = 0; i < nPoints; ++i) { elevArray->SetValue(i, m_Low); }
    } else {
        // Pass 2: affine mapping v -> Low + (v - vMin) / (vMax - vMin) * (High - Low).
        const float scale = (m_High - m_Low) / span;
        for (IGsize i = 0; i < nPoints; ++i) {
            const float v = mesh->GetPoint(i)[axis];
            elevArray->SetValue(i, m_Low + (v - vMin) * scale);
        }
    }

    // Attach the array as a point scalar attribute and pass the mesh through.
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, elevArray);
    SetOutput(mesh);
    mesh->Modified();

    IGAME_CORE_INFO("ElevationFilter: added '{}' ({} values), done", m_ArrayName, nPoints);
    return true;
}

IGAME_NAMESPACE_END
