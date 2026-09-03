#include "iGameMultiBlockGeometryFilter.h"

IGAME_NAMESPACE_BEGIN

MultiBlockGeometryFilter::MultiBlockGeometryFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

bool MultiBlockGeometryFilter::Execute() {
    input = this->GetInput(0);
    if (!input) { return false; }
    return Execute(this->input);
}

bool MultiBlockGeometryFilter::Execute(DataObject::Pointer input) {
    if (!input) { return false; }
    this->output = DataObject::New();
    return Execute(input, this->output);
}

bool MultiBlockGeometryFilter::Execute(DataObject::Pointer input, DataObject::Pointer& output) {
    if (!input) { return false; }
    bool success = ExtractRecursively(input, output);
    if (success && output) {
        this->SetOutput(0, output);
        return true;
    }
    return false;
}

bool MultiBlockGeometryFilter::ExtractRecursively(DataObject::Pointer input, DataObject::Pointer& output) {
    // Defensive check
    if (!input) {
        output = nullptr;
        return false;
    }

    // Branch node: recursive traversal
    if (input->HasSubDataObject()) {
        // 这里必须用DrawObject，因为dataObject传入scene会被强转成DrawObject
        DrawObject::Pointer outContainer = DrawObject::New();
        outContainer->SetName(input->GetName()); // Preserve name hierarchy

        for (auto it = input->SubDataObjectIteratorBegin(); it != input->SubDataObjectIteratorEnd(); ++it) {
            DataObject::Pointer subOutput;
            if (ExtractRecursively(it->second, subOutput)) {
                if (subOutput) { outContainer->AddSubDataObject(subOutput); }
            }
        }
        
        output = outContainer;
        if (!output) { return false; }
        return true;
    }
    // Leaf node: extract surface using ModelGeometryFilter
    else {
        auto modelFilter = ModelGeometryFilter::New();
        modelFilter->SetInput(input);

        if (modelFilter->Execute()) {
            output = modelFilter->GetOutput();
            if (output) {
                output->SetName(input->GetName());
                return true;
            }
        }
        return false;
    }
}

IGAME_NAMESPACE_END