#include <GhostCell/iGameGhostCellFilter.h>
#include <VTK/iGameGhostVTKReader.h>

#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGameType.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

bool FileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

std::string FindModel() {
    const std::string candidates[] = {
            "./Examples/Models/GhostDemo.vtk",
            "./Models/GhostDemo.vtk",
            "../Examples/Models/GhostDemo.vtk",
    };
    for (const auto& c: candidates) {
        if (FileExists(c)) return c;
    }
    return "";
}

bool Check(bool ok, const std::string& name) {
    if (ok) {
        std::cout << "[PASS] " << name << std::endl;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
    }
    return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

iGame::GhostCellFilter::Pointer MakeFilter(const std::string& arrayName, bool any, bool duplicate, bool hidden) {
    auto filter = iGame::GhostCellFilter::New();
    filter->SetGhostArrayName(arrayName);
    filter->SetCheckAny(any);
    filter->SetCheckDuplicateCell(duplicate);
    filter->SetCheckHiddenCell(hidden);
    return filter;
}

// 运行一次并核对输出的 GhostCellMask
bool RunCase(const std::string& model, const std::string& title, bool any, bool duplicate, bool hidden,
             const std::vector<double>& expected) {
    std::cout << "\n== " << title << " ==" << std::endl;

    Step("read model");
    auto obj = iGame::GhostVTKReader::ReadFile(model);
    if (obj.IsNull()) {
        std::cout << "READ FAILED" << std::endl;
        return false;
    }
    if (!Check(obj->GetAttributeSet()->GetAttributeIndex("vtkGhostType") >= 0, "input has Cell Data vtkGhostType")) {
        return false;
    }

    Step("run filter");
    auto filter = MakeFilter("vtkGhostType", any, duplicate, hidden);
    filter->SetInput(0, obj);
    if (!Check(filter->Execute(), "filter Execute()")) return false;

    Step("check result");
    auto out = filter->GetOutput();
    if (!Check(!out.IsNull(), "output exists")) return false;

    // 独立输出：输入上不能出现 GhostCellMask
    if (!Check(obj->GetAttributeSet()->GetAttributeIndex("GhostCellMask") < 0, "input mesh not modified")) {
        return false;
    }

    int idx = out->GetAttributeSet()->GetAttributeIndex("GhostCellMask");
    if (!Check(idx >= 0, "output has GhostCellMask")) return false;
    auto mask = iGame::DynamicCast<iGame::CharArray>(out->GetAttributeSet()->GetAttribute(idx).pointer);
    if (!Check(!mask.IsNull(), "GhostCellMask is a CharArray")) return false;
    if (!Check(mask->GetNumberOfElements() == static_cast<IGsize>(expected.size()), "mask covers all cells")) {
        return false;
    }

    bool ok = true;
    std::cout << "  GhostCellMask = [";
    for (size_t i = 0; i < expected.size(); i++) {
        const double value = mask->GetValue(i);
        std::cout << " " << value;
        ok &= Check(std::fabs(value - expected[i]) < 1e-6,
                    "cell " + std::to_string(i) + " = " + std::to_string(static_cast<int>(expected[i])));
    }
    std::cout << " ]" << std::endl;
    return ok;
}

// 指定的单元标记数组不存在时，必须报错返回，不能静默输出全 0
bool TestMissingArray(const std::string& model) {
    std::cout << "\n== Test 5: missing ghost array must fail ==" << std::endl;
    auto obj = iGame::GhostVTKReader::ReadFile(model);
    if (obj.IsNull()) { return false; }

    auto filter = MakeFilter("NoSuchGhostArray", true, false, false);
    filter->SetInput(0, obj);
    return Check(!filter->Execute(), "Execute() returns false when array is missing");
}

} // namespace

int main() {
    const std::string model = FindModel();
    if (model.empty()) {
        std::cout << "MODEL NOT FOUND: Examples/Models/GhostDemo.vtk" << std::endl;
        return 1;
    }
    std::cout << "[info] model = " << model << std::endl;
    std::cout << "[info] 6 个单元的 vtkGhostType 依次为 0,1,2,3,4,8" << std::endl;

    bool ok = true;
    // 任意非零标记
    ok &= RunCase(model, "Test 1: any ghost flag", true, false, false, {0, 1, 1, 1, 1, 1});
    // 只看 DUPLICATECELL(1)：命中 1、3
    ok &= RunCase(model, "Test 2: DUPLICATECELL only", false, true, false, {0, 1, 0, 1, 0, 0});
    // 只看 HIDDENCELL(2)：命中 2、3
    ok &= RunCase(model, "Test 3: HIDDENCELL only", false, false, true, {0, 0, 1, 1, 0, 0});
    // 两种都看（或的关系）：命中 1、2、3
    ok &= RunCase(model, "Test 4: DUPLICATE + HIDDEN", false, true, true, {0, 1, 1, 1, 0, 0});
    // 数组不存在 -> 报错
    ok &= TestMissingArray(model);

    if (ok) {
        std::cout << "\nALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << "\nSOME TESTS FAILED" << std::endl;
    return 1;
}