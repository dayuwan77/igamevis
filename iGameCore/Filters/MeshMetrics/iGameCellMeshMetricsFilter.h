#ifndef iGameCellMeshMetrics_h
#define iGameCellMeshMetrics_h

#include "iGameFilter.h"
#include "iGameVolumeMeshMetricsFilter.h"

IGAME_NAMESPACE_BEGIN
class CellMeshMetricsFilter : public Filter {
public:
    I_OBJECT(CellMeshMetricsFilter);
    static Pointer New() { return new CellMeshMetricsFilter; }
    bool Execute() override;
    bool Execute(DataObject::Pointer);
    bool Execute(DataObject::Pointer, DataObject::Pointer&);
    void setMetric(VolumeMeshMetricsFilter::VolumeMetric metric);

protected:
    VolumeMeshMetricsFilter::VolumeMetric m_Metric = VolumeMeshMetricsFilter::TET_EDGE_RATIO;
    DataObject::Pointer input;
    DataObject::Pointer output;
    CellMeshMetricsFilter();
    ~CellMeshMetricsFilter() override = default;

    bool ComputeCellMetrics(DataObject::Pointer, DataObject::Pointer&);
};

IGAME_NAMESPACE_END

#endif // !iGameCellMeshMetrics_h
