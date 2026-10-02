#ifndef iGameIntegrateVariablesFilter_h
#define iGameIntegrateVariablesFilter_h

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * Integrate point and cell attributes over the highest-dimensional cells.
 *
 * The filter follows ParaView/vtkIntegrateAttributes semantics:
 * - only the highest cell dimension present is integrated;
 * - point attributes are integrated with linear interpolation;
 * - cell attributes are multiplied by Length, Area, or Volume;
 * - the output is one vertex located at the measure-weighted centroid;
 * - point and cell results keep their original associations and are stored as
 *   double arrays with one tuple.
 *
 * CellSizeFilter is reused for supported-cell discovery and final Length/Area
 * measurements. A local line path covers legacy POLYDATA LINES, which iGame
 * stores as SurfaceMesh edges outside CellSize's face-based input. Volume
 * integration always uses ParaView's signed linear-cell decomposition,
 * preserving VTK semantics without changing CellSize.
 */
class IntegrateVariablesFilter : public Filter {
public:
    I_OBJECT(IntegrateVariablesFilter);
    static Pointer New() { return new IntegrateVariablesFilter; }

    bool Execute() override;

    void SetDivideAllCellDataByMeasure(bool value) {
        m_DivideAllCellDataByMeasure = value;
    }
    bool GetDivideAllCellDataByMeasure() const {
        return m_DivideAllCellDataByMeasure;
    }

    std::string GetMessage() const { return m_Message; }
    int GetIntegrationDimension() const { return m_IntegrationDimension; }
    int GetIntegratedCellCount() const { return m_IntegratedCellCount; }
    double GetIntegratedMeasure() const { return m_IntegratedMeasure; }
    std::string GetMeasureName() const;

protected:
    IntegrateVariablesFilter();
    ~IntegrateVariablesFilter() override = default;

private:
    bool m_DivideAllCellDataByMeasure{false};
    std::string m_Message;
    int m_IntegrationDimension{0};
    int m_IntegratedCellCount{0};
    double m_IntegratedMeasure{0.0};
};

IGAME_NAMESPACE_END

#endif
