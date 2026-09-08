#include <FeatureExtraction/iGamePlaneSamplingFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>

int main() {
    std::cout << "========== PlaneSamplingFilter Test ==========" << std::endl;

    // 1. 读取测试数据
   const std::string fileName = "./Models/ContourExtraction_cylinder_UnstructedGrid.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR: cannot open " << fileName << std::endl;
        return 1;
    }

    auto inPoints = obj->GetPoints();
    auto inCells = obj->GetCellArray();
    if (inPoints == nullptr || inCells == nullptr) {
        std::cout << "Input data has no points/cells" << std::endl;
        return 1;
    }

    const IGsize inPointNum = inPoints->GetNumberOfPoints();
    const IGsize inCellNum = inCells->GetNumberOfCells();
    std::cout << "Input  - Points: " << inPointNum << ", Cells: " << inCellNum << std::endl;

    // 2. 执行平面采样
    auto filter = iGame::PlaneSamplingFilter::New();
    filter->SetInput(obj);
    filter->SetResolution(20);

    double origin[3] = {0.0, 0.0, 0.0};
    double normal[3] = {0.0, 0.0, 1.0};
    filter->SetPlaneOrigin(origin);
    filter->SetPlaneNormal(normal);

    if (!filter->Execute()) {
        std::cout << "PlaneSamplingFilter Execute FAILED" << std::endl;
        return 1;
    }

    auto out = filter->GetOutput();
    if (out == nullptr) {
        std::cout << "Output is null" << std::endl;
        return 1;
    }

    // 3. 验证输出
    auto outPoints = out->GetPoints();
    auto outCells = out->GetCellArray();

    const IGsize outPointNum = outPoints ? outPoints->GetNumberOfPoints() : 0;
    const IGsize outCellNum = outCells ? outCells->GetNumberOfCells() : 0;

    std::cout << "Output - Points: " << outPointNum << ", Cells: " << outCellNum << std::endl;

    bool allPassed = true;

    if (outPointNum == 400) {
        std::cout << "PASS: Point count matches (400)" << std::endl;
    } else {
        std::cout << "FAIL: Point count mismatch. Expected 400, got " << outPointNum << std::endl;
        allPassed = false;
    }

    if (outCellNum == 361) {
        std::cout << "PASS: Cell count matches (361 quadrilaterals)" << std::endl;
    } else {
        std::cout << "FAIL: Cell count mismatch. Expected 361, got " << outCellNum << std::endl;
        allPassed = false;
    }

    auto outAttrSet = out->GetAttributeSet();
    if (outAttrSet) {
        auto attrs = outAttrSet->GetAllAttributes();
        if (attrs && attrs->GetNumberOfElements() > 0) {
            std::cout << "PASS: Output has attributes (" << attrs->GetNumberOfElements() << " arrays)" << std::endl;
        } else {
            std::cout << "FAIL: Output has no attributes!" << std::endl;
            allPassed = false;
        }
    }

    if (allPassed) {
        std::cout << "\n========== ALL TESTS PASSED ==========" << std::endl;
    } else {
        std::cout << "\n========== SOME TESTS FAILED ==========" << std::endl;
        return 1;
    }

    // 4. 显示结果
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(out);
    if (drawObj) {
        drawObj->SetViewStyle(IG_SURFACE);
        drawObj->ConvertToDrawableData();
    }

    auto scene = iGame::Scene::New();
    scene->AddModel(out);
    scene->ResetCameraView();

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();

    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}