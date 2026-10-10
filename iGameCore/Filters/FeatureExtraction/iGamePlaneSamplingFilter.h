#pragma once
#ifndef iGamePlaneSamplingFilter_h
#define iGamePlaneSamplingFilter_h

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameFilter.h"
#include "iGameFlatArray.h"
#include "iGamePointFinder.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class PlaneSamplingFilter : public Filter {
public:
    I_OBJECT(PlaneSamplingFilter);
    static Pointer New() { return new PlaneSamplingFilter; }

    PlaneSamplingFilter() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);

        m_Origin[0] = 0.0;
        m_Origin[1] = 0.0;
        m_Origin[2] = 0.0;
        m_Normal[0] = 0.0;
        m_Normal[1] = 0.0;
        m_Normal[2] = 1.0;
        m_U[0] = 1.0;
        m_U[1] = 0.0;
        m_U[2] = 0.0;
        m_V[0] = 0.0;
        m_V[1] = 1.0;
        m_V[2] = 0.0;
        m_Resolution = 20;
        m_AttributeName = "";
        m_ComponentIndex = 0;
        m_HalfRange = 1.0;
    }

    ~PlaneSamplingFilter() override = default;

    bool Execute() override;

    void SetPlaneOrigin(double ox, double oy, double oz);
    void SetPlaneOrigin(const double origin[3]);
    void SetPlaneNormal(double nx, double ny, double nz);
    void SetPlaneNormal(const double normal[3]);
    void SetResolution(int res);
    void SetAttributeName(const std::string& name) { m_AttributeName = name; }
    void SetComponentIndex(int index) { m_ComponentIndex = index; }

    void GetPlaneOrigin(double origin[3]) const;
    void GetPlaneNormal(double normal[3]) const;
    int GetResolution() const { return m_Resolution; }
    int GetComponentIndex() const { return m_ComponentIndex; }

private:
    double m_Origin[3];
    double m_Normal[3];
    int m_Resolution;
    std::string m_AttributeName;
    int m_ComponentIndex;
    double m_U[3];
    double m_V[3];
    double m_HalfRange;
};

IGAME_NAMESPACE_END
#endif