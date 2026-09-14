#include <CountCellVertices/iGameCountCellVerticesFilter.h>
#include <filesystem>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>

// 简单任务 #5 配套测试用例：统计每个单元的顶点数
// 运行：cd Examples && ./testCountCellVertices
// 通过条件：cell_vertex_count 数组存在、长度 == 单元数、每个值与单元点数一致
int main() {
    const std::string fileName = "./Models/ContourExtraction_cylinder_UnstructedGrid.vtk";
    std::cerr << "[testCountCellVertices] cwd=" << std::filesystem::current_path().string() << " file=" << fileName
              << " exists=" << std::filesystem::exists(fileName) << "\n"
              << std::flush;

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[testCountCellVertices] FAIL: ReadFile returned null\n" << std::flush;
        return 1;
    }
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << "[testCountCellVertices] FAIL: not an UnstructuredMesh\n" << std::flush;
        return 1;
    }
    std::cerr << "[testCountCellVertices] points=" << mesh->GetNumberOfPoints() << " cells=" << mesh->GetNumberOfCells()
              << "\n"
              << std::flush;

    auto filter = iGame::CountCellVerticesFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        std::cerr << "[testCountCellVertices] FAIL: Execute failed\n" << std::flush;
        return 1;
    }

    auto attrs = mesh->GetAttributeSet();
    auto all = attrs->GetAllAttributes();
    iGame::ArrayObject::Pointer counts = nullptr;
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || !attr.pointer) { continue; }
        if (std::string(attr.pointer->GetName()) == "cell_vertex_count") {
            counts = attr.pointer;
            break;
        }
    }
    if (counts == nullptr) {
        std::cerr << "[testCountCellVertices] FAIL: cell_vertex_count array not found\n" << std::flush;
        return 1;
    }

    const IGsize n = mesh->GetNumberOfCells();
    if (counts->GetNumberOfValues() != n) {
        std::cerr << "[testCountCellVertices] FAIL: array length " << counts->GetNumberOfValues() << " != cells " << n
                  << "\n"
                  << std::flush;
        return 1;
    }

    bool ok = true;
    for (IGsize i = 0; i < n; ++i) {
        const igIndex* pointIds = nullptr;
        const IGsize expected = static_cast<IGsize>(mesh->GetCellPointIds(i, pointIds));
        const IGsize actual = static_cast<IGsize>(counts->GetValue(i));
        if (expected != actual) {
            std::cerr << "[testCountCellVertices] FAIL: cell " << i << " expected " << expected << " got " << actual
                      << "\n"
                      << std::flush;
            ok = false;
            break;
        }
    }
    if (!ok) { return 1; }

    // 摘要输出（供人工核验）
    std::cerr << "[testCountCellVertices] PASS: " << n << " cells, all vertex counts correct\n";
    std::cerr << "[testCountCellVertices] first 5:";
    for (int i = 0; i < 5 && i < static_cast<int>(n); ++i) {
        std::cerr << " " << static_cast<long long>(counts->GetValue(i));
    }
    std::cerr << "\n" << std::flush;
    return 0;
}