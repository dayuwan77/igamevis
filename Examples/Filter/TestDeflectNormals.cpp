#pragma once

#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameAttributeSet.h"
#include "iGameType.h"
#include <cstdio>
#include <string>

// ---------------------------------------------------------------------------
// TestDeflectNormals - 自动化测试（无需命令行参数）
//
// 使用两个 AI 生成的测试模型，分别测试曲面法向和用户常数法向：
//   模型 1: DeflectNormals_SphereSurface.vtk (POLYDATA, 840 点, 1600 三角形)
//           向量场: Velocity（切向流动场）
//   模型 2: DeflectNormals_WaveSurface.vtk   (POLYDATA, 625 点, 1152 三角形)
//           向量场: Position（点坐标作为向量）
//
// 测试流程（全自动）：
//   1) 球面 + Velocity 场 + 曲面法向 + strength=1.0 → 验证输出
//   2) 波面 + Position 场 + 用户法向(0,0,1) + strength=0.5 → 验证输出
//   3) 用偏转后法向量可视化球面模型
//
// 运行方式：直接运行 testDeflectNormals.exe（无参数）
// ---------------------------------------------------------------------------

static const char* kModel1 = "./Models/DeflectNormals_SphereSurface.vtk";
static const char* kModel2 = "./Models/DeflectNormals_WaveSurface.vtk";

// 单次测试
bool runSingleTest(const char* modelPath, const char* vecAttrName,
                   float strength, bool useUserNormal,
                   double unx, double uny, double unz) {
    std::printf("[INFO] model=%s  vecAttr='%s'  strength=%.3f  useUserNormal=%d",
                modelPath, vecAttrName, strength, useUserNormal ? 1 : 0);
    if (useUserNormal)
        std::printf("  userNormal=(%.2f, %.2f, %.2f)", unx, uny, unz);
    std::printf("\n");

    // 1) 读取模型
    auto obj = iGame::FileIO::ReadFile(modelPath);
    if (obj == nullptr) {
        std::printf("[FAIL] failed to read: %s\n", modelPath);
        return false;
    }
    auto ps = DynamicCast<iGame::PointSet>(obj);
    auto sm = DynamicCast<iGame::SurfaceMesh>(obj);
    auto um = DynamicCast<iGame::UnstructuredMesh>(obj);
    long long nPts = ps ? (long long)ps->GetNumberOfPoints() : -1;
    long long nFaces = -1;
    if (um) nFaces = (long long)um->GetNumberOfCells();
    else if (sm) nFaces = (long long)sm->GetNumberOfFaces();
    std::printf("[INFO] nPts=%lld  nFaces=%lld\n", nPts, nFaces);

    // 打印已有属性
    auto attrSet = obj->GetAttributeSet();
    if (attrSet) {
        int nAttr = (int)attrSet->GetNumberOfAttributes();
        std::printf("[INFO] attributes (%d):\n", nAttr);
        for (int i = 0; i < nAttr; ++i) {
            auto& a = attrSet->GetAttribute(i);
            if (a.pointer) {
                const char* typeStr = (a.type == IG_SCALAR) ? "SCALAR"
                                   : (a.type == IG_VECTOR) ? "VECTOR" : "OTHER";
                const char* attStr = (a.attachmentType == IG_POINT) ? "POINT"
                                   : (a.attachmentType == IG_CELL) ? "CELL" : "?";
                std::printf("   [%d] %-6s %-5s dim=%d name='%s'\n",
                            i, typeStr, attStr, a.pointer->GetDimension(),
                            a.pointer->GetName().c_str());
            }
        }
    }

    // 2) 执行 DeflectNormalsFilter
    size_t beforeAttrs = attrSet ? attrSet->GetNumberOfAttributes() : 0;
    auto filter = iGame::DeflectNormalsFilter::New();
    filter->SetInput(obj);
    filter->SetAttributeByName(vecAttrName);
    filter->SetDeflectStrength(strength);
    filter->SetUseUserNormal(useUserNormal);
    if (useUserNormal) filter->SetUserNormal(unx, uny, unz);

    if (!filter->Execute()) {
        std::printf("[FAIL] Execute() failed: %s\n", filter->GetMessage().c_str());
        return false;
    }

    // 3) 验证输出属性 DeflectedNormals
    auto output = filter->GetOutput();
    auto outAttrSet = output ? output->GetAttributeSet() : nullptr;
    if (!outAttrSet) {
        std::printf("[FAIL] no output AttributeSet\n");
        return false;
    }

    int resultIdx = outAttrSet->GetAttributeIndex("DeflectedNormals");
    if (resultIdx < 0) {
        int nAttr = (int)outAttrSet->GetNumberOfAttributes();
        if (nAttr > (int)beforeAttrs) resultIdx = nAttr - 1;
    }
    if (resultIdx < 0) {
        std::printf("[FAIL] DeflectedNormals not found in output\n");
        return false;
    }

    auto& resultAttr = outAttrSet->GetAttribute(resultIdx);
    auto resultArr = DynamicCast<iGame::ArrayObject>(resultAttr.pointer);
    if (!resultArr) {
        std::printf("[FAIL] output attribute not ArrayObject\n");
        return false;
    }

    const char* attStr = (resultAttr.attachmentType == IG_POINT) ? "POINT"
                      : (resultAttr.attachmentType == IG_CELL) ? "CELL" : "UNKNOWN";
    std::printf("[PASS] DeflectedNormals  attach=%s  dim=%d  nElem=%lld\n",
                attStr, resultArr->GetDimension(),
                (long long)resultArr->GetNumberOfElements());

    // 抽样打印前 5 个偏转法向
    if (resultArr->GetDimension() == 3) {
        int printN = 5;
        if ((long long)printN > (long long)resultArr->GetNumberOfElements())
            printN = (int)resultArr->GetNumberOfElements();
        std::printf("[SAMPLE] first %d deflected normals:\n", printN);
        for (int i = 0; i < printN; ++i) {
            float v[3] = {0, 0, 0};
            resultArr->GetElement(i, v);
            std::printf("   pt[%d] = (%.5f, %.5f, %.5f)\n", i, v[0], v[1], v[2]);
        }
    }

    return true;
}


