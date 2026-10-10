#include "IntegrateVariables/iGameIntegrateVariablesFilter.h"
#include "iGameAttributeSet.h"
#include "iGameFileIO.h"

#include <iomanip>
#include <iostream>
#include <string>

namespace {

// 打印积分结果中的点属性、单元属性以及总测度。
void PrintResult(iGame::DataObject* output) {
    auto* attributes = output ? output->GetAttributeSet() : nullptr;
    if (!attributes) return;

    std::cout << std::setprecision(12);
    auto points = output->GetPoints();
    if (points && points->GetNumberOfPoints() > 0) {
        const auto& point = points->GetPoint(0);
        std::cout << "Center:" << ' ' << point[0] << ' ' << point[1]
                  << ' ' << point[2] << '\n';
    }
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.IsNone() || attribute.isDeleted || !attribute.pointer) continue;

        std::cout << (attribute.attachmentType == IG_POINT ? "Point Data: " : "Cell Data:  ")
                  << attribute.pointer->GetName() << " =";
        for (int component = 0; component < attribute.pointer->GetDimension(); ++component) {
            std::cout << ' ' << attribute.pointer->GetElementValue(0, component);
        }
        std::cout << '\n';
    }
}

// 在同一个模型上执行一次 Integrate Variables。
bool RunFilter(const iGame::DataObject::Pointer& input, bool divideCellData) {
    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(input);
    filter->SetDivideAllCellDataByMeasure(divideCellData);
    if (!filter->Execute()) {
        std::cerr << "[IntegrateVariables] " << filter->GetMessage() << '\n';
        return false;
    }

    std::cout << "\nDivide Cell Data By Volume: "
              << (divideCellData ? "On" : "Off") << '\n';
    PrintResult(filter->GetOutput().GetPointer());
    return true;
}

// 读取模型，并分别显示积分值和 Cell Data 加权平均值。
bool RunModel(const std::string& fileName) {
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input) {
        std::cerr << "[IntegrateVariables] failed to read: " << fileName << '\n';
        return false;
    }

    std::cout << "\n========== " << fileName << " ==========\n";
    return RunFilter(input, false) && RunFilter(input, true);
}

} // namespace

int main() {
    // 回归背景：合并新版 main 后，原先使用的 vase2、pyramid_roof 和
    // AIGen_Tet_TwistedRod 测试模型已从仓库删除，干净构建会因读取失败而退出。
    // 这里改用上游仍维护的二维非结构网格、混合维度网格和多边形表面网格，保证
    // Integrate Variables 在无属性、有点/单元属性及 SurfaceMesh 输入上都能成功执行。
    // 修复与本测试更新同次提交，提交主题：Merge upstream/main into feature/integrate_variables
    // 查询命令：git log --format="%h %s" -- Examples/Filter/FeatureExtraction/IntegrateVariables.cpp
    const std::string models[] = {
        "./Models/OutlineCorners_Plane.vtk",
        "./Models/ExtractCellsByType_mixed.vtk",
        "./Models/GhostCell_Pyramid.vtk",
    };

    for (const auto& model : models) {
        if (!RunModel(model)) return 1;
    }
    return 0;
}
