#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameFileIO.h>
#include <iostream>

// Run with Examples (or the examples build directory containing Models) as cwd.
namespace {
bool Check(iGame::UnstructuredMesh::Pointer mesh, bool sphere, bool strict,
           int expected, const char* label, bool empty = false) {
    auto filter = iGame::ExtractCellsByRegionFilter::New();
    if (sphere) filter->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
    else if (empty) filter->SetBox(iGame::Vector3d(10, 10, 10), iGame::Vector3d(11, 11, 11));
    else filter->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    filter->SetRequireAllPoints(strict);
    filter->SetInput(0, mesh);
    if (!filter->Execute()) {
        std::cerr << "FAIL " << label << ": Execute failed\n";
        return false;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // Each fixture cell has four private points; extraction must compact them.
    const bool ok = !out.IsNull() && out->GetNumberOfCells() == expected
                    && out->GetNumberOfPoints() == expected * 4;
    std::cout << (ok ? "PASS " : "FAIL ") << label << " expected cells=" << expected
              << " actual=" << (out.IsNull() ? -1 : static_cast<int>(out->GetNumberOfCells())) << '\n';
    return ok;
}

bool Run(const char* path, int inputCells, int boxStrict, int boxLoose) {
    auto obj = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh.IsNull() || mesh->GetNumberOfCells() != inputCells
        || mesh->GetNumberOfPoints() != inputCells * 4) {
        std::cerr << "FAIL reading fixture " << path << ". Run from Examples.\n";
        return false;
    }
    std::cout << "Model: " << path << '\n';
    bool ok = true;
    ok = Check(mesh, false, true, boxStrict, "Box strict") && ok;
    ok = Check(mesh, false, false, boxLoose, "Box loose") && ok;
    ok = Check(mesh, true, true, 1, "Sphere strict") && ok;
    ok = Check(mesh, true, false, 3, "Sphere loose (includes boundary)") && ok;
    ok = Check(mesh, false, true, 0, "Empty region", true) && ok;

    auto invalid = iGame::ExtractCellsByRegionFilter::New();
    invalid->SetInput(0, mesh);
    invalid->SetBox(iGame::Vector3d(1, -1, -1), iGame::Vector3d(-1, 1, 1));
    bool rejected = !invalid->Execute();
    std::cout << (rejected ? "PASS " : "FAIL ") << "Invalid box rejected\n";
    ok = rejected && ok;
    for (double radius : {0.0, -1.0}) {
        invalid->SetSphere(iGame::Vector3d(0, 0, 0), radius);
        rejected = !invalid->Execute();
        std::cout << (rejected ? "PASS " : "FAIL ") << "Invalid sphere radius=" << radius << '\n';
        ok = rejected && ok;
    }
    return ok;
}
} // namespace

int main() {
    bool ok = Run("./Models/ExtractCellsByRegion_BoxCases.vtk", 4, 1, 3);
    ok = Run("./Models/ExtractCellsByRegion_SphereCases.vtk", 5, 2, 4) && ok;
    std::cout << (ok ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return ok ? 0 : 1;
}
