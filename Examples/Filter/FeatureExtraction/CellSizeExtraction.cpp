#pragma once

#include "CellSize/iGameCellSizeFilter.h"
#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"

#include <iomanip>
#include <iostream>
#include <string>

// CellSize command-line test program
// Run: testCellSizeExtraction [model file path]  (prompts for input if no argument)
// Prints the first 10 values of each per-cell Length(1D)/Area(2D)/Volume(3D) attribute to stdout
// Return code: 0=success 1=read/compute failed

namespace {

void PrintAttrStats(iGame::DataObject* data) {
    auto attrSet = data->GetAttributeSet();
    auto attrs = attrSet->GetAllAttributes();
    const char* names[] = {"Length", "Area", "Volume"};
    for (const char* name : names) {
        int idx = attrSet->GetAttributeIndex(name);
        if (idx < 0) {
            std::cout << "[CellSize] attribute=\"" << name << "\" NOT FOUND\n";
            continue;
        }
        auto& attr = attrs->GetElement(idx);
        auto arr = attr.pointer.GetPointer();
        size_t n = arr->GetNumberOfElements();
        size_t show = n < 10 ? n : 10;
        std::cout << "[CellSize] attribute=\"" << name << "\" first " << show << " values:";
        for (size_t i = 0; i < show; ++i) {
            std::cout << " " << arr->GetValue(i);
        }
        std::cout << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string fileName;
    if (argc >= 2) {
        fileName = argv[1];
    }
    // Prompt for input when no command-line argument is provided
    if (fileName.empty()) {
        std::cout << "Please enter the model file path: ";
        std::getline(std::cin, fileName);
    }
    if (fileName.empty()) {
        std::cerr << "[CellSize] no model path provided\n";
        return 1;
    }

    // Read the file
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (!dataObj) {
        std::cerr << "[CellSize] failed to read file: " << fileName << "\n";
        return 1;
    }

    // Execute CellSizeFilter
    iGame::CellSizeFilter::Pointer filter = iGame::CellSizeFilter::New();
    filter->SetInput(dataObj);
    if (!filter->Execute()) {
        std::string msg = filter->GetMessage();
        if (msg.empty()) msg = "execute failed";
        std::cerr << "[CellSize] " << msg << "\n";
        return 1;
    }

    // Print results
    std::cout << "[CellSize] model=" << fileName << "\n";
    PrintAttrStats(dataObj);
    std::cout << "[CellSize] done\n";
    return 0;
}
