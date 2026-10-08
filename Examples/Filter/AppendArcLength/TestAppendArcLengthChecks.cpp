// ============================================================================
// AppendArcLength 数值/逻辑一致性回归检查（无窗口，可直接进 CI）
//
// 唯一测试模型：Models/AppendArcLength_test.vtk（8 点）
//   折线 A：(0,0,0)-(3,0,0)-(3,4,0)  → 弧长 0 / 3 / 7
//   折线 B：(10,0,0)-(13,0,0)        → 弧长 0 / 3
//   三角形：(1,5,0)-(2,6,0)-(3,7,0)  → 不是线单元，保持 0
//
// 期望值由 VTK vtkAppendArcLength 实测取得（ParaView 6.2 的 vtkAppendArcLength，
//   输出 vtkFloatArray "arc_length"，1 分量）：[0, 3, 7, 0, 3, 0, 0, 0]
//
// 失败时返回非 0，成功返回 0。运行方式（工作目录为 Examples 构建目录）：
//     ./testAppendArcLengthChecks
// ============================================================================
#include <AppendArcLength/iGameAppendArcLengthFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace iGame;

namespace {

int g_failed = 0;
int g_total = 0;

/// 定位测试模型：依次尝试「当前工作目录 / exe 同目录 / 仓库相对路径」，便于直接双击或任意目录运行。
std::string ResolveModelPath(const char* executable, const std::string& relative) {
    std::vector<std::string> candidates;
    candidates.push_back(relative);
    candidates.push_back("Examples/" + relative);
    candidates.push_back("../Examples/" + relative);
    candidates.push_back("../../Examples/" + relative);
    if (executable != nullptr) {
        const std::string exePath(executable);
        const size_t separator = exePath.find_last_of("/\\");
        if (separator != std::string::npos) {
            candidates.push_back(exePath.substr(0, separator + 1) + relative);
        }
    }
    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.good()) { return candidate; }
    }
    return candidates.front();
}


void Check(bool condition, const std::string& what) {
    ++g_total;
    if (!condition) { ++g_failed; }
    std::cout << (condition ? "  [ OK ] " : "  [FAIL] ") << what << "\n";
}

int CountArraysNamed(DataObject::Pointer object, const std::string& name, IGenum attachmentType) {
    AttributeSet* attributeSet = object ? object->GetAttributeSet() : nullptr;
    if (attributeSet == nullptr) { return 0; }
    int count = 0;
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (attribute.pointer->GetName() == name) { ++count; }
    }
    return count;
}

ArrayObject::Pointer FindArray(DataObject::Pointer object, const std::string& name) {
    AttributeSet* attributeSet = object ? object->GetAttributeSet() : nullptr;
    if (attributeSet == nullptr) { return nullptr; }
    for (IGsize i = 0; i < attributeSet->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributeSet->GetAttribute(i);
        if (attribute.isDeleted || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() == name) { return attribute.pointer; }
    }
    return nullptr;
}

const std::vector<double> kExpected = {0.0, 3.0, 7.0, 0.0, 3.0, 0.0, 0.0, 0.0};

#ifdef _WIN32
extern "C" __declspec(dllimport) unsigned long __stdcall GetConsoleProcessList(unsigned long* processList,
                                                                              unsigned long count);
#endif

/// 双击运行时控制台只属于本进程，此时暂停等待回车，避免"一闪而过"；脚本/CI（共享控制台或无控制台、
/// 输出被重定向）自动跳过，不会卡住流水线。`--pause` 强制暂停，`--no-pause` 强制不暂停。
void PauseIfNeeded(int argc, char** argv) {
    bool force = false;
    bool skip = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--pause") { force = true; }
        if (argument == "--no-pause") { skip = true; }
    }
    if (skip) { return; }
    bool ownConsole = false;
