#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameFileIO.h>
#include <iostream>

namespace {

// 统计网格上挂着的点属性(cell=false)或单元属性(cell=true)个数,输入/输出走同一函数便于对照
int CountAttachedData(iGame::UnstructuredMesh* mesh, bool cell) {
    if (mesh == nullptr || mesh->GetAttributeSet() == nullptr) return 0;
    auto arr = cell ? mesh->GetAttributeSet()->GetAllCellAttributes()
                    : mesh->GetAttributeSet()->GetAllPointAttributes();
    if (arr.IsNull()) return 0;
    int n = 0;
    auto cnt = arr->GetNumberOfElements();
    for (decltype(cnt) i = 0; i < cnt; i++) {
        auto cur = arr->GetElement(i);
        if (!cur.isDeleted && !cur.pointer.IsNull()) n++;
    }
    return n;
}

void PrintResult(const char* tag, iGame::UnstructuredMesh* out, iGame::UnstructuredMesh* in) {
    if (out == nullptr) {
        std::cout << tag << " : (null)" << std::endl;
        return;
    }
    std::cout << tag << " -> cells " << out->GetNumberOfCells() << ", points " << out->GetNumberOfPoints()
              << " (input points " << in->GetNumberOfPoints() << ")"
              << ", pointData " << CountAttachedData(out, false) << ", cellData " << CountAttachedData(out, true)
              << std::endl;
}

} // namespace

int main() {
    /*Read data*/
    const std::string fileName = "./Models/Tet_Plane.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    iGame::UnstructuredMesh::Pointer mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh.IsNull()) {
        std::cout << "Read ERROR!\n";
        return 0;
    }
    std::cout << "Input  -> cells " << mesh->GetNumberOfCells() << ", points " << mesh->GetNumberOfPoints()
              << ", pointData " << CountAttachedData(mesh, false) << ", cellData " << CountAttachedData(mesh, true)
              << std::endl;

    /* 按盒子提取（严格：所有顶点在盒子内） */
    auto boxFilter = iGame::ExtractCellsByRegionFilter::New();
    boxFilter->SetBox(iGame::Vector3d(-1.0, -1.0, -1.0), iGame::Vector3d(0.5, 0.5, 0.5));
    boxFilter->SetRequireAllPoints(true);
    boxFilter->SetInput(0, mesh);
    if (!boxFilter->Execute()) {
        std::cout << "Box Extract ERROR!\n";
        return 0;
    }
    auto boxMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(boxFilter->GetOutput());
    std::cout << "Box(strict) selected cells: " << (boxMesh.IsNull() ? 0 : boxMesh->GetNumberOfCells()) << std::endl;
    PrintResult("Box(strict) output", boxMesh, mesh);

    /* 校验非法盒：x 维 min>max，应被拒绝（Execute 返回 false） */
    auto invalidBoxFilter = iGame::ExtractCellsByRegionFilter::New();
    invalidBoxFilter->SetBox(iGame::Vector3d(0.5, -1.0, -1.0), iGame::Vector3d(-1.0, 1.0, 1.0));
    invalidBoxFilter->SetRequireAllPoints(true);
    invalidBoxFilter->SetInput(0, mesh);
    std::cout << "Box(invalid min>max) Execute -> " << (invalidBoxFilter->Execute() ? "true" : "false")
              << " (expect false)" << std::endl;

    /* 按球体提取（宽松：任一顶点在球内）。
     * 半径取 0.3，避免半径 1.0 将整张网格全选，无法验证筛选结果。 */
    auto sphereFilter = iGame::ExtractCellsByRegionFilter::New();
    sphereFilter->SetSphere(iGame::Vector3d(0.0, 0.0, 0.0), 0.3);
    sphereFilter->SetRequireAllPoints(false);
    sphereFilter->SetInput(0, mesh);
    if (!sphereFilter->Execute()) {
        std::cout << "Sphere Extract ERROR!\n";
        return 0;
    }
    auto sphereMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(sphereFilter->GetOutput());
    std::cout << "Sphere(loose) selected cells: " << (sphereMesh.IsNull() ? 0 : sphereMesh->GetNumberOfCells())
              << std::endl;
    PrintResult("Sphere(loose) output", sphereMesh, mesh);

    return 0;
}
