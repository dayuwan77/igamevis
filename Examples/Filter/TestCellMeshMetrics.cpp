#include <Core/iGameScene.h>
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <MeshMetrics/iGameVolumeMeshMetricsFilter.h>
#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// 智能模型路径查找（无论在根目录还是构建目录都能自动找到）
std::string FindModelPath(const std::string& modelName) {
    std::vector<std::string> searchPaths = {"./Models/" + modelName, "./Examples/Models/" + modelName,
                                            "../Examples/Models/" + modelName, "../../Examples/Models/" + modelName};
    for (const auto& path: searchPaths) {
        if (std::filesystem::exists(path)) { return path; }
    }
    return "./Models/" + modelName;
}

// 步骤 1 辅助：打印多块树结构
void PrintTreeStructure(iGame::DataObject::Pointer node, int depth = 0) {
    if (!node) return;
    std::string indent(depth * 4, ' ');
    std::string name = node->GetName().empty() ? "(未命名)" : node->GetName();
    if (node->HasSubDataObject()) {
        std::cout << indent << "├── 装配组: [" << name << "] (包含 " << node->GetNumberOfSubDataObjects()
                  << " 个子块)\n";
        for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it) {
            PrintTreeStructure(it->second, depth + 1);
        }
    } else {
        auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(node);
        std::cout << indent << "└── 实体网格: [" << name << "]"
                  << (mesh ? " (单元数 = " + std::to_string(mesh->GetNumberOfCells()) + ")" : "") << "\n";
    }
}

