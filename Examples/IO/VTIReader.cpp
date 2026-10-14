/**
 * @class   VTIReaderSelfCheck
 * @brief   .vti (VTK XML ImageData) 读取自检：不创建渲染窗口，直接校验点数 / 维度 / 属性。
 *
 *          用法：testVTIReader [file.vti]
 *          默认依次尝试 ./Models/3DMatrix.vti 与仓库根目录的 3DMatrix.vti（样例数据）。
 *          找不到数据文件时打印 SKIP 并以 0 退出；校验失败返回非 0，便于直接进 CI。
 */
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameStructuredMesh.h>
#include <iGameType.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    std::string filePath;
    if (argc > 1) {
        filePath = argv[1];
    } else {
        const std::vector<std::string> candidates{"./Models/3DMatrix.vti", "../3DMatrix.vti", "../../3DMatrix.vti",
                                                  "../../../3DMatrix.vti", "../../../../3DMatrix.vti"};
        for (const auto& path: candidates) {
            if (fs::exists(path)) {
                filePath = path;
                break;
            }
        }
        if (filePath.empty()) {
            std::cout << "[testVTIReader] SKIP: 3DMatrix.vti not found (pass the file path as argv[1])\n";
            return 0;
        }
    }

    std::cout << "[testVTIReader] file=" << filePath << "\n";
    auto obj = iGame::FileIO::ReadFile(filePath);
    if (obj == nullptr) {
        std::cerr << "[testVTIReader] FAIL: ReadFile returned null\n";
        return 1;
    }

    auto mesh = iGame::DynamicCast<iGame::StructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << "[testVTIReader] FAIL: not a StructuredMesh (type=" << obj->GetDataObjectType() << ")\n";
        return 1;
    }

    const igIndex* size = mesh->GetDimensionSize();
    std::cout << "[testVTIReader] dimension=" << size[0] << "x" << size[1] << "x" << size[2]
              << " points=" << mesh->GetNumberOfPoints() << " cells=" << mesh->GetNumberOfCells() << "\n";

    int failures = 0;
    const IGsize expectedPoints = 21ull * 21ull * 21ull; // WholeExtent="0 20 0 20 0 20"
    if (mesh->GetNumberOfPoints() != expectedPoints) {
        std::cerr << "[testVTIReader] FAIL: expected " << expectedPoints << " points\n";
        ++failures;
    }
    if (size[0] != 21 || size[1] != 21 || size[2] != 21) {
        std::cerr << "[testVTIReader] FAIL: expected dimension 21x21x21\n";
        ++failures;
    }

    auto* attributes = mesh->GetAttributeSet();
    std::cout << "[testVTIReader] attributes=" << attributes->GetNumberOfAttributes() << "\n";
    bool hasTensor = false;
    bool hasMask = false;
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.pointer == nullptr) { continue; }
        std::cout << "[testVTIReader]   [" << i << "] name=" << attr.pointer->GetName() << " type=" << attr.type
                  << " dim=" << attr.pointer->GetDimension() << " values=" << attr.pointer->GetNumberOfValues()
                  << "\n";
        if (attr.type == IG_TENSOR && attr.pointer->GetName() == "values") {
            hasTensor = attr.pointer->GetDimension() == 9 && attr.pointer->GetNumberOfValues() == expectedPoints * 9;
        }
        if (attr.pointer->GetName() == "vtkValidPointMask") {
            hasMask = attr.pointer->GetNumberOfValues() == expectedPoints;
            for (IGsize v = 0; hasMask && v < attr.pointer->GetNumberOfValues(); ++v) {
                hasMask = attr.pointer->GetValue(v) == 1.0;
            }
        }
    }
    if (!hasTensor) {
        std::cerr << "[testVTIReader] FAIL: tensor attribute 'values' (9 components) not loaded\n";
        ++failures;
    }
    if (!hasMask) {
        std::cerr << "[testVTIReader] FAIL: attribute 'vtkValidPointMask' (all ones) not loaded\n";
        ++failures;
    }

    // 惰性数据范围：Element[0] 为各分量模长的 [min,max]，Element[1] 为第 0 个分量的范围
    auto tensorRange = attributes->GetAttribute("values").GetDataRange();
    if (tensorRange != nullptr) {
        std::cout << "[testVTIReader] values magnitude range=[" << tensorRange->GetValue(0) << ", "
                  << tensorRange->GetValue(1) << "]\n";
        if (std::abs(tensorRange->GetValue(1) - 2.3544767157) > 1e-6) {
            std::cerr << "[testVTIReader] FAIL: unexpected tensor magnitude max\n";
            ++failures;
        }
    }

    if (failures != 0) {
        std::cerr << "[testVTIReader] FAILED (" << failures << " check(s))\n";
        return 1;
    }
    std::cout << "[testVTIReader] PASS\n";
    return 0;
}