#ifdef _WIN32
    unsigned long processes[4] = {0, 0, 0, 0};
    ownConsole = GetConsoleProcessList(processes, 4) == 1;
#endif
    if (!force && !ownConsole) { return; }
    std::cout << "\n按回车键退出..." << std::flush;
    std::cin.get();
}


bool SameValues(ArrayObject::Pointer array, const std::vector<double>& want) {
    if (!array) { return false; }
    const int dimension = std::max(1, array->GetDimension());
    if (array->GetNumberOfValues() != want.size() * static_cast<size_t>(dimension)) { return false; }
    for (size_t i = 0; i < want.size(); ++i) {
        if (std::fabs(array->GetElementValue(i, 0) - want[i]) > 1e-6) { return false; }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "=== AppendArcLength 回归检查（Models/AppendArcLength_test.vtk）===\n";

    const std::string modelPath = ResolveModelPath(argc > 0 ? argv[0] : nullptr, "Models/AppendArcLength_test.vtk");
    std::cout << "模型：" << modelPath << "\n";
    DataObject::Pointer input = FileIO::ReadFile(modelPath);
    Check(input != nullptr, "读取 " + modelPath);
    if (!input) { return 1; }

    auto points = DynamicCast<PointSet>(input);
    Check(points != nullptr && points->GetNumberOfPoints() == 8, "输入有 8 个点");

    auto filter = AppendArcLengthFilter::New();
    filter->SetInput(input);
    const bool executed = filter->Execute();
    Check(executed, "Execute() 成功");
    if (!executed) { return 1; }

    DataObject::Pointer output = filter->GetOutput(0);
    Check(output != nullptr, "有输出数据");
    if (!output) { return 1; }

    auto outputPoints = DynamicCast<PointSet>(output);
    Check(outputPoints != nullptr && outputPoints->GetNumberOfPoints() == 8, "输出保留 8 个点");

    std::cout << "\n=== 1. arc_length 数值（VTK vtkAppendArcLength 参考）===\n";
    ArrayObject::Pointer arcLength = FindArray(output, "arc_length");
    Check(arcLength != nullptr, "产出点属性数组 arc_length");
    if (arcLength) {
        Check(arcLength->GetDimension() == 1, "维数 = 1");
        Check(CountArraysNamed(output, "arc_length", IG_POINT) == 1, "点属性里只有一份 arc_length");
        Check(SameValues(arcLength, kExpected),
              "数值 = [0, 3, 7, 0, 3, 0, 0, 0]（两条折线累加，三角形顶点保持 0）");
        Check(std::fabs(arcLength->GetElementValue(2, 0) - 7.0) < 1e-6, "折线 A 末点 = 3 + 4 = 7");
        Check(std::fabs(arcLength->GetElementValue(4, 0) - 3.0) < 1e-6, "折线 B 末点 = 3");
        Check(std::fabs(arcLength->GetElementValue(5, 0)) < 1e-6 &&
                      std::fabs(arcLength->GetElementValue(6, 0)) < 1e-6 &&
                      std::fabs(arcLength->GetElementValue(7, 0)) < 1e-6,
              "非线单元（三角形）的顶点保持 0");
    }

    std::cout << "\n=== 2. 重复执行按覆盖语义处理（不堆积同名数组）===\n";
    Check(filter->Execute(), "再次 Execute() 成功");
    DataObject::Pointer second = filter->GetOutput(0);
    Check(CountArraysNamed(second, "arc_length", IG_POINT) == 1, "仍然只有一份 arc_length");
    Check(SameValues(FindArray(second, "arc_length"), kExpected), "数值不变");

    std::cout << "\n结果：通过 " << (g_total - g_failed) << " / " << g_total << "，失败 " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "[PASS] AppendArcLength 回归检查全部通过\n"
                               : "[FAIL] AppendArcLength 回归检查存在失败项\n");
    PauseIfNeeded(argc, argv);
    return g_failed == 0 ? 0 : 1;
}
