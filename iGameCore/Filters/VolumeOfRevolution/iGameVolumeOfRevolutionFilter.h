#ifndef iGameVolumeOfRevolution_h
#define iGameVolumeOfRevolution_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include <string>

IGAME_NAMESPACE_BEGIN

struct RevolutionEdge {
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

    // 执行失败的原因（输入不符合条件时给出可读说明，供界面/调用方展示）。
    // 空字符串表示上一次执行没有可报告的失败原因。
    const std::string& GetMessage() const { return m_Message; }

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

    // 记录非致命提示与致命失败原因（与 iGameLinearExtrusionFilter 的同类约定一致）
    std::string m_Message;
};

IGAME_NAMESPACE_END

#endif