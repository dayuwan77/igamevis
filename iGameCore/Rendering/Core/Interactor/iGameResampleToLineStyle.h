#ifndef IGAMEVIS_RESAMPLETOLINE_STYLE_H
#define IGAMEVIS_RESAMPLETOLINE_STYLE_H

#include "iGameBasicStyle.h"

#include "iGameSelection.h"
IGAME_NAMESPACE_BEGIN
class Model;
class DataObject;
class Patinter3D;

class ResampleToLineStyle : public BasicStyle {
public:
    I_OBJECT(ResampleToLineStyle);
    static Pointer New() { return new ResampleToLineStyle; }

    void Initialize(SmartPointer<Interactor> interactor,
                    SmartPointer<Selection> s);
    void MousePressEvent(IEvent _event) override;
    void MouseMoveEvent(IEvent _event) override;
    void MouseReleaseEvent(IEvent _event) override;

protected:
    ResampleToLineStyle();
    ~ResampleToLineStyle() override;

    void LeftButtonMouseMove(IEvent _event)
        ;
    virtual void RightButtonMouseMove() override;
    virtual void MiddleButtonMouseMove() override;

    void Draw();

    void ComputeResampleToLine();
    void DrawLine();

    void UpdateLine();

    void Emit();

    SmartPointer<Model> m_Model;
    SmartPointer<DataObject> m_DataObject;
    SmartPointer<Painter3D> m_Painter3D;
    SmartPointer<ClipSelection> m_Selection;

private:
    Vector3Tovec3 v;
    vec3ToVector3d V;

    igm::vec3 Origin, Target, Center;
    igm::vec3 Center2Start, Center2End;
    igm::vec3 Intersection, TempCenter, TempOrigin, TempTarget;
    IGuint OrigHandle{}, TargetHandle{}, CenterHandle{};
    IGuint LineHandle{};
    int Selected = -1;// 0:center 1:head 2:rear 3:lines

    float NDC_Z{};
    float PickRadius{};

    igm::mat4 MVP;
    igm::mat4 InvertedMVP;
    std::vector<Vector3d> Plane;


};