#include <DataProcessing/iGameMeshTetrahedralize.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iostream>
#include <sstream>

int main() {
    const std::string fileName = "./Models/VolumeSimplification_FlangedTube.vtk";
    auto scene = iGame::Scene::New();
    auto input = iGame::FileIO::ReadFile(fileName);

    if (input == nullptr) {
        std::cerr << "Falied to read input File:" << fileName << std::endl;
        return 1;
    }

    auto tetraFilter = iGame::MeshTetrahedralize::New();
    tetraFilter->SetInput(input);
    if (!tetraFilter->Execute()) {
        std::cerr << "Failed to tetrahedralize mesh: "
                  << tetraFilter->m_failReason << std::endl;
        return 1;
    }

    auto output = tetraFilter->GetOutput();
    if (output == nullptr) {
        std::cerr << "Failed to get MeshTetrahedralize output." << std::endl;
        return 1;
    }

    auto inputPointSet = DynamicCast<iGame::PointSet>(input);
    auto outputMesh = DynamicCast<iGame::VolumeMesh>(output);
    if (inputPointSet == nullptr || outputMesh == nullptr) {
        std::cerr << "Input is not a point set or output is not a volume mesh." << std::endl;
        return 1;
    }

    IGsize inputCellCount = 0;
    if (auto inputMesh = DynamicCast<iGame::UnstructuredMesh>(input)) {
        inputCellCount = inputMesh->GetNumberOfCells();
    } else if (auto inputMesh = DynamicCast<iGame::VolumeMesh>(input)) {
        inputCellCount = inputMesh->GetNumberOfVolumes();
    } else {
        std::cerr << "Unsupported input mesh type." << std::endl;
        return 1;
    }

    IGsize tetraCount = 0;
    IGsize nonTetraCount = 0;
    for (IGsize i = 0; i < outputMesh->GetNumberOfVolumes(); ++i) {
        if (outputMesh->GetVolume(i)->GetCellType() == iGame::IG_TETRA) {
            ++tetraCount;
        } else {
            ++nonTetraCount;
        }
    }

    std::cout << "Input : points=" << inputPointSet->GetNumberOfPoints()
              << ", cells=" << inputCellCount << '\n'
              << "Output: points=" << outputMesh->GetNumberOfPoints()
              << ", volumes=" << outputMesh->GetNumberOfVolumes() << '\n'
              << "Tetra cells=" << tetraCount
              << ", non-tetra cells=" << nonTetraCount << std::endl;

    if (nonTetraCount != 0) {
        std::cerr << "FAIL: output contains non-tetrahedral cells." << std::endl;
        return 1;
    }
    std::cout << "PASS: all output cells are tetrahedra." << std::endl;

    scene->AddModel(outputMesh);

    auto drawObject = DynamicCast<iGame::DrawObject>(outputMesh);

    if (drawObject == nullptr) {
        std::cerr << "Output is not drawable." << std::endl;
        return 1;
    }

    drawObject->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    drawObject->ViewCloudPicture(scene, 0, 0);

    std::ostringstream info;
    info << "Mesh Tetrahedralize\n"
         << "Input points: " << inputPointSet->GetNumberOfPoints() << '\n'
         << "Input cells: " << inputCellCount << '\n'
         << "Output points: " << outputMesh->GetNumberOfPoints() << '\n'
         << "Output volumes: " << outputMesh->GetNumberOfVolumes() << '\n'
         << "Tetra cells: " << tetraCount << '\n'
         << "Non-tetra cells: " << nonTetraCount;
    scene->SetCornerAnnotationText(info.str());
    scene->SetCornerAnnotationPosition(20.0f, 20.0f);
    scene->SetCornerAnnotationVisible(true);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();

    window->SetInteractor(interactor);

    scene->ResetCameraView();
    window->Show();
    return 0;
}
