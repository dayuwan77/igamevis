#include <Core/iGameScene.h>
#include <ModelSurface/iGameMultiBlockGeometryFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameCellType.h>
#include <iostream>

int main() {
    std::cout << "[Examples] Starting MultiBlockGeometryFilter Test Case...\n";

    /* 1. Create Scene */
    auto scene = iGame::Scene::New();

    /* 2. Read Model File (supports .ex2, .vtu, .vtm, .vtk, etc.) */
    const std::string fileName = "./Models/multiblock_test.vtm";
    iGame::DataObject::Pointer root = iGame::FileIO::ReadFile(fileName);

    /* 3. Instantiate MultiBlock Geometry Filter */
    auto filter = iGame::MultiBlockGeometryFilter::New();
    filter->SetInput(root);

    /* 4. Execute Extraction Algorithm */
    if (!filter->Execute()) {
        std::cerr << "[Error] MultiBlockGeometryFilter execution failed!\n";
        return -1;
    }

    /* 5. Fetch Output Dataset */
    auto res = filter->GetOutput();
    if (res == nullptr) {
        std::cerr << "[Error] Output dataset is null!\n";
        return -1;
    }

    std::cout << "[Success] Surface extraction succeeded!\n";

    /* 6. Add Extracted Surface to Scene (Supports both Multi-block and Single-block) */
    if (res->HasSubDataObject()) {
        std::cout << "   -> Extracted Sub-block Count: " << res->GetNumberOfSubDataObjects() << std::endl;
        for (auto it = res->SubDataObjectIteratorBegin(); it != res->SubDataObjectIteratorEnd(); ++it) {
            auto subMesh = it->second;
            auto drawObj = iGame::DynamicCast<iGame::DrawObject>(subMesh);
            if (drawObj) {
                drawObj->SetViewStyle(IG_SURFACE);     // Surface shading
                drawObj->AddViewStyle(IG_WIREFRAME);   // Wireframe overlay
                drawObj->ConvertToDrawableData();
                scene->AddModel(subMesh);
            }
        }
    } else {
        auto drawObj = iGame::DynamicCast<iGame::DrawObject>(res);
        if (drawObj) {
            drawObj->SetViewStyle(IG_SURFACE);
            drawObj->AddViewStyle(IG_WIREFRAME);
            drawObj->ConvertToDrawableData();
            scene->AddModel(res);
        }
    }

    /* 7. Launch 3D OpenGL Rendering Window */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "Launching 3D Interactive Window...\n";
    window->Show();
    return 0;
}