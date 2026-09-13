#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iostream>
#include <vector>

// Run with Examples (or the examples build directory containing Models) as cwd.
namespace {
void AddAttributes(iGame::UnstructuredMesh::Pointer mesh) {
    auto pointValues = iGame::FloatArray::New();
    pointValues->SetName("PreviewPointValue");
    pointValues->SetDimension(1);
    for (int i = 0; i < mesh->GetNumberOfPoints(); ++i) pointValues->AddValue(static_cast<float>(100 + i));
    mesh->GetAttributeSet()->AddScalar(IG_POINT, pointValues);

    auto cellValues = iGame::IntArray::New();
    cellValues->SetName("MaterialId");
    cellValues->SetDimension(1);
    for (int i = 0; i < mesh->GetNumberOfCells(); ++i) cellValues->AddValue(1000 + i);
    mesh->GetAttributeSet()->AddScalar(IG_CELL, cellValues);
}

bool CheckAttributes(iGame::UnstructuredMesh::Pointer output, const std::vector<int>& expectedIds) {
    auto attributes = output.IsNull() ? nullptr : output->GetAttributeSet();
    if (attributes == nullptr) return false;
    const auto& pointAttribute = attributes->GetScalar("PreviewPointValue");
    const auto& cellAttribute = attributes->GetScalar("MaterialId");
    if (pointAttribute.IsNone() || cellAttribute.IsNone() || pointAttribute.attachmentType != IG_POINT ||
        cellAttribute.attachmentType != IG_CELL || pointAttribute.pointer->GetArrayType() != IG_FloatArray ||
        cellAttribute.pointer->GetArrayType() != IG_IntArray) return false;
    if (pointAttribute.pointer->GetNumberOfElements() != output->GetNumberOfPoints() ||
        cellAttribute.pointer->GetNumberOfElements() != output->GetNumberOfCells()) return false;

    double value[1]{};
    for (int cell = 0; cell < static_cast<int>(expectedIds.size()); ++cell) {
        cellAttribute.pointer->GetElement(cell, value);
        if (value[0] != 1000 + expectedIds[cell]) return false;
        for (int point = 0; point < 4; ++point) {
            pointAttribute.pointer->GetElement(cell * 4 + point, value);
            if (value[0] != 100 + expectedIds[cell] * 4 + point) return false;
        }
    }
    return true;
}

bool Check(iGame::UnstructuredMesh::Pointer mesh, bool sphere, bool strict,
           const std::vector<int>& expectedIds, const char* label, bool empty = false) {
    auto filter = iGame::ExtractCellsByRegionFilter::New();
    if (sphere) filter->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
    else if (empty) filter->SetBox(iGame::Vector3d(10, 10, 10), iGame::Vector3d(11, 11, 11));
    else filter->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    filter->SetRequireAllPoints(strict);
    filter->SetInput(0, mesh);

    if (!filter->Preview() || filter->GetSelectedCellIds() != expectedIds) {
        std::cerr << "FAIL " << label << ": preview IDs do not match expected selection\n";
        return false;
    }
    if (!filter->Execute()) {
        std::cerr << "FAIL " << label << ": Execute failed\n";
        return false;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // Each fixture cell has four private points; extraction must compact them.
    const int expected = static_cast<int>(expectedIds.size());
    const bool ok = !out.IsNull() && filter->GetSelectedCellIds() == expectedIds
                    && out->GetNumberOfCells() == expected && out->GetNumberOfPoints() == expected * 4;
    std::cout << (ok ? "PASS " : "FAIL ") << label << " expected IDs=";
    for (const int id : expectedIds) std::cout << id << ',';
    std::cout << " actual cells=" << (out.IsNull() ? -1 : static_cast<int>(out->GetNumberOfCells())) << '\n';
    const bool attributesOk = expectedIds.empty() || CheckAttributes(out, expectedIds);
    std::cout << (attributesOk ? "PASS " : "FAIL ") << label
              << (expectedIds.empty() ? " empty output has no attributes to copy\n" : " attributes preserved\n");
    return ok && attributesOk;
}

bool Run(const char* path, int inputCells, const std::vector<int>& boxStrict,
         const std::vector<int>& boxLoose) {
    auto obj = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh.IsNull() || mesh->GetNumberOfCells() != inputCells
        || mesh->GetNumberOfPoints() != inputCells * 4) {
        std::cerr << "FAIL reading fixture " << path << ". Run from Examples.\n";
        return false;
    }
    AddAttributes(mesh);
    std::cout << "Model: " << path << '\n';
    bool ok = true;
    ok = Check(mesh, false, true, boxStrict, "Box strict") && ok;
    ok = Check(mesh, false, false, boxLoose, "Box loose") && ok;
    ok = Check(mesh, true, true, {0}, "Sphere strict") && ok;
    ok = Check(mesh, true, false, {0, 1, 3}, "Sphere loose (includes boundary)") && ok;
    ok = Check(mesh, false, true, {}, "Empty region", true) && ok;

    // Reuse the same filter after changing the region: this is the code path used by the GUI's live preview.
    auto adjustable = iGame::ExtractCellsByRegionFilter::New();
    adjustable->SetInput(0, mesh);
    adjustable->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    adjustable->SetRequireAllPoints(false);
    const bool firstPreview = adjustable->Preview() && adjustable->GetSelectedCellIds() == boxLoose;
    adjustable->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
    adjustable->SetRequireAllPoints(true);
    const bool adjustedPreview = adjustable->Preview() && adjustable->GetSelectedCellIds() == std::vector<int>{0};
    std::cout << ((firstPreview && adjustedPreview) ? "PASS " : "FAIL ")
              << "Preview updates after changing region parameters\n";
    ok = firstPreview && adjustedPreview && ok;

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
    bool ok = Run("./Models/ExtractCellsByRegion_BoxCases.vtk", 4, {0}, {0, 1, 3});
    ok = Run("./Models/ExtractCellsByRegion_SphereCases.vtk", 5, {0, 4}, {0, 1, 3, 4}) && ok;
    std::cout << (ok ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return ok ? 0 : 1;
}
