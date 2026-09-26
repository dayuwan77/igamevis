#include "FeatureExtraction/iGameCountCellFacesFilter.h"
#include "iGameFileIO.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: testCountCellFaces <input-model>\n";
        return 1;
    }

    const std::string inputFile = argv[1];
    auto input = iGame::FileIO::ReadFile(inputFile);
    if (input.IsNull()) {
        std::cerr << "Failed to read input model: " << inputFile << '\n';
        return 2;
    }

    auto filter = iGame::CountCellFacesFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) {
        std::cerr << "CountCellFacesFilter failed: " << filter->GetMessage() << '\n';
        return 3;
    }

    const auto faceCounts = filter->GetResult();
    if (faceCounts.IsNull()) {
        std::cerr << "CountCellFacesFilter returned no result.\n";
        return 4;
    }

    std::cout << "Input: " << inputFile << '\n';
    std::cout << "Attribute: " << iGame::CountCellFacesFilter::ResultAttributeName << '\n';
    std::cout << "Number of cells: " << faceCounts->GetNumberOfValues() << '\n';
    for (IGsize cellId = 0; cellId < faceCounts->GetNumberOfValues(); ++cellId) {
        std::cout << "cell[" << cellId << "] = " << faceCounts->GetValue(cellId) << '\n';
    }

    return 0;
}
