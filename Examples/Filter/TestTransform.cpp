#include <Transformation/iGameTransformFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>

#include <iostream>

int main()
{
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/Convert_Quad_Bicycle.vtk";
    auto input = iGame::FileIO::ReadFile(fileName);

    if (input == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }

    auto filter = iGame::TransformFilter::New();
    filter->SetInput(input);

    filter->SetTranslation(10.0f, 0.0f, 0.0f);
    filter->SetRotation(0.0f, 0.0f, 45.0f);
    filter->SetScale(1.5f, 1.5f, 1.5f);

    if (!filter->Execute()) {
        std::cout << "Transform ERROR!\n";
        return 0;
    }

    auto output = filter->GetOutput();
    if (output == nullptr) {
        std::cout << "Output ERROR!\n";
        return 0;
    }

    scene->AddModel(output);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();

    window->SetInteractor(interactor);
    window->Show();

    return 0;
}