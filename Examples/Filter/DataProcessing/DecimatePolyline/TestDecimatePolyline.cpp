#include <DataProcessing/iGameDecimatePolylineFilter.h>

#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <vector>

int main() {
    const std::string fileName = "Models/DecimatePolyline_Curve.vtk";
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input || !iGame::DecimatePolylineFilter::CanProcessInput(input)) {
        std::cerr << "Failed to read a supported polyline: " << fileName << '\n';
        return 1;
    }

    auto filter = iGame::DecimatePolylineFilter::New();
    filter->SetInput(input);
    filter->SetTargetReduction(0.9);
    if (!filter->Execute()) {
        std::cerr << "DecimatePolylineFilter execution failed.\n";
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!output || output->GetNumberOfPoints() != 15 || output->GetNumberOfCells() != 1 ||
        output->GetCellType(0) != iGame::IG_POLY_LINE) {
        std::cerr << "Unexpected output topology.\n";
        return 1;
    }

    const auto& attribute = output->GetAttributeSet()->GetAttribute("OriginalPointId");
    auto originalIds = iGame::DynamicCast<iGame::IntArray>(attribute.pointer);
    const std::vector<int> paraViewPointIds{
            0, 10, 27, 37, 42, 56, 63, 73, 87, 92, 108, 123, 139, 144, 150};
    if (!originalIds || originalIds->GetNumberOfElements() != paraViewPointIds.size()) {
        std::cerr << "OriginalPointId was not copied correctly.\n";
        return 1;
    }

    for (IGsize i = 0; i < paraViewPointIds.size(); ++i) {
        if (originalIds->GetValue(i) != paraViewPointIds[i]) {
            std::cerr << "Result differs from ParaView at output point " << i << ".\n";
            return 1;
        }
    }

    auto chainedFilter = iGame::DecimatePolylineFilter::New();
    chainedFilter->SetInput(output);
    chainedFilter->SetTargetReduction(0.5);
    if (!iGame::DecimatePolylineFilter::CanProcessInput(output) || !chainedFilter->Execute()) {
        std::cerr << "The filter cannot process its own output.\n";
        return 1;
    }

    auto chainedOutput = iGame::DynamicCast<iGame::UnstructuredMesh>(chainedFilter->GetOutput());
    if (!chainedOutput || chainedOutput->GetNumberOfPoints() != 7 ||
        chainedOutput->GetNumberOfCells() != 1 ||
        chainedOutput->GetCellType(0) != iGame::IG_POLY_LINE) {
        std::cerr << "Unexpected chained output topology.\n";
        return 1;
    }

    std::cout << "DecimatePolyline: 151 points -> " << output->GetNumberOfPoints()
              << " -> " << chainedOutput->GetNumberOfPoints()
              << " points; ParaView reference matched and output is reusable.\n";
    return 0;
}
