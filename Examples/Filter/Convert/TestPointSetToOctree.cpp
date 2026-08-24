#include <Convert/iGamePointSetToOctreeFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

int main() {
    /* 创建场景 */
    auto scene = iGame::Scene::New();

    /* 读取点集文件 */
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }

    /* 点集转八叉树：执行转换 */
    auto filter = iGame::PointSetToOctreeFilter::New();
    filter->SetInput(obj);
    filter->SetNumberOfPointsPerCell(1);
    filter->Execute();

    /* 以点的形式显示八叉树输出结果 */
    auto res = filter->GetOutput();
    if (res != nullptr) {
        scene->AddModel(res);
        auto resDraw = iGame::DynamicCast<iGame::DrawObject>(res);
        resDraw->SetViewStyle(IG_POINTS);
        resDraw->SetPointSize(5);
    }

    /* 启动窗口 */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}