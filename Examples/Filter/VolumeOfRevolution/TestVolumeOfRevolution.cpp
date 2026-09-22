#include <VolumeOfRevolution/iGameVolumeOfRevolutionFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSmartPointer.h>
#include <iGameVector.h>
#include <string>
#include <vector>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/vase2.vtk";

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        std::cin.get();
        return 1;
    }


    double angleRad = 360.0 * M_PI / 180.0;
    auto filter = iGame::VolumeOfRevolutionFilter::New();

    filter->SetAxis(iGame::Vector3d(0.0, 0.0, 1.0).normalized(), iGame::Vector3d(0.0, 0.0, 0.0));
    filter->SetResolution(36);
    filter->SetAngle(angleRad);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        std::cin.get();
        return 0;
    }
    obj = filter->GetOutput();
    scene->AddModel(obj);
    // Set up the render window
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // Set up the interactor
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    // Start the render loop
    window->Show();
}