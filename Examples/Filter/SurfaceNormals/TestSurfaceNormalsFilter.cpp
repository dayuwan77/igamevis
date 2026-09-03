#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>

#include <iostream>
#include <string>

namespace {

void PrintNormals(const iGame::SurfaceMesh::Pointer& mesh) {
    if (mesh == nullptr) {
        std::cout << "输出网格为空。" << std::endl;
        return;
    }

    auto attributes = mesh->GetAttributeSet();
    if (attributes == nullptr) {
        std::cout << "输出网格没有属性集。" << std::endl;
        return;
    }

    std::cout << "\n========== Surface Normals ==========\n";
    std::cout << "点数量: " << mesh->GetNumberOfPoints() << '\n';
    std::cout << "面数量: " << mesh->GetNumberOfFaces() << '\n';

    std::cout << "\n---------- 点法向量 ----------\n";

    auto pointNormals = attributes->GetArrayPointer(
        IG_NORMAL, IG_POINT, "Normals");

    if (pointNormals == nullptr) {
        std::cout << "未找到点法向量属性 Normals。" << std::endl;
    } else {
        std::cout << "点法向量数量: "
                  << pointNormals->GetNumberOfElements() << '\n';

        for (IGsize pointId = 0;
             pointId < pointNormals->GetNumberOfElements();
             ++pointId) {
            float normal[3] = {0.0f, 0.0f, 0.0f};
            pointNormals->GetElement(pointId, normal);

            std::cout << "Point " << pointId
                      << ": ("
                      << normal[0] << ", "
                      << normal[1] << ", "
                      << normal[2] << ")\n";
        }
    }

    std::cout << "\n---------- 面法向量 ----------\n";

    auto cellNormals = attributes->GetArrayPointer(
        IG_NORMAL, IG_CELL, "Normals");

    if (cellNormals == nullptr) {
        std::cout << "未找到面法向量属性 Normals。" << std::endl;
    } else {
        std::cout << "面法向量数量: "
                  << cellNormals->GetNumberOfElements() << '\n';

        for (IGsize faceId = 0;
             faceId < cellNormals->GetNumberOfElements();
             ++faceId) {
            float normal[3] = {0.0f, 0.0f, 0.0f};
            cellNormals->GetElement(faceId, normal);

            std::cout << "Face " << faceId
                      << ": ("
                      << normal[0] << ", "
                      << normal[1] << ", "
                      << normal[2] << ")\n";
        }
    }

    std::cout << "\n=====================================\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string fileName = "././Models/SurfaceNormalsFilter_test.vtk";

    if (argc > 1 && argv[1] != nullptr) {
        fileName = argv[1];
    }

    std::cout << "读取模型: " << fileName << std::endl;

    auto input = iGame::FileIO::ReadFile(fileName);
    if (input == nullptr) {
        std::cout << "读取模型失败。" << std::endl;
        std::cout << "用法:\n";
        std::cout << "TestSurfaceNormalsFilter.exe Models/example.vtk\n";
        std::cout << "按任意键退出...";
        std::cin.get();
        return 1;
    }

    auto inputMesh = iGame::DynamicCast<iGame::SurfaceMesh>(input);
    if (inputMesh == nullptr) {
        std::cout << "输入数据不是 SurfaceMesh。" << std::endl;
        std::cout << "SurfaceNormalsFilter 只支持 SurfaceMesh。" << std::endl;
        std::cout << "按任意键退出...";
        std::cin.get();
        return 1;
    }

    auto filter = iGame::SurfaceNormalsFilter::New();
    filter->SetInput(inputMesh);

    if (!filter->Execute()) {
        std::cout << "SurfaceNormalsFilter 执行失败。" << std::endl;
        std::cout << "按任意键退出...";
        std::cin.get();
        return 1;
    }

    auto output = iGame::DynamicCast<iGame::SurfaceMesh>(
        filter->GetOutput(0));

    if (output == nullptr) {
        std::cout << "Filter 没有生成有效的 SurfaceMesh 输出。"
                  << std::endl;
        std::cout << "按任意键退出...";
        std::cin.get();
        return 1;
    }

    PrintNormals(output);

    auto scene = iGame::Scene::New();

    // 将 filter 输出作为模型数据加入场景。
    scene->AddModel(output);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "\n关闭渲染窗口后程序退出。" << std::endl;

    window->Show();

    return 0;
}