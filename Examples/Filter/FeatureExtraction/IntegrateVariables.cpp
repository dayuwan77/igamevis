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

} // namespace

int main() {
    const std::string fileName = "./Models/AIGen_Tet_TwistedRod.vtk";
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input) {
        std::cerr << "[IntegrateVariables] failed to read: " << fileName << '\n';
        return 1;
    }

    const bool passed = RunFilter(input, false) && RunFilter(input, true);
    std::cout << "\n[IntegrateVariables] " << (passed ? "PASSED" : "FAILED") << '\n';
    return passed ? 0 : 1;
}
