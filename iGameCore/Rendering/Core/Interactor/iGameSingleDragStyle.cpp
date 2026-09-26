#include "iGameSingleDragStyle.h"
#include "iGameInteractor.h"
#include "iGamePointPicker.h"
#include "iGameScene.h"

#include <algorithm>

IGAME_NAMESPACE_BEGIN

SingleDragStyle::SingleDragStyle() {
    m_SelectedPointId = -1;

    m_SelectedNDCZ = 0;
    m_MVP = igm::mat4{};
    m_InvertedMVP = igm::mat4{};
    m_LastMousePosition = igm::vec2{};
    m_DragDepthDirection = igm::vec3{0.0f, 0.0f, 1.0f};
    m_ConstraintAxis = ConstraintAxis::FreePlane;
}
SingleDragStyle::~SingleDragStyle() {}

void SingleDragStyle::MousePressEvent(IEvent event) {
    SelectionStyle::MousePressEvent(event);
    m_LastMousePosition = event.pos;
    m_MVP = m_Interactor->GetMVP();
    m_InvertedMVP = m_MVP.invert();

    auto& pos = event.pos;
    igm::vec3 point1 = GetNearWorldCoord(pos, m_InvertedMVP);
    igm::vec3 point2 = GetFarWorldCoord(pos, m_InvertedMVP);

    igm::vec3 dir = (point1 - point2).normalized();
    // 右键拖动时沿当前观察射线改变深度，补足二维屏幕拖动缺少的第三个自由度。
    m_DragDepthDirection = (point2 - point1).normalized();

    Point p;
    SmartPointer<PointPicker> picker = PointPicker::New();
    picker->SetPoints(m_Points);
    // 单点 PointSet 的自身包围盒尺寸为 0，PointPicker 的默认拾取半径也会变为 0，
    // 从而导致“点可见但永远拾取不到”。拖动交互应按当前场景尺度给出稳定半径。
    if (auto* scene = m_Interactor->GetScene()) {
        const auto sphere = scene->GetRotationBoundingSphere();
        picker->SetPickRadius(std::max(1.0e-6, static_cast<double>(sphere.w) * 0.02));
    }
    m_SelectedPointId = picker->PickClosetPointOnLine(
            Vector3d(point1.x, point1.y, point1.z),
            Vector3d(dir.x, dir.y, dir.z), p);

    //m_Model->GetPointPainter()->Clear();
    if (m_SelectedPointId != -1) {
        //std::cout << "click point id: " << m_SelectedPointId << std::endl;
        auto& tp = m_Points->GetPoint(m_SelectedPointId);
        igm::vec4 p{tp[0], tp[1], tp[2], 1.f};
        p = m_MVP * p;
        m_SelectedNDCZ = p.z / p.w;

        // 不绘制额外的红色选择点：可拖动 PointSet 本身就是唯一的查询点。
    }
}

void SingleDragStyle::MouseMoveEvent(IEvent event) {
    igm::vec2 pos = event.pos;

    if (m_MouseMode == MouseButton::LeftButton ||
        m_MouseMode == MouseButton::RightButton) {
        // 未命中可拖动点时，维持基础交互：左键仍可旋转视图。
        if (m_SelectedPointId == -1) {
            BasicStyle::MouseMoveEvent(event);
            return;
        }

        //std::cout << "drag point id: " << m_SelectedPointId << std::endl;

        if (m_Selection) {
            auto epos = m_Points->GetPoint(m_SelectedPointId);
            if (m_ConstraintAxis == ConstraintAxis::FreePlane &&
                m_MouseMode == MouseButton::LeftButton) {
                igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                              1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));
                igm::vec4 Point_NDC{NDC, m_SelectedNDCZ, 1.f};
                igm::vec4 newPoint_WorldCoord = m_InvertedMVP * Point_NDC;
                newPoint_WorldCoord /= newPoint_WorldCoord.w;
                epos = Vector3f{newPoint_WorldCoord.x, newPoint_WorldCoord.y,
                                newPoint_WorldCoord.z};
            } else if (m_ConstraintAxis == ConstraintAxis::FreePlane) {
                const igm::vec2 delta = pos - m_LastMousePosition;
                const float pixels = std::abs(delta.y) >= std::abs(delta.x)
                                             ? -delta.y
                                             : delta.x;
                float sceneRadius = 1.0f;
                if (auto* scene = m_Interactor->GetScene())
                    sceneRadius = std::max(1.0e-6f, scene->GetRotationBoundingSphere().w);
                const float viewportSize = std::max(
                        1.0f, std::max(m_Interactor->GetWidth(), m_Interactor->GetHeight()));
                const float displacement =
                        pixels * (2.0f * sceneRadius / viewportSize);
                epos[0] += m_DragDepthDirection.x * displacement;
                epos[1] += m_DragDepthDirection.y * displacement;
                epos[2] += m_DragDepthDirection.z * displacement;
            } else {
                const igm::vec2 delta = pos - m_LastMousePosition;
                const float pixels = std::abs(delta.x) >= std::abs(delta.y)
                                             ? delta.x
                                             : -delta.y;
                float sceneRadius = 1.0f;
                if (auto* scene = m_Interactor->GetScene())
                    sceneRadius = std::max(1.0e-6f, scene->GetRotationBoundingSphere().w);
                const float viewportSize = std::max(
                        1.0f, std::max(m_Interactor->GetWidth(), m_Interactor->GetHeight()));
                const float displacement = pixels * (2.0f * sceneRadius / viewportSize);
                const int component = static_cast<int>(m_ConstraintAxis) - 1;
                epos[component] += displacement;
            }
            // 必须先写入坐标，再通知观察者；否则 Qt 端读到的是旧位置。
            m_Points->SetPoint(m_SelectedPointId, epos);
            m_Selection->SelectionCallBackEvent(IG_DRAGPOINT,
                                                m_SelectedPointId);

            auto drawObject = DynamicCast<DrawObject>(m_Model->GetDataObject());
            if (drawObject) drawObject->ForceReConvertToDrawableData();

            //m_Model->GetPointPainter()->Clear();
            if (auto scene = m_Model->GetScene()) scene->Update();
        }
        m_LastMousePosition = pos;
    }
}
IGAME_NAMESPACE_END
