#include "iGameCellMeshMetricsFilter.h"

IGAME_NAMESPACE_BEGIN

CellMeshMetricsFilter::CellMeshMetricsFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    input = nullptr;
    output = nullptr;
}

bool CellMeshMetricsFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    input = m_Inputs->GetElement(0);
    return Execute(input);
}

bool CellMeshMetricsFilter::Execute(DataObject::Pointer input) {
    if (!input) { return false; }
    this->output = DataObject::New();
    return Execute(input, output);
}

bool CellMeshMetricsFilter::Execute(DataObject::Pointer input, DataObject::Pointer& output) {
    if (!input) { return false; }
    bool success = ComputeCellMetrics(input, output);
    if (success && output) {
        this->SetOutput(0, output);
        return true;
    }
    return false;
}

void CellMeshMetricsFilter::setMetric(VolumeMeshMetricsFilter::VolumeMetric metric) { m_Metric = metric; }

bool CellMeshMetricsFilter::ComputeCellMetrics(DataObject::Pointer input, DataObject::Pointer& output) {
    if (!input) { return false; }

    // Branch node: recursive traversal
    if (input->HasSubDataObject()) {
        DataObject::Pointer outputContainer = DataObject::New();
        outputContainer->SetName(input->GetName()); // Preserve name hierarchy

        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it) {
            DataObject::Pointer subOutput;
            if (ComputeCellMetrics(it->second, subOutput)) {
                if (subOutput) { outputContainer->AddSubDataObject(subOutput); }
            }
        }
        output = outputContainer;
        return true;
    }
    // Leaf node: compute volume cell quality metrics
    else {
        auto volumeFilter = VolumeMeshMetricsFilter::New();
        if (m_Metric == VolumeMeshMetricsFilter::INVALID) {
            std::cout << "ERROR::CellMeshMetrics: set m_Metric before use" << std::endl;
            return false;
        } else {
            volumeFilter->SetVolumeMetric(m_Metric);
        }
        volumeFilter->SetInput(input);

        if (volumeFilter->Execute()) {
            output = volumeFilter->GetOutput(0);
            if (output) {
                output->SetName(input->GetName());
                return true;
            }
        }
        return false;
    }
}

IGAME_NAMESPACE_END