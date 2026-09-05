#ifndef iGameVolumeOfRevolution_h
#define iGameVolumeOfRevolution_h

#include "iGameDataObject.h"
#include "iGameFilter.h"

IGAME_NAMESPACE_BEGIN

struct Edge {
    IGsize v0, v1;
    IGsize cellId;
};
struct PointProjection {
    double r, h;
    Vector3d v_perp;
};
class VolumeOfRevolutionFilter : public Filter {
public:
    I_OBJECT(VolumeOfRevolutionFilter);
    static Pointer New() { return new VolumeOfRevolutionFilter; }

    //void SetAxis(RevolutionAxis axis) { m_Axis = axis; }
    void SetAxis(const Vector3d& dir, const Vector3d& point) {
        m_AxisDirection = dir;
        m_AxisPoint = point;
    }
    void SetResolution(int res) { m_Resolution = (res < 3) ? 3 : res; }
    void SetAngle(double angle) { m_Angle = angle; }

    bool Execute() override;

protected:
    VolumeOfRevolutionFilter() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~VolumeOfRevolutionFilter() override = default;

private:
    int m_Resolution = 36;
    double m_Angle = 2.0 * 3.141592653589793;
    Vector3d m_AxisDirection; // 轴向方向，默认 (0,0,1)
    Vector3d m_AxisPoint;     // 轴上一点，默认 (0,0,0)
};

IGAME_NAMESPACE_END

#endif