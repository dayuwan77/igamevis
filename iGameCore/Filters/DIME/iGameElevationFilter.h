/**
 * @class   ElevationFilter
 * @brief   Generate a scalar field ("Elevation") from point coordinates.
 *          The value on the chosen axis is affinely mapped from its source
 *          range [vMin, vMax] onto the user-specified output range
 *          [Low, High] (defaults to [0, 1]).
 * @note    DIME Filter #19 (owner: Wang Yilin). Input: any PointSet-derived
 *          mesh. Output: same mesh with a new IG_SCALAR / IG_POINT attribute.
 */
#ifndef iGameElevationFilter_h
#define iGameElevationFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN

class ElevationFilter : public Filter {
    I_OBJECT(ElevationFilter)
public:
    static Pointer New() { return new ElevationFilter; }

    bool Execute() override;

    // Axis along which the elevation value is sampled.
    enum class Axis : int { X = 0, Y = 1, Z = 2 };

    void SetAxis(Axis axis) { m_Axis = axis; }
    Axis GetAxis() const { return m_Axis; }

    // Destination range of the affine mapping ([0, 1] by default).
    void SetRange(float low, float high) { m_Low = low; m_High = high; }
    float GetLow() const { return m_Low; }
    float GetHigh() const { return m_High; }

    // Name of the generated scalar array.
    void SetArrayName(const std::string& name) { m_ArrayName = name; }
    const std::string& GetArrayName() const { return m_ArrayName; }

protected:
    ElevationFilter()
        : m_Axis(Axis::Z), m_Low(0.0f), m_High(1.0f), m_ArrayName("Elevation") {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~ElevationFilter() override = default;

private:
    // Component index of the sampled coordinate.
    int AxisIndex() const { return static_cast<int>(m_Axis); }

    Axis m_Axis;
    float m_Low;
    float m_High;
    std::string m_ArrayName;
};

IGAME_NAMESPACE_END
#endif
