#include "iGameElevationFilter.h"

IGAME_NAMESPACE_BEGIN

bool ElevationFilter::SetDirection(float dx, float dy, float dz) {
    // 零向量没有投影方向，拒绝并保持原值
    if (dx == 0.f && dy == 0.f && dz == 0.f) {
        igError("ElevationFilter: zero direction vector rejected");
        return false;
    }
    m_Direction = Vector3f(dx, dy, dz);
    return true;
}

void ElevationFilter::SetAxis(Axis axis) {
    // 轴向 = 对应的单位基向量
    switch (axis) {
        case Axis::X: SetDirection(1.f, 0.f, 0.f); break;
        case Axis::Y: SetDirection(0.f, 1.f, 0.f); break;
        case Axis::Z: SetDirection(0.f, 0.f, 1.f); break;
    }
}

bool ElevationFilter::Execute() {
    // 卫语句：输入有效、映射区间合法、网格非空
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
    if (m_Direction.norm() == 0.0) {
        igError("ElevationFilter: zero direction vector");
        return false;
    }
    const IGsize nPoints = mesh->GetNumberOfPoints();
    if (nPoints == 0) {
        igError("ElevationFilter: empty mesh");
        return false;
    }

    IGAME_CORE_INFO("ElevationFilter: Execute() start (direction = ({}, {}, {}), "
                    "output range = [{}, {}])",
                    m_Direction[0], m_Direction[1], m_Direction[2], m_Low, m_High);

    // 第一趟：逐点求投影 h = p · d 的范围 [hMin, hMax]
    float hMin = std::numeric_limits<float>::max();
    float hMax = std::numeric_limits<float>::lowest();
    for (IGsize i = 0; i < nPoints; ++i) {
        const float h = static_cast<float>(mesh->GetPoint(i).dot(m_Direction));
        hMin = std::min(hMin, h);
        hMax = std::max(hMax, h);
    }
    IGAME_CORE_INFO("ElevationFilter: nPoints = {}, projection range = [{}, {}]",
                    nPoints, hMin, hMax);

    auto elevArray = FloatArray::New();
    elevArray->SetName(m_ArrayName);
    elevArray->SetDimension(1);
    elevArray->Resize(nPoints);

    const float span = hMax - hMin;
    if (span <= 0.0f) {
        // 网格垂直于方向（无高程差）：降级为常量 Low，避免除零
        IGAME_CORE_WARN("ElevationFilter: mesh perpendicular to direction, "
                        "all values set to {}", m_Low);
        for (IGsize i = 0; i < nPoints; ++i) { elevArray->SetValue(i, m_Low); }
    } else {
        // 第二趟：仿射映射 h -> Low + (h - hMin) / (hMax - hMin) * (High - Low)
        const float scale = (m_High - m_Low) / span;
        for (IGsize i = 0; i < nPoints; ++i) {
            const float h = static_cast<float>(mesh->GetPoint(i).dot(m_Direction));
            elevArray->SetValue(i, m_Low + (h - hMin) * scale);
        }
    }

    // 挂为点标量属性，原网格直通输出
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, elevArray);
    SetOutput(mesh);
    mesh->Modified();

    IGAME_CORE_INFO("ElevationFilter: added '{}' ({} values), done", m_ArrayName, nPoints);
    return true;
}

IGAME_NAMESPACE_END
