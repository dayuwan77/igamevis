#include "Elevation/iGameElevationFilter.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameMacro.h"
#include "iGamePointSet.h"

#include <limits>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

ElevationFilter::ElevationFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ElevationFilter::SetDirection(float dx, float dy, float dz) {
    if (dx == 0.f && dy == 0.f && dz == 0.f) { return false; }
    if (m_Direction[0] != dx || m_Direction[1] != dy || m_Direction[2] != dz) {
        m_Direction = Vector3f(dx, dy, dz);
        this->Modified();
    }
    return true;
}

bool ElevationFilter::SetDirection(const Vector3f& d) {
    return SetDirection(d[0], d[1], d[2]);
}

void ElevationFilter::SetOutputRange(double low, double high) {
    if (low >= high) { return; }
    if (m_Low != low || m_High != high) {
        m_Low = low;
        m_High = high;
        this->Modified();
    }
}

void ElevationFilter::SetArrayName(const std::string& name) {
    if (m_ArrayName != name) {
        m_ArrayName = name;
        this->Modified();
    }
}

bool ElevationFilter::Execute() {
    const double dNorm = m_Direction.norm();
    IGAME_CORE_INFO("ElevationFilter: Execute() start (direction = ({}, {}, {}), "
                    "output range = [{}, {}])",
                    m_Direction[0], m_Direction[1], m_Direction[2], m_Low, m_High);

    auto obj = GetInput(0);
    if (obj == nullptr) {
        igError("ElevationFilter: GetInput(0) is nullptr!");
        return false;
    }
    auto mesh = DynamicCast<PointSet>(obj);
    if (mesh == nullptr) {
        igError("ElevationFilter: DynamicCast<PointSet> failed!");
        return false;
    }
    if (dNorm == 0.0) {
        igError("ElevationFilter: direction vector is zero!");
        return false;
    }

    const IGsize nPoints = mesh->GetNumberOfPoints();
    if (nPoints == 0) {
        igError("ElevationFilter: No points in mesh!");
        return false;
    }

    // 第一趟：求投影范围 [hMin, hMax]
    double hMin = std::numeric_limits<double>::max();
    double hMax = std::numeric_limits<double>::lowest();
    for (IGsize i = 0; i < nPoints; ++i) {
        const double h = mesh->GetPoint(i).dot(m_Direction);
        if (h < hMin) { hMin = h; }
        if (h > hMax) { hMax = h; }
    }
    IGAME_CORE_INFO("ElevationFilter: projection range: [{}, {}]", hMin, hMax);

    // 投影退化（网格垂直于方向）：降级输出常量 Low，避免除零产生 NaN
    const bool degenerate = (hMax <= hMin);
    if (degenerate) {
        IGAME_CORE_INFO("ElevationFilter: WARNING - all projections are identical "
                        "(flat mesh); every elevation value will be {}", m_Low);
    }

    auto elevArr = FloatArray::New();
    elevArr->SetName(m_ArrayName);
    elevArr->SetDimension(1);
    elevArr->Resize(nPoints);

    // 第二趟：仿射映射 h ∈ [hMin, hMax] → [Low, High]
    const double srcSpan = hMax - hMin;
    const double dstSpan = m_High - m_Low;
    for (IGsize i = 0; i < nPoints; ++i) {
        double out = m_Low;
        if (!degenerate) {
            const double h = mesh->GetPoint(i).dot(m_Direction);
            out = m_Low + (h - hMin) / srcSpan * dstSpan;
        }
        elevArr->SetValue(i, out);
    }

    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, elevArr);

    IGAME_CORE_INFO("ElevationFilter: Added {} (IG_SCALAR/IG_POINT), elements = {}",
                    m_ArrayName, elevArr->GetNumberOfElements());

    SetOutput(0, mesh);
    IGAME_CORE_INFO("ElevationFilter: Execute() done, returning true");
    return true;
}

IGAME_NAMESPACE_END
