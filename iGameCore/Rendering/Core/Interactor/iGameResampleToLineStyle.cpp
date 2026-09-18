#include "iGameResampleToLineStyle.h"

#include"iGameInteractor.h"

#include "iGameScene.h"

#include <algorithm>

ResampleToLineStyle::ResampleToLineStyle() {

}

ResampleToLineStyle::~ResampleToLineStyle() {
    for (int i = 0; i < 10; i++) {
        if (LineHandle[i] != 0) { m_Painter3D->Delete(LineHandle[i]);
        }

    }
    if (OrigHandle != 0) { m_Painter3D->Delete(OrigHandle); }
    if (TargetHandle != 0) { m_Painter3D->Delete(TargetHandle); }
    if (CenterHandle != 0) { m_Painter3D->Delete(CenterHandle); }
    if (LineHandle != 0) { m_Painter3D->Delete(LineHandle); }
}

void ResampleToLineStyle::LeftButtonMouseMove(IEvent _event) {
    auto pos = _event.pos;
    if (Selected == 0) {
        //center
        
        
        igm::vec pos1 = igm::vec2(pos.x, 0);
        igm::vec pos2 = igm::vec2(pos.x, m_Interactor->GetHeight());

        //视锥平面
        igm::vec3 p1 = GetNearWorldCoord(pos1, IntertedMVP);
        igm::vec3 p2 = GetFarWorldCoord(pos1, InvertedMVP);
        igm::vec3 p3 = GetNearWorldCoord(pos2, InvertedMVP);
        igm::vec3 intersection;

        //计算平面与视锥的交点
        LinePlaneIntersection(Origin, Target, p1, p2, p3, intersection);
        //if (!m_DataObject->GetBoundingBox().isIn(V(intersection))) { return; }

        Center = intersection;
        Origin = Center + Center2Start;
        Target = Center + Center2End;

        ComputeResampleToLine();
        Draw();
        if (IsPreview()) Emit();
    
    } else if (Selected == 1) {
        //head
        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, NDC_Z, 1.f};
        igm::vec4 newPoint_WorldCoord = InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;

        Start = newPoint_WorldCoord.xyz();
        igm::vec3 Direction = -(Start - Center).normalized();
        End = Center + Direction * Center2End.length();

        ComputeSlicingPlane();
        Draw();
        if (IsPreview()) Emit();
    } else if (Selected == 2) {

        igm::vec2 NDC(2.0f * pos.x / m_Interactor->GetWidth() - 1.0f,
                      1.0f - (2.0f * pos.y / m_Interactor->GetHeight()));

        igm::vec4 Point_NDC{NDC, NDC_Z, 1.f};
        igm::vec4 newPoint_WorldCoord = InvertedMVP * Point_NDC;
        newPoint_WorldCoord /= newPoint_WorldCoord.w;

        End = newPoint_WorldCoord.xyz();
        igm::vec3 Direction = -(End - Center).normalized();
        Start = Center + Direction * Center2Start.length();

        ComputeSlicingPlane();
        Draw();
        if (IsPreview()) Emit();

    } else if (Selected == 3) {
        return;

        igm::vec2 pos1 = igm::vec2(pos.x, 0);
        igm::vec2 pos2 = igm::vec2(pos.x, m_Interactor->GetHeight());

        // p1,p2,p3 组成视锥平面 form the cone plane
        igm::vec3 p1 = GetNearWorldCoord(pos1, InvertedMVP);
        igm::vec3 p2 = GetFarWorldCoord(pos1, InvertedMVP);
        igm::vec3 p3 = GetNearWorldCoord(pos2, InvertedMVP);
        igm::vec3 intersection;

        // 计算直线与与视锥平面的交点 Calculate the intersection of the line with the cone plane
        LinePlaneIntersection(Start, End, p1, p2, p3, intersection);
        igm::vec3 Vector = intersection - Intersection;
        if (!m_DataObject->GetBoundingBox().isIn(V(TempCenter + Vector)))
            return;

        Center = TempCenter + Vector;
        Start = TempStart + Vector;
        End = TempEnd + Vector;

        ComputeSlicingPlane();
        Draw();
        if (IsPreview()) Emit();
    }
}

void ResampleToLineStyle::Draw() {
    if (OrigHandle != 0) m_Painter3D->Delete(OrigHandle);
    if (CenterHandle != 0) m_Painter3D->Delete(CenterHandle);
    if (TargetHandle != 0) m_Painter3D->Delete(TargetHandle);
    if (LineHandle != 0) m_Painter3D->Delete(LineHandle);

    m_Painter3D->SetPen(16);
    if (Selected == -1) {
        m_Painter3D->SetPen(Color::Red);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
        StartHandle = m_Painter3D->DrawPoint(V(Start));
        EndHandle = m_Painter3D->DrawPoint(V(End));
    } else if (Selected == 0) {
        m_Painter3D->SetPen(Color::Green);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
        m_Painter3D->SetPen(Color::Red);
        StartHandle = m_Painter3D->DrawPoint(V(Start));
        EndHandle = m_Painter3D->DrawPoint(V(End));
    } else if (Selected == 1) {
        m_Painter3D->SetPen(Color::Green);
        StartHandle = m_Painter3D->DrawPoint(V(Start));

        m_Painter3D->SetPen(Color::Red);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
        EndHandle = m_Painter3D->DrawPoint(V(End));
    } else if (Selected == 2) {
        m_Painter3D->SetPen(Color::Green);
        EndHandle = m_Painter3D->DrawPoint(V(End));
        m_Painter3D->SetPen(Color::Red);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
        StartHandle = m_Painter3D->DrawPoint(V(Start));
    } else if (Selected == 3) {
        m_Painter3D->SetPen(Color::Red);
        CenterHandle = m_Painter3D->DrawPoint(V(Center));
        StartHandle = m_Painter3D->DrawPoint(V(Start));
        EndHandle = m_Painter3D->DrawPoint(V(End));

        m_Painter3D->SetPen(2);
        m_Painter3D->SetPen(Color::Green);
        LineHandle = m_Painter3D->DrawLine(V(Start), V(End));
    }

    if (Selected != 3) {
        m_Painter3D->SetPen(2);
        m_Painter3D->SetPen(Color::Red);
        LineHandle = m_Painter3D->DrawLine(V(Start), V(End));
    }


    DrawLine();
}