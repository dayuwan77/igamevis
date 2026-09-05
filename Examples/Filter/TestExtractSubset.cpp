#include <ExtractSubset/iGameExtractSubsetFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>

int main() {
    // 创建场景
    auto scene = iGame::Scene::New();

    // 读取结构化网格数据文件
    const std::string fileName = "./Models/kit.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "读取文件失败!\n";
        return 0;
    }

    // 创建提取子集过滤器
    auto filter = iGame::ExtractSubsetFilter::New();
    filter->SetInput(obj);

    // 设置感兴趣区域 (VOI: Volume of Interest)
    // 格式: minI, maxI, minJ, maxJ, minK, maxK
    // 根据实际数据结构调整这些参数
    filter->SetVOI(0, 2, 0, 23, 0, 16);

    // 执行提取
    if (!filter->Execute()) {
        std::cout << "过滤器执行失败!\n";
        return 0;
    }

    // 获取结果
    auto result = filter->GetOutput();
    if (result != nullptr) {
        scene->AddModel(result);
    }

    // 启动渲染窗口
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    // 设置交互器
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    // 显示窗口
    window->Show();
}
