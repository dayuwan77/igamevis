#pragma once

#include "DataProcessing/iGameRandomAttributesFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameUnstructuredMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameAttributeSet.h"
#include "iGameType.h"
#include <cstdio>
#include <string>

// ---------------------------------------------------------------------------
// TestRandomAttributes - 自动化测试（无需命令行参数）
//
// 使用两个 AI 生成的测试模型，分别测试 IG_POINT 和 IG_CELL 两种挂载模式：
//   模型 1: RandomAttributes_TetraMesh.vtk      (UNSTRUCTURED_GRID, 125 点, 384 单元)
//   模型 2: RandomAttributes_StructuredGrid.vtk  (STRUCTURED_GRID, 512 点, 343 单元)
//
// 测试流程（全自动）：
//   1) 读取模型 1 → 生成点随机标量 (IG_POINT) → 打印验证
//   2) 重新读取模型 1 → 生成单元随机标量 (IG_CELL) → 打印验证
//   3) 读取模型 2 → 生成点随机标量 (IG_POINT) → 打印验证
//   4) 用最后一个随机标量着色显示模型 2
//
// 运行方式：直接运行 testRandomAttributes.exe（无参数）
// ---------------------------------------------------------------------------

static const char* kModel1 = "./Models/RandomAttributes_TetraMesh.vtk";
static const char* kModel2 = "./Models/RandomAttributes_StructuredGrid.vtk";

// 单次测试：读取模型 -> 生成随机标量 -> 打印验证
bool runSingleTest(const char* modelPath, IGenum attachType,
                   float minVal, float maxVal, unsigned seed) {
    std::string attachStr = (attachType == IG_CELL) ? "CELL" : "POINT";

    // 1) 读取模型
    auto mesh = iGame::FileIO::ReadFile(modelPath);
    if (mesh == nullptr) {
        std::printf("[FAIL] %s: failed to read model\n", modelPath);
        return false;
    }
    auto attrSet = mesh->GetAttributeSet();
    if (attrSet == nullptr) {
        std::printf("[FAIL] %s: no attribute set\n", modelPath);
        return false;
    }
    size_t beforeAttrs = attrSet->GetNumberOfAttributes();

    // 打印网格信息
    auto ps = DynamicCast<iGame::PointSet>(mesh);
    auto um = DynamicCast<iGame::UnstructuredMesh>(mesh);
    auto sm = DynamicCast<iGame::SurfaceMesh>(mesh);
    long long nPts = ps ? (long long)ps->GetNumberOfPoints() : -1;
    long long nCells = -1;
    if (um) nCells = (long long)um->GetNumberOfCells();
    else if (sm) nCells = (long long)sm->GetNumberOfFaces();
    std::printf("[INFO] model=%s  attach=%s  nPts=%lld  nCells=%lld\n",
                modelPath, attachStr.c_str(), nPts, nCells);

    // 2) 生成随机标量
    auto filter = iGame::RandomAttributesFilter::New();
    filter->SetInput(mesh);
    filter->SetRange(minVal, maxVal);
    filter->SetSeed(seed);
    filter->SetAttachmentType(attachType);
    if (!filter->Execute()) {
        std::printf("[FAIL] %s: Execute() failed for attach=%s\n",
                    modelPath, attachStr.c_str());
        return false;
    }

    // 3) 验证新增属性
    size_t afterAttrs = attrSet->GetNumberOfAttributes();
    if (afterAttrs <= beforeAttrs) {
        std::printf("[FAIL] %s: no new attribute added\n", modelPath);
        return false;
    }
    auto obj = attrSet->GetAttribute((int)afterAttrs - 1);
    auto arr = DynamicCast<iGame::ArrayObject>(obj.pointer);
    if (!arr) {
        std::printf("[FAIL] %s: new attribute is not ArrayObject\n", modelPath);
        return false;
    }
    std::string attrName = arr->GetName();
    IGsize attrCount = arr->GetNumberOfElements();

    // 期望值：点模式=点数，单元模式=单元数
    long long expected = (attachType == IG_CELL) ? nCells : nPts;
    bool countOk = ((long long)attrCount == expected);

    std::printf("[PASS] %s: attach=%s  newAttr='%s'  nElem=%lld  expected=%lld  %s\n",
                modelPath, attachStr.c_str(), attrName.c_str(),
                (long long)attrCount, expected,
                countOk ? "COUNT_MATCH" : "COUNT_MISMATCH");

    return countOk;
}


int main() {
    std::printf("============================================\n");
    std::printf("  TestRandomAttributes (Auto Test)\n");
    std::printf("============================================\n\n");

    bool allPass = true;

    // Test 1: TetraMesh + IG_POINT
    std::printf("--- Test 1: TetraMesh + POINT ---\n");
    allPass &= runSingleTest(kModel1, IG_POINT, 0.0f, 255.0f, 42u);

    // Test 2: TetraMesh + IG_CELL
    std::printf("\n--- Test 2: TetraMesh + CELL ---\n");
    allPass &= runSingleTest(kModel1, IG_CELL, 0.0f, 1.0f, 100u);

    // Test 3: StructuredGrid + IG_POINT
    std::printf("\n--- Test 3: StructuredGrid + POINT ---\n");
    allPass &= runSingleTest(kModel2, IG_POINT, -10.0f, 10.0f, 7u);

    // Test 4: StructuredGrid + IG_CELL
    std::printf("\n--- Test 4: StructuredGrid + CELL ---\n");
    allPass &= runSingleTest(kModel2, IG_CELL, 0.0f, 1000.0f, 999u);

    // Summary
    std::printf("\n============================================\n");
    std::printf("  Result: %s\n", allPass ? "ALL PASSED" : "SOME FAILED");
    std::printf("============================================\n");

    // Render the last model with random scalars for visual verification
    auto mesh = iGame::FileIO::ReadFile(kModel2);
    if (mesh) {
        auto filter = iGame::RandomAttributesFilter::New();
        filter->SetInput(mesh);
        filter->SetRange(0.0f, 255.0f);
        filter->SetSeed(7u);
        filter->SetAttachmentType(IG_POINT);
        filter->Execute();

        auto attrSet = mesh->GetAttributeSet();
        int attrIdx = (int)attrSet->GetNumberOfAttributes() - 1;

        auto scene = iGame::Scene::New();
        scene->AddModel(mesh);
        auto drawObj = DynamicCast<iGame::DrawObject>(mesh);
        if (drawObj) drawObj->ViewCloudPicture(scene, attrIdx);

        auto window = iGame::RenderWindow::New();
        window->SetSize(960, 720);
        window->SetScene(scene);

        auto interactor = iGame::Interactor::New();
        interactor->Initialize(scene);
        interactor->CreateDefaultStyle();
        window->SetInteractor(interactor);
        window->Show();
    }

    return allPass ? 0 : 1;
}
