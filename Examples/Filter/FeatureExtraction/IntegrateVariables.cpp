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
    const std::string models[] = {
        "./Models/vase2.vtk",
        "./Models/SurfaceNormalsFilter_pyramid_roof.vtk",
        "./Models/AIGen_Tet_TwistedRod.vtk",
    };

    for (const auto& model : models) {
        if (!RunModel(model)) return 1;
    }
    return 0;
}
