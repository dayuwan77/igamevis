#include "Selection/iGameExtractBlockFilter.h"
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace iGame;

static void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        std::filesystem::create_directories("ExtractionResults");
        for (const auto* file : {"ExtractBlockFlat.vtm", "ExtractBlockNested.vtm"}) {
            auto input = FileIO::ReadFile(std::string("Examples/Models/") + file);
            Check(input != nullptr, "read multiblock fixture");
            auto filter = ExtractBlockFilter::New();
            filter->SetInput(input);
            if (std::string(file) == "ExtractBlockFlat.vtm") filter->SetBlockIndex(1);
            else filter->SetBlockPath({1, 0});
            if (!filter->Execute()) throw std::runtime_error(filter->GetLastError());
            auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            Check(output && output->GetNumberOfPoints() == 5 && output->GetNumberOfCells() == 2,
                  "tetrahedron block counts");
            Check(output->GetCellType(0) == IG_TETRA && output->GetCellType(1) == IG_TETRA,
                  "tetrahedron block types");
            Check(FileIO::WriteFile(std::string("ExtractionResults/") + file + ".vtk", output),
                  "write block result");
            filter->SetBlockName("ExtractSelectionTetrahedra");
            if (!filter->Execute()) throw std::runtime_error(filter->GetLastError());
            auto named = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            Check(named && named->GetNumberOfCells() == 2, "name lookup");
            Check(ExtractBlockFilter::GetBlocks(input).size() == 2, "input hierarchy unchanged");
            std::cout << file << ": index/path and name extraction passed; 5 points, 2 cells\n";
        }
        std::cout << "BlockExtractionExamples: all cases passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "BlockExtractionExamples FAILED: " << error.what() << '\n';
        return 1;
    }
}
