#pragma once

#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameUnstructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameAttributeSet.h"
#include "iGameType.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// 说明：
//   本测试对指定的非结构化/表面网格模型执行"法向偏转"（DeflectNormals）。
//   模型中必须已经存在一个 3 分量的向量场属性（常用名如 Position / Velocity 等），
//   filter 会取该向量场 V，把每个点的基准法向 base（曲面法向 或 用户常数法向）
//   按 ND = normalize(base + strength * V) 进行偏转，结果写入一个新的 3 分量
//   点向量属性，名字固定为 DeflectedNormals。
//
//   用法：
//     testDeflectNormals.exe [modelPath] [vecAttrName] [strength] [useUserNormal nx ny nz]
//
//       例1：最简调用（默认模型、默认向量属性 "Position"、strength=1.0、用曲面法向）
//             testDeflectNormals.exe
//
//       例2：指定模型 + 向量场名字
//             testDeflectNormals.exe  ./Models/VectorFieldTest_Fan_UnstructuredGrid.vtk  Position
//
//       例3：指定偏转强度为 0.5
//             testDeflectNormals.exe  ./Models/VectorFieldTest_Fan_UnstructuredGrid.vtk  Position  0.5
//
//       例4：使用用户常数法向 (0,0,1) 作为基准法向 + strength=2.0
//             testDeflectNormals.exe  ./Models/VectorFieldTest_Fan_UnstructuredGrid.vtk  Position  2.0  1  0 0 1
//             （useUserNormal=1 表示启用，后面跟 nx ny nz；0 或省略则用曲面法向）
// ---------------------------------------------------------------------------

// 默认测试模型（运行时工作目录为 cmake-build-examples，Models 会被自动拷贝过去）
static const char* kDefaultModel = "./Models/VectorFieldTest_Fan_UnstructuredGrid.vtk";
// 默认向量场属性名（根据 summary：该模型 POINT_DATA 里有 3 分量的 Position 可作为向量场）
static const char* kDefaultVecAttr = "Position";
// 默认偏转强度
static const float kDefaultStrength = 1.0f;


