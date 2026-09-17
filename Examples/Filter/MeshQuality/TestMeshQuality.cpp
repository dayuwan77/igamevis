#include <MeshQuality/iGameMeshQualityFilter.h>
#include <iGameFileIO.h>

#include <iostream>
#include <iomanip>

int main() {

    const std::string fileName = "./Models/Convert_Quad_Bicycle.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);

    if (obj == nullptr) {
        std::cout << "Read ERROR!" << std::endl;
        return 1;
    }

    auto filter = iGame::MeshQualityFilter::New();
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "MeshQualityFilter Execute ERROR!" << std::endl;
        return 1;
    }

    std::cout << std::fixed << std::setprecision(15);
    std::cout << "Mesh Quality Result" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "NumberOfCells: "<< filter->GetNumberOfCells() << std::endl;
    std::cout << "Minimum: "<< filter->GetMinimum() << std::endl;
    std::cout << "Maximum: "<< filter->GetMaximum() << std::endl;
    std::cout << "Average: "<< filter->GetAverage() << std::endl;

    return 0;
}