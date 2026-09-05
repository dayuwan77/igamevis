#include <BoundaryMeshQuality/iGameBoundaryMeshQualityFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iostream>

// 串行展示三个 BoundaryMeshQuality 指标：
//   1. DistanceFromCellCenterToFaceCenter
//   2. DistanceFromCellCenterToFacePlane
//   3. AngleFaceNormalAndCellCenterToFaceCenterVector
//
// 每次跑一个 filter，往同一个 DrawObject 的 AttributeSet 追加一个新属性；
// 然后弹一个独立窗口显示该属性云图。关闭当前窗口后才进入下一个指标。

int main() {

    auto baseScene = iGame::Scene::New();
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj != nullptr) {
        baseScene->AddModel(dataObj);
    } else {
        std::cerr << "Read ERROR!\n";
        return -1;
    }

    auto drawObj = DynamicCast<iGame::DrawObject>(dataObj);
    if (!drawObj) {
        std::cerr << "Loaded data is not a DrawObject\n";
        return -1;
    }

    const int baseAttrCount = drawObj->GetAttributeSet()->GetNumberOfAttributes();

    iGame::BoundaryMeshQualityFilter::Pointer filter =
        iGame::BoundaryMeshQualityFilter::New();

    struct MetricEntry {
        const char* title;
        iGame::BoundaryMeshQualityFilter::BoundaryMetric metric;
    };

    const MetricEntry metrics[] = {
        {"Metric 1/3: DistanceFromCellCenterToFaceCenter",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_CENTER},
        {"Metric 2/3: DistanceFromCellCenterToFacePlane",
         iGame::BoundaryMeshQualityFilter::DISTANCE_FROM_CELL_CENTER_TO_FACE_PLANE},
        {"Metric 3/3: AngleFaceNormalAndCellCenterToFaceCenterVector",
         iGame::BoundaryMeshQualityFilter::ANGLE_FACE_NORMAL_AND_CELL_CENTER_TO_FACE_CENTER_VECTOR},
    };

    for (int i = 0; i < 3; ++i) {
        std::cout << ">>> " << metrics[i].title << "\n";

        filter->SetBoundaryMetric(metrics[i].metric);
        filter->SetInput(drawObj);
        if (!filter->Execute()) {
            std::cerr << "Filter execute failed: " << filter->GetMessage() << "\n";
            return -1;
        }

        // 与 Qt 路径一致: filter 跑完后重建 GPU 可绘制数据，
        // 否则新增的属性不会上色，颜色映射保持上一次结果
        drawObj->ConvertToDrawableData();

        const int attrIndex = baseAttrCount + i;

        // 每个窗口使用独立 Scene，避免共享状态相互污染
        auto scene = iGame::Scene::New();
        scene->AddModel(drawObj);

        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
        window->SetSize(1920, 1080);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);

        // 切到第 i 个新属性 (-1 表示 magnitude / 自动维度)
        drawObj->ViewCloudPicture(scene, attrIndex, -1);

        // Show() 阻塞，直到用户关闭当前窗口
        window->Show();

        std::cout << "    window closed.\n";
    }

    std::cout << "All three boundary metrics displayed. Done.\n";
    return 0;
}
