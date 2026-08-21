/**
 * Test: ElevationFilter (DIME Filter #19)
 *
 * Builds a small sloped surface mesh in code (4 points, 2 triangles) with
 * known Z coordinates 0 / 1 / 2 / 3, then verifies:
 *   1. Default mapping  [0, 1]  : elevation = z / 3      (endpoints pinned)
 *   2. Custom mapping  [10, 20] : elevation = 10 + z / 3 * 10
 *   3. Flat-mesh degradation   : all values fall back to Low, no NaN
 * The test is self-contained (no external model file, no render window);
 * it returns 0 on success and 1 on the first failed assertion.
 */
#include <cmath>
#include <iostream>
#include <DIME/iGameElevationFilter.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>

static int g_failures = 0;

static void CheckValue(const char* tag, float actual, float expected) {
    const bool ok = std::fabs(actual - expected) < 1e-5f;
    std::cout << "[" << (ok ? "PASS" : "FAIL") << "] " << tag
              << ": expected " << expected << ", got " << actual << "\n";
    if (!ok) { ++g_failures; }
}

// Retrieve the point-attached FloatArray named `name` from the mesh.
static iGame::FloatArray::Pointer FindArray(iGame::PointSet::Pointer mesh, const std::string& name) {
    auto attrs = mesh->GetAttributeSet();
    for (int i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.pointer != nullptr && attr.attachmentType == IG_POINT &&
            attr.pointer->GetName() == name) {
            return iGame::DynamicCast<iGame::FloatArray>(attr.pointer);
        }
    }
    return nullptr;
}

// 4-point sloped surface: a strip of two triangles, Z rises 0 -> 3.
static iGame::UnstructuredMesh::Pointer NewSlopedMesh() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));   // p0: z = 0
    mesh->AddPoint(iGame::Point(1.f, 0.f, 1.f));   // p1: z = 1
    mesh->AddPoint(iGame::Point(0.f, 1.f, 2.f));   // p2: z = 2
    mesh->AddPoint(iGame::Point(1.f, 1.f, 3.f));   // p3: z = 3
    igIndex tri0[3] = {0, 1, 2};
    igIndex tri1[3] = {1, 3, 2};
    mesh->AddCell(tri0, 3, iGame::IG_TRIANGLE);
    mesh->AddCell(tri1, 3, iGame::IG_TRIANGLE);
    return mesh;
}

int main() {
    // ---- Case 1: default range [0, 1] on Z axis --------------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        if (!filter->Execute() || filter->GetOutput() != mesh) {
            std::cout << "[FAIL] case 1: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        if (arr == nullptr) {
            std::cout << "[FAIL] case 1: attribute 'Elevation' not found\n";
            return 1;
        }
        CheckValue("case1 p0 (z=0 -> 0)",        arr->GetValue(0), 0.0f);
        CheckValue("case1 p1 (z=1 -> 1/3)",      arr->GetValue(1), 1.0f / 3.0f);
        CheckValue("case1 p2 (z=2 -> 2/3)",      arr->GetValue(2), 2.0f / 3.0f);
        CheckValue("case1 p3 (z=3 -> 1)",        arr->GetValue(3), 1.0f);
    }

    // ---- Case 2: custom range [10, 20] -----------------------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetAxis(iGame::ElevationFilter::Axis::Z);
        filter->SetRange(10.f, 20.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 2: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case2 p0 (z=0 -> 10)",       arr->GetValue(0), 10.0f);
        CheckValue("case2 p2 (z=2 -> 50/3)",     arr->GetValue(2), 10.0f + 2.0f / 3.0f * 10.0f);
        CheckValue("case2 p3 (z=3 -> 20)",       arr->GetValue(3), 20.0f);
    }

    // ---- Case 3: flat mesh degrades to constant Low (no NaN) -------------
    {
        auto mesh = iGame::UnstructuredMesh::New();
        mesh->AddPoint(iGame::Point(0.f, 0.f, 5.f));
        mesh->AddPoint(iGame::Point(1.f, 0.f, 5.f));
        mesh->AddPoint(iGame::Point(0.f, 1.f, 5.f));
        igIndex tri[3] = {0, 1, 2};
        mesh->AddCell(tri, 3, iGame::IG_TRIANGLE);

        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetRange(2.f, 6.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 3: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        for (int i = 0; i < 3; ++i) {
            CheckValue("case3 flat mesh -> Low",  arr->GetValue(i), 2.0f);
        }
    }

    if (g_failures == 0) {
        std::cout << "TestElevation: all checks passed\n";
        return 0;
    }
    std::cout << "TestElevation: " << g_failures << " check(s) failed\n";
    return 1;
}
