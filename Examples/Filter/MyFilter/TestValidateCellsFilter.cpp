#include <MyFilter/iGameValidateCellsFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>

int main() {
    auto scene = iGame::Scene::New();

    const std::string fileName = "././Models/iGameValidateCellsFilter_test.vtk";
    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "读取文件失败!\n";
        std::cout << "按任意键退出...";
        std::cin.get();
        return 0;
    }

    // 先将数据对象加入场景，创建关联的 Model。
    // 无效单元格的高亮绘制依赖 Model 持有的 SelectedCell Painter3D，
    // 因此必须在 filter->Execute() 之前让 Model 存在并关联到 filter。
    scene->AddModel(obj);
    auto model = scene->GetCurrentModel();

    auto filter = iGame::ValidateCellsFilter::New();
    filter->SetInput(obj);
    filter->SetModel(model);  // 关键：设置 Model，Execute() 中的选择高亮才会生效

    if (!filter->Execute()) {
        std::cout << "单元校验执行失败!\n";
        std::cout << "按任意键退出...";
        std::cin.get();
        return 0;
    }

    int invalidCount = filter->GetInvalidCellCount();
    std::cout << "无效单元数量: " << invalidCount << std::endl;
    if (invalidCount > 0) {
        const auto& ids = filter->GetInvalidCellIds();
        std::cout << "无效单元 ID 列表: ";
        for (auto id : ids) std::cout << id << " ";
        std::cout << std::endl;
    }

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();  // 此函数会阻塞直到渲染窗口关闭

    // 渲染窗口关闭后，等待用户按键再退出
    std::cout << "按任意键退出..." << std::endl;
    std::cin.get();

    return 0;
}
