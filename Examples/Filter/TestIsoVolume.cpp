#include <IsoVolume/iGameIsoVolumeFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iostream>

/*
 * TestIsoVolume: 等值面之间的体提取(IsoVolume)自动化测试
 * 读取 Examples/Models 下 AI 生成的测试模型(相对路径,无需手动输入),运行 Filter 并打印输出点数/单元数。
 * 测试模型:
 *   1) IsoVolumeTest_RadialShell.vtk  -- Radius 场(到球心距离), 区间 [0.5, 0.9] -> 球壳
 *   2) IsoVolumeTest_DoubleBlob.vtk    -- BlobLevel 场(两个高斯团), 区间 [0.2, 0.6] -> 两个分离区域
 */
static iGame::DataObject::Pointer RunIsoVolumeTest(const std::string& fileName, const std::string& scalarName,
                                                   double lower, double upper, int dim) {
    std::cout << "==== Test: " << fileName << "  scalar=" << scalarName
              << "  [" << lower << ", " << upper << "] (dim " << dim << ")\n";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "  Read ERROR!\n";
        return nullptr;
    }

    /* 按名称取点属性数组 */
    auto attrs = obj->GetAttributeSet()->GetAllPointAttributes();
    if (attrs == nullptr || attrs->GetNumberOfElements() == 0) {
        std::cout << "  No point attributes ERROR!\n";
        return nullptr;
    }
    int found = -1;
    for (int i = 0; i < (int)attrs->GetNumberOfElements(); i++) {
        if (attrs->GetElement(i).pointer->GetName() == scalarName) {
            found = i;
            break;
        }
    }
    if (found < 0) { found = 0; }
    auto& attr = attrs->GetElement(found);
    auto array = attr.pointer;

    /* 等值面之间的体提取 */
    auto filter = iGame::IsoVolumeFilter::New();
    filter->SetInput(obj);
    filter->SetIsoScalarData(array, lower, upper, dim);
    filter->Execute();

    auto res = filter->GetOutput();
    if (res == nullptr) {
        std::cout << "  Output NULL ERROR!\n";
        return nullptr;
    }
    unsigned long long np = 0, nc = 0;
    if (auto m = iGame::DynamicCast<iGame::UnstructuredMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfCells();
    } else if (auto m = iGame::DynamicCast<iGame::SurfaceMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfFaces();
    } else if (auto m = iGame::DynamicCast<iGame::VolumeMesh>(res)) {
        np = m->GetNumberOfPoints();
        nc = m->GetNumberOfVolumes();
    }
    std::cout << "  Output points = " << np << ", cells = " << nc << "\n";
    return res;
}

int main() {
    /* 模型 1: 径向球壳 (Radius = 到球心距离, [0.5, 0.9] -> 两层等值面包夹的球壳) */
    auto res1 = RunIsoVolumeTest("./Models/IsoVolumeTest_RadialShell.vtk", "Radius", 0.5, 0.9, 0);

    /* 模型 2: 双高斯团 (BlobLevel, [0.7, 0.95] -> 两个高斯核心附近的等值体) */
    auto res2 = RunIsoVolumeTest("./Models/IsoVolumeTest_DoubleBlob.vtk", "BlobLevel", 0.7, 0.95, 0);

    std::cout << "TestIsoVolume DONE\n";

    /* 可视化展示 RadialShell 结果 (单个体, 最接近此前 ClipTest 的可渲染结果) */
    if (res1) {
        auto scene = iGame::Scene::New();
        auto draw = iGame::DynamicCast<iGame::DrawObject>(res1);
        if (draw != nullptr) {
            draw->SetViewStyle(IG_SURFACE);
            draw->ViewCloudPicture(scene, 0);
        }
        scene->AddModel(res1);
        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
        window->SetSize(1280, 720);
        window->SetScene(scene);
        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);
        window->Show();
        window->RenderOneFrame();
    }
    return 0;
}