// 步骤 3：遍历输出树 —— 输出每个叶子及全局的指标范围，着色并配置色条
void ProcessAndColorTree(iGame::DataObject::Pointer node, iGame::Scene::Pointer scene, double& globalMin,
                         double& globalMax, bool& colorBarBound, int depth = 0) {
    if (!node) return;
    std::string indent(depth * 4, ' ');

    // 分支节点：向下递归
    if (node->HasSubDataObject()) {
        std::string name = node->GetName().empty() ? "(未命名)" : node->GetName();
        std::cout << indent << "├── 装配组: [" << name << "] (" << node->GetNumberOfSubDataObjects() << " 个子块)\n";
        for (auto it = node->SubDataObjectIteratorBegin(); it != node->SubDataObjectIteratorEnd(); ++it) {
            ProcessAndColorTree(it->second, scene, globalMin, globalMax, colorBarBound, depth + 1);
        }
        return;
    }

    // 叶子节点：查找 Metric 质量属性并统计范围
    std::cout << indent << "└── 实体网格: [" << node->GetName() << "]\n";
    auto attrSet = node->GetAttributeSet();
    int qualityIndex = -1;
    if (attrSet) {
        for (int i = 0; i < (int)attrSet->GetNumberOfAttributes(); ++i) {
            auto& attr = attrSet->GetAttribute(i);
            if (!attr.isDeleted && attr.pointer && attr.pointer->GetName().find("Metric") != std::string::npos) {
                qualityIndex = i;
                break;
            }
        }
    }
    if (qualityIndex == -1) {
        std::cout << indent << "    (未找到 Metric 质量属性，跳过)\n";
        return;
    }

    auto metricAttr = attrSet->GetAttribute(qualityIndex).pointer;
    const int totalCells = (int)metricAttr->GetNumberOfElements();
    double minVal = DBL_MAX, maxVal = -DBL_MAX, sum = 0.0;
    for (int i = 0; i < totalCells; ++i) {
        double val = metricAttr->GetValue(i);
        sum += val;
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    const double avg = (totalCells > 0) ? (sum / totalCells) : 0.0;
    globalMin = std::min(globalMin, minVal);
    globalMax = std::max(globalMax, maxVal);

    // ⭐ 步骤 3 核心：控制台输出评估指标范围
    std::cout << indent << "    └─ 指标 [" << metricAttr->GetName() << "]: 单元总数 = " << totalCells
              << " | 范围 = [" << minVal << ", " << maxVal << "] | 平均值 = " << avg << "\n";

    // 3D 伪彩着色（表面 + 线框）
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(node);
    if (!drawObj) return;
    drawObj->SetViewStyle(IG_SURFACE);
    drawObj->AddViewStyle(IG_WIREFRAME);
    drawObj->ConvertToDrawableData();
    drawObj->ViewCloudPicture(scene.GetPointer(), qualityIndex);
    scene->AddModel(node);

    // 色条：绑定着色网格的 color mapper（须在 ViewCloudPicture 之后）
    if (!colorBarBound) {
        auto colorBar = scene->GetColorBar2DActor();
        if (colorBar && drawObj->GetColorMapper()) {
            colorBar->SetColorMapper(drawObj->GetColorMapper());
            colorBar->SetTitle("TET_ASPECT_RATIO");
            scene->SetColorBarVisible(true);
            colorBarBound = true;
            std::cout << indent << "    └─ 色条已绑定: TET_ASPECT_RATIO (蓝=范围最小, 白=中值, 红=范围最大)\n";
        }
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // Windows 控制台 UTF-8 支持
#endif
    std::cout << std::unitbuf; // 立即刷新输出，便于定位崩溃点

    std::cout << "\n============================================================\n";
    std::cout << "  【iGameVis】多块体网格 TET Aspect Ratio 质量评估测试\n";
    std::cout << "============================================================\n";

    // 1. 创建场景
    auto scene = iGame::Scene::New();

    // ========== 步骤 1：读取 .vtm 多块模型（两个重叠的飞机） ==========
    std::cout << "\n[步骤 1] 读取多块模型 (.vtm)...\n";
    const std::string vtmPath = FindModelPath("multiblock_test.vtm");
    std::cout << "  文件: " << vtmPath << "\n";
    auto multiBlockObj = iGame::FileIO::ReadFile(vtmPath);
    if (!multiBlockObj) {
        std::cerr << "  [错误] 读取多块模型失败: " << vtmPath << "\n";
        return -1;
    }
    std::cout << "  读取成功，多块树结构:\n";
    PrintTreeStructure(multiBlockObj);
    std::cout << "  [步骤 1] 完成\n";

    // ========== 步骤 2：应用四面体 Aspect Ratio 评估指标 ==========
    std::cout << "\n[步骤 2] 应用四面体 Aspect Ratio (纵横比, 1=正四面体最优) 评估...\n";
    auto filter = iGame::CellMeshMetricsFilter::New();
    filter->setMetric(iGame::VolumeMeshMetricsFilter::TET_ASPECT_RATIO);
    filter->SetInput(0, multiBlockObj);
    if (!filter->Execute()) {
        std::cerr << "  [错误] 质量评估算法执行失败！\n";
        return -1;
    }
    auto outputObj = filter->GetOutput(0);
    if (!outputObj) {
        std::cerr << "  [错误] 输出数据为空！\n";
        return -1;
    }
    std::cout << "  评估完成，属性已挂载到每个叶子网格 (Metric2)\n";
    std::cout << "  [步骤 2] 完成\n";

    // ========== 步骤 3：着色 + 色条 + 输出指标范围 ==========
    std::cout << "\n[步骤 3] 伪彩着色、显示色条并输出指标范围...\n";
    double globalMin = DBL_MAX, globalMax = -DBL_MAX;
    bool colorBarBound = false;
    ProcessAndColorTree(outputObj, scene, globalMin, globalMax, colorBarBound);
    std::cout << "  ------------------------------------------------\n";
    std::cout << "  全局指标范围 (TET_ASPECT_RATIO): [" << globalMin << ", " << globalMax << "]\n";
    std::cout << "  [步骤 3] 完成\n";

    // 4. 启动 3D OpenGL 交互渲染窗口
    std::cout << "\n正在拉起 3D 渲染窗口（左键旋转，滚轮缩放，底部色条显示指标映射范围）...\n";
    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
