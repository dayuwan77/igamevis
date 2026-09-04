#ifndef iGamePointVolumeInterpolatorFilter_h
#define iGamePointVolumeInterpolatorFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameFlatArray.h"

IGAME_NAMESPACE_BEGIN

// 体数据插值过滤器：对一组查询点在体网格上做插值，输出带插值属性的 PointSet。
class PointVolumeInterpolatorFilter : public Filter {
public:
    I_OBJECT(PointVolumeInterpolatorFilter);
    static Pointer New() { return new PointVolumeInterpolatorFilter; }

    void SetAttributeByIndex(int index) { m_AttributeIndex = index; }
    void SetAttributeByName(const std::string& name) { m_AttributeName = name; }

    // 输出点集上命中掩码属性的属性名：1=查询点命中体单元，0=未命中
    static constexpr const char* HitMaskName = "HitMask";

    bool Execute() override;

private:
    bool ComputeBarycentric(const Point& p, Cell* cell, std::vector<double>& weights);
    void InterpolateAttribute(const std::vector<double>& weights, ArrayObject* data, Cell* cell, float* result);

protected:
    PointVolumeInterpolatorFilter();
    ~PointVolumeInterpolatorFilter() override = default;

    int m_AttributeIndex{-1};
    std::string m_AttributeName{""};
};

IGAME_NAMESPACE_END
#endif
