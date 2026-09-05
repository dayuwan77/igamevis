#ifndef iGameAngularPeriodicFilter_h
#define iGameAngularPeriodicFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

// 角度周期复制过滤器：把一个网格绕指定轴旋转复制 N 份（角度周期），
// 输出一个合并后的 UnstructuredMesh（包含原始网格 + 各旋转副本）。
class AngularPeriodicFilter : public Filter {
public:
    I_OBJECT(AngularPeriodicFilter);
    static Pointer New() { return new AngularPeriodicFilter; }

    // 设置旋转轴：过点 origin，方向 axis（自动归一化）
    void SetRotationAxis(const Point& origin, const Vector3d& axis);
    // 设置份数（含原始网格，即 total copies）
    void SetNumberOfCopies(int n) { m_NumberOfCopies = n; }
    // 设置旋转总角度（度），按份数均分：第 i 份旋转 angle*i/copies 度
    void SetAngle(float angle) { m_Angle = angle; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }

private:
    // 把点 p 绕轴（m_AxisOrigin, m_AxisNormalized）旋转 angleRad 弧度
    Point RotatePoint(const Point& p, float angleRad);

protected:
    AngularPeriodicFilter();
    ~AngularPeriodicFilter() override = default;

    Point m_AxisOrigin{0.f, 0.f, 0.f};
    Vector3d m_AxisNormalized{0.0, 0.0, 1.0};
    int m_NumberOfCopies{2};
    float m_Angle{360.0f};

    std::string m_Message{""};
};

IGAME_NAMESPACE_END
#endif
