#include <ExtractEdges/iGameExtractEdgesFilter.h>
#include <filesystem>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>

// 中等任务 #28 配套测试用例：提取网格边（去重）
// 运行：cd Examples && ./testExtractEdges
// 通过条件：输出网格全为 IG_LINE 单元，且每条边恰有 2 个互异端点
int main() {
    const std::string fileName = "./Models/ContourExtraction_cylinder_UnstructedGrid.vtk";
    std::cerr << "[testExtractEdges] cwd=" << std::filesystem::current_path().string() << " file=" << fileName
              << " exists=" << std::filesystem::exists(fileName) << "\n"
              << std::flush;

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[testExtractEdges] FAIL: ReadFile returned null\n" << std::flush;
        return 1;
    }
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << "[testExtractEdges] FAIL: not an UnstructuredMesh\n" << std::flush;
        return 1;
    }
    std::cerr << "[testExtractEdges] input points=" << mesh->GetNumberOfPoints()
              << " cells=" << mesh->GetNumberOfCells() << "\n"
              << std::flush;

    auto filter = iGame::ExtractEdgesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        std::cerr << "[testExtractEdges] FAIL: Execute failed\n" << std::flush;
        return 1;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        std::cerr << "[testExtractEdges] FAIL: output is not UnstructuredMesh\n" << std::flush;
        return 1;
    }

    const IGsize edgeNum = out->GetNumberOfCells();
    for (IGsize i = 0; i < edgeNum; ++i) {
        if (out->GetCellType(i) != iGame::IG_LINE) {
            std::cerr << "[testExtractEdges] FAIL: cell " << i << " is not LINE\n" << std::flush;
            return 1;
        }
        const igIndex* pointIds = nullptr;
        const int cellSize = out->GetCellPointIds(i, pointIds);

        if (cellSize != 2) {
            std::cerr << "[testExtractEdges] FAIL: cell " << i << " size " << cellSize << " != 2\n" << std::flush;
            return 1;
        }
    }

    std::cerr << "[testExtractEdges] PASS: " << edgeNum << " unique edges, all are LINE(2 points)\n";
    return 0;
}