#include <AppendReduce/iGameAppendReduceFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>
#include <string>

using namespace iGame;

static int g_passCount = 0;
static int g_failCount = 0;

static void TestCheck(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << std::endl;
        g_passCount++;
    } else {
        std::cout << "  [FAIL] " << name << std::endl;
        g_failCount++;
    }
}

static SurfaceMesh::Pointer CreateTriangleMesh(
    float v0x, float v0y, float v0z,
    float v1x, float v1y, float v1z,
    float v2x, float v2y, float v2z)
{
    auto points = Points::New();
    points->AddPoint(v0x, v0y, v0z);
    points->AddPoint(v1x, v1y, v1z);
    points->AddPoint(v2x, v2y, v2z);

    auto faces = CellArray::New();
    igIndex faceIds[3] = {0, 1, 2};
    faces->AddCellIds(faceIds, 3);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

int main(int argc, char* argv[]) {
    std::cout << "===== TestAppendReduce =====" << std::endl;

    // ===================================================================
    // Test 1: Merge points ON (programmatically created meshes)
    // ===================================================================
    std::cout << "\n[Test 1] Merge points ON" << std::endl;
    {
        auto mesh1 = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto mesh2 = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto filter = AppendReduceFilter::New();
        filter->AddInput(mesh1);
        filter->AddInput(mesh2);
        filter->SetMergePoints(true);
        filter->SetTolerance(1e-6f);
        filter->Execute();

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        TestCheck("output is SurfaceMesh", outMesh != nullptr);
        if (outMesh) {
            TestCheck("vertex count = 4 (merged)",
                outMesh->GetPoints()->GetNumberOfPoints() == 4);
            TestCheck("face count = 2",
                outMesh->GetFaces()->GetNumberOfCells() == 2);
        }
    }

    // ===================================================================
    // Test 2: Merge points OFF
    // ===================================================================
    std::cout << "\n[Test 2] Merge points OFF" << std::endl;
    {
        auto mesh1 = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto mesh2 = CreateTriangleMesh(2, 0, 0,  3, 0, 0,  2, 1, 0);

        auto filter = AppendReduceFilter::New();
        filter->AddInput(mesh1);
        filter->AddInput(mesh2);
        filter->SetMergePoints(false);
        filter->Execute();

        auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
        TestCheck("output is SurfaceMesh", outMesh != nullptr);
        if (outMesh) {
            TestCheck("vertex count = 6 (not merged)",
                outMesh->GetPoints()->GetNumberOfPoints() == 6);
            TestCheck("face count = 2",
                outMesh->GetFaces()->GetNumberOfCells() == 2);
        }
    }

    // ===================================================================
    // Test 3: Point scalar attribute merge
    // ===================================================================
    std::cout << "\n[Test 3] Point scalar attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto tempA = FloatArray::New();
        tempA->SetName("Temperature");
        tempA->SetDimension(1);
        tempA->AddValue(100.0f);
        tempA->AddValue(200.0f);
        tempA->AddValue(150.0f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, tempA);

        auto tempB = FloatArray::New();
        tempB->SetName("Temperature");
        tempB->SetDimension(1);
        tempB->AddValue(200.0f);
        tempB->AddValue(150.0f);
        tempB->AddValue(300.0f);
        meshB->GetAttributeSet()->AddScalar(IG_POINT, tempB);

        auto f = AppendReduceFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->SetTolerance(1e-6f);
        f->Execute();

        auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
        TestCheck("output is SurfaceMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Temperature");
            TestCheck("Temperature attribute exists", !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Temperature is point attribute",
                    attr.attachmentType == IG_POINT);
                TestCheck("Temperature element count = 4",
                    attr.pointer->GetNumberOfElements() == 4);
                TestCheck("Temperature dimension = 1",
                    attr.pointer->GetDimension() == 1);
            }
        }
    }

    // ===================================================================
    // Test 4: Cell scalar attribute merge
    // ===================================================================
    std::cout << "\n[Test 4] Cell scalar attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(2, 0, 0,  3, 0, 0,  2, 1, 0);

        auto stressA = FloatArray::New();
        stressA->SetName("Stress");
        stressA->SetDimension(1);
        stressA->AddValue(10.5f);
        meshA->GetAttributeSet()->AddScalar(IG_CELL, stressA);

        auto stressB = FloatArray::New();
        stressB->SetName("Stress");
        stressB->SetDimension(1);
        stressB->AddValue(20.5f);
        meshB->GetAttributeSet()->AddScalar(IG_CELL, stressB);

        auto f = AppendReduceFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(false);
        f->Execute();

        auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
        TestCheck("output is SurfaceMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Stress");
            TestCheck("Stress attribute exists", !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Stress is cell attribute",
                    attr.attachmentType == IG_CELL);
                TestCheck("Stress element count = 2",
                    attr.pointer->GetNumberOfElements() == 2);
            }
        }
    }

    // ===================================================================
    // Test 5: Attribute intersection rule (only common attributes survive)
    // ===================================================================
    std::cout << "\n[Test 5] Attribute intersection rule" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        // meshA: Temperature + Pressure
        auto tempA = FloatArray::New();
        tempA->SetName("Temperature");
        tempA->SetDimension(1);
        tempA->AddValue(100.0f);
        tempA->AddValue(200.0f);
        tempA->AddValue(150.0f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, tempA);

        auto pressA = FloatArray::New();
        pressA->SetName("Pressure");
        pressA->SetDimension(1);
        pressA->AddValue(1.0f);
        pressA->AddValue(2.0f);
        pressA->AddValue(1.5f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, pressA);

        // meshB: Temperature only (no Pressure)
        auto tempB = FloatArray::New();
        tempB->SetName("Temperature");
        tempB->SetDimension(1);
        tempB->AddValue(200.0f);
        tempB->AddValue(150.0f);
        tempB->AddValue(300.0f);
        meshB->GetAttributeSet()->AddScalar(IG_POINT, tempB);

        auto f = AppendReduceFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->Execute();

        auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
        TestCheck("output is SurfaceMesh", out != nullptr);
        if (out) {
            auto& tempAttr = out->GetAttributeSet()->GetScalar("Temperature");
            auto& pressAttr = out->GetAttributeSet()->GetScalar("Pressure");
            TestCheck("Temperature exists (common attribute)", !tempAttr.IsNone());
            TestCheck("Pressure does NOT exist (meshA only)", pressAttr.IsNone());
        }
    }

    // ===================================================================
    // Test 6: Point vector attribute merge
    // ===================================================================
    std::cout << "\n[Test 6] Point vector attribute merge" << std::endl;
    {
        auto meshA = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
        auto meshB = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

        auto velA = FloatArray::New();
        velA->SetName("Velocity");
        velA->SetDimension(3);
        float vA[9] = {1,0,0, 0,1,0, 1,1,0};
        for (int i = 0; i < 9; i++) {
            velA->AddValue(vA[i]);
        }
        meshA->GetAttributeSet()->AddVector(IG_POINT, velA);

        auto velB = FloatArray::New();
        velB->SetName("Velocity");
        velB->SetDimension(3);
        float vB[9] = {0,1,0, 1,1,0, 2,0,0};
        for (int i = 0; i < 9; i++) {
            velB->AddValue(vB[i]);
        }
        meshB->GetAttributeSet()->AddVector(IG_POINT, velB);

        auto f = AppendReduceFilter::New();
        f->AddInput(meshA);
        f->AddInput(meshB);
        f->SetMergePoints(true);
        f->Execute();

        auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
        TestCheck("output is SurfaceMesh", out != nullptr);
        if (out) {
            auto& attr = out->GetAttributeSet()->GetVector("Velocity");
            TestCheck("Velocity vector attribute exists", !attr.IsNone());
            if (!attr.IsNone()) {
                TestCheck("Velocity dimension = 3",
                    attr.pointer->GetDimension() == 3);
                TestCheck("Velocity element count = 4",
                    attr.pointer->GetNumberOfElements() == 4);
            }
        }
    }

    // ===================================================================
    // Test 7: File-based test using VTK models (planeA + planeB)
    // ===================================================================
    std::cout << "\n[Test 7] File-based test (planeA + planeB VTK models)" << std::endl;
    {
        const char* fileA = "Models/AppendReduce_planeA.vtk";
        const char* fileB = "Models/AppendReduce_planeB.vtk";

        auto objA = FileIO::ReadFile(fileA);
        auto objB = FileIO::ReadFile(fileB);

        TestCheck("planeA.vtk loaded successfully", objA != nullptr);
        TestCheck("planeB.vtk loaded successfully", objB != nullptr);

        if (objA && objB) {
            // Test 7a: merge ON
            std::cout << "  [7a] Merge points ON" << std::endl;
            auto f = AppendReduceFilter::New();
            f->AddInput(objA);
            f->AddInput(objB);
            f->SetMergePoints(true);
            f->SetTolerance(1e-6f);

            bool ok = f->Execute();
            TestCheck("Execute returns true", ok);

            auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
            TestCheck("output is SurfaceMesh", out != nullptr);
            if (out) {
                // planeA: 16 points, planeB: 16 points
                // Shared edge at x=3: 4 points each => 4 merged, 4 removed
                // Total: 16 + 16 - 4 = 28
                int nPts = (int)out->GetPoints()->GetNumberOfPoints();
                TestCheck("vertex count = 28 (4 shared vertices merged)", nPts == 28);
                TestCheck("face count = 18 (9+9)",
                    out->GetFaces()->GetNumberOfCells() == 18);

                // Check Temperature (both meshes have it)
                auto& tempAttr = out->GetAttributeSet()->GetScalar("Temperature");
                TestCheck("Temperature exists (both meshes have it)",
                    !tempAttr.IsNone());

                // Check Pressure (only planeA has it)
                auto& pressAttr = out->GetAttributeSet()->GetScalar("Pressure");
                TestCheck("Pressure does NOT exist (only planeA has it)",
                    pressAttr.IsNone());

                // Check Velocity (only planeB has it)
                auto& velAttr = out->GetAttributeSet()->GetVector("Velocity");
                TestCheck("Velocity does NOT exist (only planeB has it)",
                    velAttr.IsNone());

                // Check Stress (cell scalar, both meshes have it)
                auto& stressAttr = out->GetAttributeSet()->GetScalar("Stress");
                TestCheck("Stress cell attribute exists (both meshes have it)",
                    !stressAttr.IsNone() && stressAttr.attachmentType == IG_CELL);
            }

            // Test 7b: merge OFF
            std::cout << "  [7b] Merge points OFF" << std::endl;
            auto f2 = AppendReduceFilter::New();
            f2->AddInput(objA);
            f2->AddInput(objB);
            f2->SetMergePoints(false);

            bool ok2 = f2->Execute();
            TestCheck("Execute returns true", ok2);

            auto out2 = DynamicCast<SurfaceMesh>(f2->GetOutput());
            if (out2) {
                int nPts = (int)out2->GetPoints()->GetNumberOfPoints();
                TestCheck("vertex count = 32 (16+16, no merge)", nPts == 32);
                TestCheck("face count = 18 (9+9)",
                    out2->GetFaces()->GetNumberOfCells() == 18);
            }
        } else {
            std::cout << "  [SKIP] VTK model files not found (expected in Models/)" << std::endl;
        }
    }

    // ===================================================================
    // Summary
    // ===================================================================
    std::cout << "\n===== Test Summary =====" << std::endl;
    std::cout << "  Passed: " << g_passCount << std::endl;
    std::cout << "  Failed: " << g_failCount << std::endl;
    std::cout << "  Total:  " << (g_passCount + g_failCount) << std::endl;

    if (g_failCount == 0) {
        std::cout << "\nAll tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!" << std::endl;
        return 1;
    }
}