int main() {
    std::printf("============================================\n");
    std::printf("  TestDeflectNormals (Auto Test)\n");
    std::printf("============================================\n\n");

    bool allPass = true;

    // Test 1: Sphere + Velocity field + surface normal + strength=1.0
    std::printf("--- Test 1: SphereSurface + Velocity + surface normal ---\n");
    allPass &= runSingleTest(kModel1, "Velocity", 1.0f, false, 0, 0, 1);

    // Test 2: Wave + Position field + user normal (0,0,1) + strength=0.5
    std::printf("\n--- Test 2: WaveSurface + Position + user normal ---\n");
    allPass &= runSingleTest(kModel2, "Position", 0.5f, true, 0.0, 0.0, 1.0);

    // Summary
    std::printf("\n============================================\n");
    std::printf("  Result: %s\n", allPass ? "ALL PASSED" : "SOME FAILED");
    std::printf("============================================\n");

    // Render sphere model with deflected normals
    auto obj = iGame::FileIO::ReadFile(kModel1);
    if (obj) {
        auto filter = iGame::DeflectNormalsFilter::New();
        filter->SetInput(obj);
        filter->SetAttributeByName("Velocity");
        filter->SetDeflectStrength(1.0f);
        filter->SetUseUserNormal(false);
        filter->Execute();

        auto output = filter->GetOutput();
        auto outAttrSet = output ? output->GetAttributeSet() : nullptr;

        auto scene = iGame::Scene::New();
        scene->AddModel(output);

        auto drawObj = DynamicCast<iGame::DrawObject>(output);
        if (drawObj && outAttrSet) {
            int idx = outAttrSet->GetAttributeIndex("DeflectedNormals");
            drawObj->SetViewStyle(IG_SURFACE);
            if (idx >= 0) drawObj->ViewCloudPicture(scene, idx);
        }

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
