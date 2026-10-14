// GhostCellFilter 测试（新的 vtkGhostType / GhostCellMask 语义）
//
//  模型：Examples/Models/GhostCell_Pyramid.vtk（6 个三角形）、
//        Examples/Models/GhostCell_TwoTets.vtk（2 个四面体），
//        两者的 ghost 标记都写在【单元】数据上，数组名 vtkGhostType（单分量）。
//
//  金字塔的 6 个单元取值：  0, 1, 2, 32, 8, 16
//  两四面体：            32, 1
//
//  期望结果（掩码数组 GhostCellMask，0/1）：
//    任意 ghost 标记      -> 金字塔 [0,1,1,1,1,1]   四面体 [1,1]
//    只勾 DUPLICATECELL=1 -> 金字塔 [0,1,0,0,0,0]   四面体 [0,1]
//    只勾 REFINEDCELL=8   -> 金字塔 [0,0,0,0,1,0]
//    只勾 EXTERIORCELL=16 -> 金字塔 [0,0,0,0,0,1]
//    只勾 HIDDENCELL=32   -> 金字塔 [0,0,0,1,0,0]   四面体 [1,0]
//    1 + 32 都勾          -> 金字塔 [0,1,0,1,0,0]   四面体 [1,1]
//  （值 2 是 VTK 的 HIDDENPOINT，是"点"上的位，单元数组里不参与判断）
//
//  另外检查：独立输出（输入不被修改）、找不到 vtkGhostType 时要报错返回 false。
#include <GhostCell/iGameGhostCellFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>

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

// 依次尝试常见运行目录，找到模型后返回完整相对路径
std::string FindModel(const std::string& name) {
    const std::string candidates[] = {
            "./Examples/Models/" + name,
            "./Models/" + name,
            "../Examples/Models/" + name,
    };
    for (const auto& c: candidates) {
        if (FileExists(c)) { return c; }
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

// 读取单元属性里的单分量数组（不存在返回 false）
bool ReadCellArray(iGame::DataObject::Pointer obj, const std::string& name, std::vector<double>& out) {
    out.clear();
    if (obj.IsNull()) { return false; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return false; }
    const int idx = attrs->GetAttributeIndex(name);
    if (idx < 0) { return false; }
    auto& attr = attrs->GetAttribute(static_cast<IGsize>(idx));
    if (attr.isDeleted || attr.pointer.IsNull()) { return false; }
    if (attr.attachmentType != IG_CELL) { return false; }
    for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
        out.push_back(attr.pointer->GetValue(i));
    }
    return true;
}

bool ExpectValues(const std::vector<double>& actual, const std::vector<double>& expected,
                  const std::string& title) {
    std::cout << "    " << title << " = [";
    for (size_t i = 0; i < actual.size(); i++) {
        std::cout << (i ? " " : "") << actual[i];
    }
    std::cout << " ]  expect [";
    for (size_t i = 0; i < expected.size(); i++) {
        std::cout << (i ? " " : "") << expected[i];
    }
    std::cout << " ]" << std::endl;
    if (actual.size() != expected.size()) { return Check(false, title + " size"); }
    bool ok = true;
    for (size_t i = 0; i < expected.size(); i++) {
        ok &= Check(actual[i] == expected[i],
                    title + " cell " + std::to_string(i) + " = " + std::to_string(expected[i]));
    }
    return ok;
}

// 跑一次 filter 并核对掩码
bool RunCase(const std::string& modelName, const std::vector<double>& expected,
             const std::string& title, bool checkAny = true, bool checkDuplicate = false,
             bool checkHidden = false, bool checkRefined = false, bool checkExterior = false) {
    std::cout << "\n== " << title << " ==" << std::endl;

    Step("find model");
    const std::string path = FindModel(modelName);
    if (path.empty()) {
        std::cout << "MODEL NOT FOUND: " << modelName << std::endl;
        return false;
    }

    Step("read " + path);
    auto input = iGame::FileIO::ReadFile(path);
    if (input.IsNull()) {
        std::cout << "READ FAILED" << std::endl;
        return false;
    }

    std::vector<double> inputGhosts;
    if (!Check(ReadCellArray(input, "vtkGhostType", inputGhosts),
               "input has cell array vtkGhostType")) {
        return false;
    }

    Step("run filter");
    auto filter = iGame::GhostCellFilter::New();
    filter->SetCheckAny(checkAny);
    filter->SetCheckDuplicateCell(checkDuplicate);
    filter->SetCheckHiddenCell(checkHidden);
    filter->SetCheckRefinedCell(checkRefined);
    filter->SetCheckExteriorCell(checkExterior);
    filter->SetInput(0, input);
    if (!Check(filter->Execute(), "filter Execute()")) { return false; }

    Step("check output (independent copy)");
    auto output = filter->GetOutput();
    if (!Check(!output.IsNull(), "output exists")) { return false; }
    const iGame::DataObject* outputRaw = output;
    const iGame::DataObject* inputRaw = input;
    if (!Check(outputRaw != inputRaw, "output is a different object (input not modified)")) {
        return false;
    }
    std::vector<double> tmp;
    if (!Check(!ReadCellArray(input, "GhostCellMask", tmp),
               "input has no GhostCellMask after Execute()")) {
        return false;
    }
    if (!Check(ReadCellArray(input, "vtkGhostType", tmp) && tmp == inputGhosts,
               "input vtkGhostType is unchanged")) {
        return false;
    }

    std::vector<double> mask;
    if (!Check(ReadCellArray(output, "GhostCellMask", mask), "output has cell array GhostCellMask")) {
        return false;
    }
    if (!Check(ReadCellArray(output, "vtkGhostType", tmp) && tmp == inputGhosts,
               "output keeps vtkGhostType")) {
        return false;
    }
    return ExpectValues(mask, expected, title + " mask");
}

} // namespace