int main(int argc, char** argv) {
    // ---------- 1) 解析命令行参数 ----------
    std::string modelPath   = kDefaultModel;
    std::string vecAttrName = kDefaultVecAttr;
    float       strength    = kDefaultStrength;
    bool        useUserNormal = false;
    double      unx = 0.0, uny = 0.0, unz = 1.0;  // 用户法向默认 +Z

    if (argc >= 2)  modelPath   = argv[1];
    if (argc >= 3)  vecAttrName = argv[2];
    if (argc >= 4)  strength    = (float)std::atof(argv[3]);
    if (argc >= 5) {
        int flag = std::atoi(argv[4]);
        useUserNormal = (flag != 0);
        if (useUserNormal && argc >= 8) {
            unx = std::atof(argv[5]);
            uny = std::atof(argv[6]);
            unz = std::atof(argv[7]);
        }
    }

    std::printf("[TestDeflectNormals] model=%s\n", modelPath.c_str());
    std::printf("[TestDeflectNormals] vecAttr='%s'  strength=%.3f  useUserNormal=%d",
                vecAttrName.c_str(), strength, useUserNormal ? 1 : 0);
    if (useUserNormal)
        std::printf("  userNormal=(%.2f, %.2f, %.2f)", unx, uny, unz);
    std::printf("\n");

    // ---------- 2) 读取模型 ----------
    auto obj = iGame::FileIO::ReadFile(modelPath);
    if (obj == nullptr) {
        std::printf("[Error] 读取模型失败：%s\n", modelPath.c_str());
        return -1;
    }
    auto dataObjType = obj->GetDataObjectType();
    std::printf("[Info] dataObjectType = %d  (4=SurfaceMesh, 6=UnstructuredMesh)\n", (int)dataObjType);

    // 尝试拿 PointSet 基类（点数）、SurfaceMesh/UnstructuredMesh（单元/面数）
    auto ps = DynamicCast<iGame::PointSet>(obj);
    auto sm = DynamicCast<iGame::SurfaceMesh>(obj);
    auto um = DynamicCast<iGame::UnstructuredMesh>(obj);
    if (ps) std::printf("[Info] nPoints = %lld\n", (long long)ps->GetNumberOfPoints());
    if (um) std::printf("[Info] nCells  = %lld\n", (long long)um->GetNumberOfCells());
    if (sm && !um) std::printf("[Info] nFaces  = %lld\n", (long long)sm->GetNumberOfFaces());

    // 打印当前已有的属性名列表，方便调试确认向量场叫啥
    auto attrSet = obj->GetAttributeSet();
    if (attrSet) {
        int nAttr = (int)attrSet->GetNumberOfAttributes();
        std::printf("[Info] number of attributes = %d\n", nAttr);
        for (int i = 0; i < nAttr; ++i) {
            auto& a = attrSet->GetAttribute(i);
            if (a.pointer) {
                const char* typeStr = "UNKNOWN";
                if      (a.type == IG_SCALAR) typeStr = "SCALAR";
                else if (a.type == IG_VECTOR) typeStr = "VECTOR";
                else if (a.type == IG_TENSOR) typeStr = "TENSOR";
                const char* attachStr = "?";
                if      (a.attachmentType == IG_POINT) attachStr = "POINT";
                else if (a.attachmentType == IG_CELL)  attachStr = "CELL";
                std::printf("   [%d] %-8s %-6s dim=%d  nElem=%lld  name='%s'\n",
                            i, typeStr, attachStr,
                            a.pointer->GetDimension(),
                            (long long)a.pointer->GetNumberOfElements(),
                            a.pointer->GetName().c_str());
            }
        }
    }

    // ---------- 3) 执行 DeflectNormalsFilter ----------
    size_t beforeAttrs = attrSet ? attrSet->GetNumberOfAttributes() : 0;

    auto filter = iGame::DeflectNormalsFilter::New();
    filter->SetInput(obj);
    filter->SetAttributeByName(vecAttrName);     // 按名指定向量场属性
    filter->SetDeflectStrength(strength);        // 偏转强度
    filter->SetUseUserNormal(useUserNormal);     // 是否用用户常数法向
    if (useUserNormal)
        filter->SetUserNormal(unx, uny, unz);    // 用户法向

    bool ok = filter->Execute();
    if (!ok) {
        std::printf("[Error] DeflectNormalsFilter::Execute() 失败！message=%s\n",
                    filter->GetMessage().c_str());
        return -1;
    }

    // ---------- 4) 验证输出：新增的 DeflectedNormals 属性 ----------
    auto output = filter->GetOutput();
    auto outAttrSet = output ? output->GetAttributeSet() : nullptr;
    if (!outAttrSet) {
        std::printf("[Error] filter 输出没有 AttributeSet！\n");
        return -1;
    }

    // 定位名字为 "DeflectedNormals" 的属性
    std::string resultAttrName = "DeflectedNormals";
    int resultIdx = outAttrSet->GetAttributeIndex(resultAttrName);
    if (resultIdx < 0) {
        // 若按名找不到，就尝试使用"执行后新增的最后一个属性"
        int nAttr = (int)outAttrSet->GetNumberOfAttributes();
        if (nAttr > (int)beforeAttrs) resultIdx = nAttr - 1;
        if (resultIdx >= 0) {
            auto& a = outAttrSet->GetAttribute(resultIdx);
            if (a.pointer) resultAttrName = a.pointer->GetName();
        }
    }

    if (resultIdx < 0) {
        std::printf("[Error] 找不到输出属性 DeflectedNormals！\n");
        return -1;
    }

    auto& resultAttr = outAttrSet->GetAttribute(resultIdx);
    auto resultArr = DynamicCast<iGame::ArrayObject>(resultAttr.pointer);
    if (!resultArr) {
        std::printf("[Error] 输出属性不是有效 ArrayObject！\n");
        return -1;
    }

    const char* attachStr = (resultAttr.attachmentType == IG_POINT) ? "POINT"
                          : (resultAttr.attachmentType == IG_CELL)  ? "CELL"  : "UNKNOWN";
    std::printf("[OK] DeflectNormals 执行成功！输出属性：'%s'  attach=%s  dim=%d  nElem=%lld\n",
                resultAttrName.c_str(), attachStr,
                resultArr->GetDimension(),
                (long long)resultArr->GetNumberOfElements());

    // 抽样打印前几个点的偏转后法向
    if (resultArr->GetDimension() == 3) {
        std::printf("[Sample] 前 5 个点的偏转后法向：\n");
        int printN = 5;
        if ((long long)printN > (long long)resultArr->GetNumberOfElements())
            printN = (int)resultArr->GetNumberOfElements();
        for (int i = 0; i < printN; ++i) {
            float v[3] = {0, 0, 0};
            resultArr->GetElement(i, v);
            std::printf("   pt[%d] = (%.5f, %.5f, %.5f)\n", i, v[0], v[1], v[2]);
        }
    }

    // ---------- 5) 用偏转后的法向属性显示模型 ----------
    auto scene = iGame::Scene::New();
    scene->AddModel(output);

    auto drawObj = DynamicCast<iGame::DrawObject>(output);
    if (drawObj) {
        // 用偏转后的向量属性做云图着色；实际 UI 里一般会做法向渲染，
        // 这里用 ViewCloudPicture 至少能把属性挂上去让用户能看到变化
        drawObj->SetViewStyle(IG_SURFACE);
        drawObj->ViewCloudPicture(scene, resultIdx);
    }

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 960);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
