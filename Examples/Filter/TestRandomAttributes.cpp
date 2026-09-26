#pragma once

#include "DataProcessing/iGameRandomAttributesFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameUnstructuredMesh.h"
#include "iGameAttributeSet.h"
#include "iGameType.h"   // 提供全局别名 IGenum/IGsize，以及枚举 IG_POINT/IG_CELL
#include <cstdio>
#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// 说明：
//   参照结果：C:\Users\ROG\Downloads\Tet_Plane (3).vtu   —— dime 平台的结果文件。
//   dime 在该模型上添加的"随机数数组"名称为 RandomPointScalars（点上的随机标量）。
//
//   本测试要复现类似结果：
//     1) 读入模型：Tet_Plane.vtu（8604 点 / 30700 单元，纯四面体，非结构网格）；
//     2) 用 RandomAttributesFilter 生成随机标量，挂载位置由命令行选择：
//          默认挂在点上（IG_POINT，属性名 RandomPointScalars）；
//          传第二个参数 cell 则挂在面/单元上（IG_CELL，属性名 RandomCellScalars）；
//     3) 打印新属性元素个数（挂点=点数8604，挂单元=单元数30700，以此区分两种模式）；
//     4) 用该随机标量弹窗着色显示。
//
//   用法：
//     testRandomAttributes.exe [modelPath] [point|cell]
//       例：testRandomAttributes.exe  ./Models/Tet_Plane.vtu  cell   → 挂到单元
//           testRandomAttributes.exe  ./Models/Tet_Plane.vtu         → 挂到点（默认）
//           testRandomAttributes.exe                                → 使用默认路径
// ---------------------------------------------------------------------------

// 输入/待挂载随机数的模型路径（可用命令行第 1 个参数覆盖）
// 构建后 Models 目录会被拷贝到 cmake-build-examples 下，因此用 ./Models/... 相对路径
static const char* kModelPath = "./Models/Tet_Plane.vtk";


int main(int argc, char** argv) {
    // 1) 模型路径（默认工作目录，可用命令行第 1 个参数覆盖）
    std::string fileName = kModelPath;
    if (argc >= 2) fileName = argv[1];

    // 1.1) 挂载位置：默认点；命令行第 2 个参数为 "cell" 时挂到面/单元
    IGenum attachType = IG_POINT;   // IG_POINT / IG_CELL（注意：均为全局枚举，非 iGame:: 作用域）
    if (argc >= 3 && std::strcmp(argv[2], "cell") == 0) attachType = IG_CELL;

    // 2) 读取 vtu --> 非结构化网格（纯四面体）
    auto mesh = iGame::FileIO::ReadFile(fileName);
    if (mesh == nullptr) {
        igError("failed to read vtu: {}\n", fileName);
        return -1;
    }
    auto attrSet = mesh->GetAttributeSet();
    if (attrSet == nullptr) { igError("no attribute set\n"); return -1; }

    // 记录挂载前已有的属性个数，用作渲染时最后一个新属性的下标依据
    size_t beforeAttrs = attrSet->GetNumberOfAttributes();

    // 3) 生成随机标量（挂点或挂面）
    auto filter = iGame::RandomAttributesFilter::New();
    filter->SetInput(mesh);
    filter->SetRange(0.0f, 255.0f);        // 随机范围
    filter->SetSeed(0u);                   // 固定种子 → 结果可复现
    filter->SetAttachmentType(attachType); // 点(默认) 或 单元(cell)
    if (!filter->Execute()) {
        igError("random attributes filter execute failed!\n");
        return -1;
    }

    // 4) 打印验证信息：网格点/单元数 + 本次新增属性名/元素个数
    auto ps = DynamicCast<iGame::PointSet>(mesh);
    auto um = DynamicCast<iGame::UnstructuredMesh>(mesh);
    std::string newAttrName = "(unknown)";
    IGsize newAttrCount = 0;
    if (attrSet->GetNumberOfAttributes() > beforeAttrs) {
        auto obj = attrSet->GetAttribute((int)attrSet->GetNumberOfAttributes() - 1);
        // ArrayObject 派生类提供 GetName / GetNumberOfElements
        if (obj.pointer) {
            auto arr = DynamicCast<iGame::ArrayObject>(obj.pointer);
            if (arr) {
                newAttrName = arr->GetName();
                newAttrCount = arr->GetNumberOfElements();
            }
        }
    }
    std::printf("[TestRandomAttributes] attach=%s points=%lld cells=%lld newAttr='%s' newAttrCount=%lld\n",
        (attachType == IG_CELL) ? "CELL" : "POINT",
        ps ? (long long)ps->GetNumberOfPoints() : -1LL,
        um ? (long long)um->GetNumberOfCells() : -1LL,
        newAttrName.c_str(), (long long)newAttrCount);

    // 5) 用新随机标量着色显示（最后一个属性即本次新增）
    auto scene = iGame::Scene::New();
    scene->AddModel(mesh);
    int attrIdx = (int)attrSet->GetNumberOfAttributes() - 1;
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
    return 0;
}