int main() {
    bool ok = true;

    // 金字塔：单元取值 0, 1, 2, 32, 8, 16
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 1, 1, 1, 1, 1}, "Test 1: pyramid, any ghost flag");
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 1, 0, 0, 0, 0}, "Test 2: pyramid, DUPLICATECELL(1) only",
                  false, true, false);
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 0, 0, 0, 1, 0}, "Test 3: pyramid, REFINEDCELL(8) only",
                  false, false, false, true, false);
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 0, 0, 0, 0, 1}, "Test 4: pyramid, EXTERIORCELL(16) only",
                  false, false, false, false, true);
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 0, 0, 1, 0, 0}, "Test 5: pyramid, HIDDENCELL(32) only",
                  false, false, true);
    ok &= RunCase("GhostCell_Pyramid.vtk", {0, 1, 0, 1, 0, 0}, "Test 6: pyramid, DUPLICATE + HIDDEN",
                  false, true, true);

    // 两个四面体：单元取值 32, 1
    ok &= RunCase("GhostCell_TwoTets.vtk", {1, 1}, "Test 7: two tets, any ghost flag");
    ok &= RunCase("GhostCell_TwoTets.vtk", {0, 1}, "Test 8: two tets, DUPLICATECELL(1) only",
                  false, true, false);
    ok &= RunCase("GhostCell_TwoTets.vtk", {1, 0}, "Test 9: two tets, HIDDENCELL(32) only",
                  false, false, true);

    // 找不到 vtkGhostType 时要报错返回 false，而不是静默输出全 0
    std::cout << "\n== Test 10: model without vtkGhostType must fail ==" << std::endl;
    {
        auto mesh = iGame::SurfaceMesh::New();
        mesh->SetName("NoGhostArray");
        mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
        mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
        auto faces = iGame::CellArray::New();
        mesh->SetFaces(faces);
        igIndex ids[3] = {0, 1, 2};
        faces->AddCellIds(ids, 3);

        auto filter = iGame::GhostCellFilter::New();
        filter->SetInput(0, mesh);
        ok &= Check(!filter->Execute(), "Execute() returns false when vtkGhostType is missing");
    }

    if (ok) {
        std::cout << "\nALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << "\nSOME TESTS FAILED" << std::endl;
    return 1;
}
