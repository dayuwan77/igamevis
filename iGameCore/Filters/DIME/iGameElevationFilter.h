/**
 * @class   ElevationFilter
 * @brief   高程标量场：把点坐标沿指定方向的投影线性映射到输出区间。
 *          h = p · d（点积投影），再从 [hMin, hMax] 仿射映射到 [Low, High]。
 *          方向不必是单位向量（缩放不影响结果），但不能是零向量。
 * @note    DIME Filter #19（负责人：王奕霖）。输入：任意 PointSet 派生网格。
 *          输出：同一网格 + 新增 IG_SCALAR / IG_POINT 属性。
 */
#ifndef iGameElevationFilter_h
#define iGameElevationFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN

class ElevationFilter : public Filter {
public:
    // I_OBJECT 必须位于 public 区，否则 Pointer 等类型别名为私有，外部无法使用
    I_OBJECT(ElevationFilter)
    static Pointer New() { return new ElevationFilter; }

    bool Execute() override;

    // 投影方向（默认 (0,0,1) 即 Z 轴）。零向量被拒绝并保持原值。
    bool SetDirection(float dx, float dy, float dz);
    bool SetDirection(const Vector3f& d) { return SetDirection(d[0], d[1], d[2]); }
    const Vector3f& GetDirection() const { return m_Direction; }

    // 轴向便捷接口：内部转为对应的单位基向量。
    enum class Axis : int { X = 0, Y = 1, Z = 2 };
    void SetAxis(Axis axis);

    // 输出区间（默认 [0, 1]）。
    void SetRange(float low, float high) { m_Low = low; m_High = high; }
    float GetLow() const { return m_Low; }
    float GetHigh() const { return m_High; }

    // 生成的标量数组名。
    void SetArrayName(const std::string& name) { m_ArrayName = name; }
    const std::string& GetArrayName() const { return m_ArrayName; }

protected:
    ElevationFilter()
        : m_Direction(0.f, 0.f, 1.f), m_Low(0.0f), m_High(1.0f),
          m_ArrayName("Elevation") {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~ElevationFilter() override = default;

private:
    Vector3f m_Direction;  // 投影方向
    float m_Low;           // 输出区间下限
    float m_High;          // 输出区间上限
    std::string m_ArrayName;
};

IGAME_NAMESPACE_END
#endif
