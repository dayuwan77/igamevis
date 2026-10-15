#include "FeatureExtraction/iGamePlaneSamplingFilter.h"
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>
#include <iGameDrawObject.h>
#include <cmath>
#include <string>
#include <iostream>

// 辅助：查属性数组（按名字 + 挂载位置）
static iGame::ArrayObject::Pointer findArray(iGame::DataObject::Pointer obj,
                                             const std::string& name,
                                             IGenum attachment) {
    if (obj == nullptr) return nullptr;
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) return nullptr;
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) return nullptr;
    for (int i = 0; i < (int) all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) continue;
        if (attr.attachmentType != attachment) continue;
        if (std::string(attr.pointer->GetName()) == name) return attr.pointer;
    }
    return nullptr;
}

// main
int main() {
    std::cout << "========== PlaneSamplingFilter Test ==========" << std::endl;

    // ---- 1. 读测试数据 ----
    const std::string fileName = "./Models/ContourExtraction_cylinder_UnstructedGrid.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR: cannot open " << fileName << std::endl;
        return 1;
    }

    auto inPoints = obj->GetPoints();
    auto inCells  = obj->GetCellArray();
    if (inPoints == nullptr || inCells == nullptr) {
        std::cout << "Input data has no points/cells" << std::endl;
        return 1;
    }

    const IGsize inPointNum = inPoints->GetNumberOfPoints();
    const IGsize inCellNum  = inCells ->GetNumberOfCells();
    std::cout << "Input  - Points: " << inPointNum
              << ", Cells: " << inCellNum << std::endl;

    bool allPassed = true;

    // Test 1：默认参数采样
    //   原点 (0,0,0) / 法向 (0,0,1) / 分辨率 20
    //   期望：400 采样点 / 361 四边形 / 有 vtkValidPointMask
    {
        std::cout << "\n--- Test 1: Default sampling (origin 0,0,0; normal 0,0,1; res 20) ---" << std::endl;

        auto filter = iGame::PlaneSamplingFilter::New();
        filter->SetInput(obj);
        filter->SetResolution(20);
        double origin[3] = {0.0, 0.0, 0.0};
        double normal[3] = {0.0, 0.0, 1.0};
        filter->SetPlaneOrigin(origin);
        filter->SetPlaneNormal(normal);

        if (!filter->Execute()) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
            if (out == nullptr) {
                std::cout << "FAIL: output is null or not UnstructuredMesh" << std::endl;
                allPassed = false;
            } else {
                std::cout << "Output - Points: " << out->GetNumberOfPoints()
                          << ", Cells: " << out->GetNumberOfCells() << std::endl;
                if (out->GetNumberOfPoints() == 400 && out->GetNumberOfCells() == 361) {
                    std::cout << "PASS" << std::endl;
                } else {
                    std::cout << "FAIL: expect 400/361, got "
                              << out->GetNumberOfPoints() << "/" << out->GetNumberOfCells()
                              << std::endl;
                    allPassed = false;
                }
                // vtkValidPointMask 是否存在
                auto mask = findArray(out, "vtkValidPointMask", IG_POINT);
                if (mask != nullptr) {
                    std::cout << "PASS: 'vtkValidPointMask' present" << std::endl;
                } else {
                    std::cout << "FAIL: 'vtkValidPointMask' missing" << std::endl;
                    allPassed = false;
                }
            }
        }
    }

    // Test 2：不同分辨率（10x10）
    //   期望：100 采样点 / 81 四边形
    {
        std::cout << "\n--- Test 2: Resolution 10 (10x10) ---" << std::endl;

        auto filter = iGame::PlaneSamplingFilter::New();
        filter->SetInput(obj);
        filter->SetResolution(10);
        double origin[3] = {0.0, 0.0, 0.0};
        double normal[3] = {0.0, 0.0, 1.0};
        filter->SetPlaneOrigin(origin);
        filter->SetPlaneNormal(normal);

        if (!filter->Execute()) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
            if (out == nullptr) {
                std::cout << "FAIL: output is null or not UnstructuredMesh" << std::endl;
                allPassed = false;
            } else {
                std::cout << "Output - Points: " << out->GetNumberOfPoints()
                          << ", Cells: " << out->GetNumberOfCells() << std::endl;
                if (out->GetNumberOfPoints() == 100 && out->GetNumberOfCells() == 81) {
                    std::cout << "PASS" << std::endl;
                } else {
                    std::cout << "FAIL: expect 100/81, got "
                              << out->GetNumberOfPoints() << "/" << out->GetNumberOfCells()
                              << std::endl;
                    allPassed = false;
                }
            }
        }
    }

    // Test 3：矢量属性处理
    //   输入若有 3 维矢量 V，输出应有 V（dim == 3）
    //   验证 PR 中的 AddVector 改动
    {
        std::cout << "\n--- Test 3: Vector attribute handling (AddVector) ---" << std::endl;

        auto vIn = findArray(obj, "V", IG_POINT);
        if (vIn == nullptr) {
            std::cout << "SKIP: input has no 'V' attribute" << std::endl;
        } else {
            auto filter = iGame::PlaneSamplingFilter::New();
            filter->SetInput(obj);
            filter->SetResolution(20);
            double origin[3] = {0.0, 0.0, 0.0};
            double normal[3] = {0.0, 0.0, 1.0};
            filter->SetPlaneOrigin(origin);
            filter->SetPlaneNormal(normal);

            if (!filter->Execute()) {
                std::cout << "FAIL: Execute failed" << std::endl;
                allPassed = false;
            } else {
                auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
                if (out == nullptr) {
                    std::cout << "FAIL: output is null" << std::endl;
                    allPassed = false;
                } else {
                    auto vOut = findArray(out, "V", IG_POINT);
                    if (vOut == nullptr) {
                        std::cout << "FAIL: output has no 'V' attribute" << std::endl;
                        allPassed = false;
                    } else if (vOut->GetDimension() == 3) {
                        std::cout << "PASS: 'V' dimension == 3 (vector)" << std::endl;
                    } else {
                        std::cout << "FAIL: 'V' dimension = " << vOut->GetDimension()
                                  << ", expect 3" << std::endl;
                        allPassed = false;
                    }
                }
            }
        }
    }

    // Test 4：Mask 语义（采样平面远离模型）
    //   原点 (0,0,100000) / 法向 (0,0,1) —— 所有采样点都不在模型内
    //   期望：输出 400 采样点 / 361 四边形 / vtkValidPointMask 全为 0
    //   验证 PR 中"去掉最近点吸附"的改动
    {
        std::cout << "\n--- Test 4: Mask validity (plane far outside model) ---" << std::endl;

        auto filter = iGame::PlaneSamplingFilter::New();
        filter->SetInput(obj);
        filter->SetResolution(20);
        double origin[3] = {0.0, 0.0, 100000.0};
        double normal[3] = {0.0, 0.0, 1.0};
        filter->SetPlaneOrigin(origin);
        filter->SetPlaneNormal(normal);

        if (!filter->Execute()) {
            std::cout << "FAIL: Execute failed" << std::endl;
            allPassed = false;
        } else {
            auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
            if (out == nullptr) {
                std::cout << "FAIL: output is null" << std::endl;
                allPassed = false;
            } else {
                auto mask = findArray(out, "vtkValidPointMask", IG_POINT);
                if (mask == nullptr) {
                    std::cout << "FAIL: 'vtkValidPointMask' missing" << std::endl;
                    allPassed = false;
                } else {
                    bool allZero = true;
                    for (IGsize i = 0; i < 400; ++i) {
                        if (mask->GetElementValue(i, 0) != 0.0) {
                            allZero = false;
                            break;
                        }
                    }
                    if (allZero) {
                        std::cout << "PASS: all vtkValidPointMask values are 0" << std::endl;
                    } else {
                        std::cout << "FAIL: some vtkValidPointMask values are non-zero" << std::endl;
                        allPassed = false;
                    }
                }
            }
        }
    }

    // ---- 最终结果 ----
    if (!allPassed) {
        std::cout << "\n========== SOME TESTS FAILED ==========" << std::endl;
        return 1;
    }
    std::cout << "\n========== ALL TESTS PASSED ==========" << std::endl;

    // 弹窗展示：默认参数采样后的结果
    std::cout << "\n===== Show sampled model in window =====" << std::endl;

    {
        auto filterShow = iGame::PlaneSamplingFilter::New();
        filterShow->SetInput(obj);
        filterShow->SetResolution(20);
        double origin[3] = {0.0, 0.0, 0.0};
        double normal[3] = {0.0, 0.0, 1.0};
        filterShow->SetPlaneOrigin(origin);
        filterShow->SetPlaneNormal(normal);

        if (!filterShow->Execute()) {
            std::cout << "FAIL: filter execute failed" << std::endl;
            return 1;
        }

        auto out = filterShow->GetOutput();
        if (!out) {
            std::cout << "FAIL: output is null" << std::endl;
            return 1;
        }

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

        std::cout << "Showing window, close it to exit..." << std::endl;
        window->Show();
    }

    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}