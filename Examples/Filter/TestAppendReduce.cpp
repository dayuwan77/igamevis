#include <AppendReduce/iGameAppendReduceFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>

using namespace iGame;

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

int main() {
    std::cout << "===== TestAppendReduce =====" << std::endl;

    auto mesh1 = CreateTriangleMesh(0, 0, 0,  1, 0, 0,  0, 1, 0);
    auto mesh2 = CreateTriangleMesh(1, 0, 0,  0, 1, 0,  1, 1, 0);

    std::cout << "mesh1: 3 pts, 1 face" << std::endl;
    std::cout << "mesh2: 3 pts, 1 face" << std::endl;
    std::cout << "Total input: 6 pts, 2 faces" << std::endl;

    // Test 1: merge points on
    std::cout << "\n--- Test 1: Merge points ON ---" << std::endl;
    auto filter = AppendReduceFilter::New();
    filter->SetInput(0, mesh1);
    filter->SetInput(1, mesh2);
    filter->SetMergePoints(true);
    filter->SetTolerance(1e-6f);
    filter->Execute();

    auto result = filter->GetOutput();
    auto outMesh = DynamicCast<SurfaceMesh>(result);
    if (outMesh) {
        std::cout << "Expected: 4 unique pts, 2 faces" << std::endl;
        std::cout << "Actual pts: " << outMesh->GetPoints()->GetNumberOfPoints() << std::endl;
        std::cout << "Actual faces: " << outMesh->GetFaces()->GetNumberOfCells() << std::endl;
    }

    // Test 2: merge points off
    std::cout << "\n--- Test 2: Merge points OFF ---" << std::endl;
    auto filter2 = AppendReduceFilter::New();
    filter2->SetInput(0, mesh1);
    filter2->SetInput(1, mesh2);
    filter2->SetMergePoints(false);
    filter2->Execute();

    auto result2 = filter2->GetOutput();
    auto outMesh2 = DynamicCast<SurfaceMesh>(result2);
    if (outMesh2) {
        std::cout << "Expected: 6 pts, 2 faces" << std::endl;
        std::cout << "Actual pts: " << outMesh2->GetPoints()->GetNumberOfPoints() << std::endl;
        std::cout << "Actual faces: " << outMesh2->GetFaces()->GetNumberOfCells() << std::endl;
    }

    // Test 3: AddInput method
    std::cout << "\n--- Test 3: AddInput method ---" << std::endl;
    auto filter3 = AppendReduceFilter::New();
    filter3->AddInput(mesh1);
    filter3->AddInput(mesh2);
    filter3->SetMergePoints(true);
    filter3->Execute();
    std::cout << "AddInput test done" << std::endl;

    // Test 5: Point scalar attribute merge
    std::cout << "\n--- Test 5: Point scalar attribute merge ---" << std::endl;
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
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Temperature");
            if (!attr.IsNone()) {
                std::cout << "[PASS] Point attr Temperature merged" << std::endl;
                std::cout << "  elements: " << attr.pointer->GetNumberOfElements() << " (expected 4)" << std::endl;
                std::cout << "  dim: " << attr.pointer->GetDimension() << " (expected 1)" << std::endl;
            } else {
                std::cout << "[FAIL] Point attr Temperature not found" << std::endl;
            }
        }
    }

    // Test 6: Cell scalar attribute merge
    std::cout << "\n--- Test 6: Cell scalar attribute merge ---" << std::endl;
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
        if (out) {
            auto& attr = out->GetAttributeSet()->GetScalar("Stress");
            if (!attr.IsNone() && attr.attachmentType == IG_CELL) {
                std::cout << "[PASS] Cell attr Stress merged" << std::endl;
                std::cout << "  elements: " << attr.pointer->GetNumberOfElements() << " (expected 2)" << std::endl;
            } else {
                std::cout << "[FAIL] Cell attr Stress not found or wrong type" << std::endl;
            }
        }
    }

    // Test 7: Attribute intersection rule
    std::cout << "\n--- Test 7: Attribute intersection rule ---" << std::endl;
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

        auto pressA = FloatArray::New();
        pressA->SetName("Pressure");
        pressA->SetDimension(1);
        pressA->AddValue(1.0f);
        pressA->AddValue(2.0f);
        pressA->AddValue(1.5f);
        meshA->GetAttributeSet()->AddScalar(IG_POINT, pressA);

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
        if (out) {
            auto& tempAttr = out->GetAttributeSet()->GetScalar("Temperature");
            auto& pressAttr = out->GetAttributeSet()->GetScalar("Pressure");
            std::cout << "Temperature exists: " << (!tempAttr.IsNone() ? "YES [PASS]" : "NO [FAIL]") << std::endl;
            std::cout << "Pressure exists: " << (pressAttr.IsNone() ? "NO [PASS] (only in meshA)" : "YES [FAIL]") << std::endl;
        }
    }

    // Test 8: Point vector attribute merge
    std::cout << "\n--- Test 8: Point vector attribute merge ---" << std::endl;
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
        if (out) {
            auto& attr = out->GetAttributeSet()->GetVector("Velocity");
            if (!attr.IsNone() && attr.pointer->GetDimension() == 3) {
                std::cout << "[PASS] Vector attr Velocity merged" << std::endl;
                std::cout << "  elements: " << attr.pointer->GetNumberOfElements() << " (expected 4)" << std::endl;
                std::cout << "  dim: " << attr.pointer->GetDimension() << " (expected 3)" << std::endl;
            } else {
                std::cout << "[FAIL] Vector attr Velocity merge failed" << std::endl;
            }
        }
    }

    // Test 4: File-based test (optional)
    std::cout << "\n--- Test 4: File-based test ---" << std::endl;
    auto obj1 = FileIO::ReadFile("./Models/AppendReduce_mesh1.vtk");
    auto obj2 = FileIO::ReadFile("./Models/AppendReduce_mesh2.vtk");

    if (obj1 && obj2) {
        auto filter4 = AppendReduceFilter::New();
        filter4->AddInput(obj1);
        filter4->AddInput(obj2);
        filter4->SetMergePoints(true);
        filter4->SetTolerance(1e-4f);

        if (filter4->Execute()) {
            auto result4 = filter4->GetOutput();

            auto scene = Scene::New();
            (DynamicCast<DrawObject>(result4))->SetViewStyle(IG_SURFACE);
            scene->AddModel(result4);

            RenderWindow::Pointer window = RenderWindow::New();
            window->SetSize(1920, 1080);
            window->SetScene(scene);
            auto interactor = Interactor::New();
            interactor->Initialize(scene);
            interactor->CreateDefaultStyle();
            window->SetInteractor(interactor);
            window->Show();
        }
    } else {
        std::cout << "Model files not found, skipping file-based test" << std::endl;
        std::cout << "(Requires .vtk files in Examples/Models/)" << std::endl;

        auto drawObj = DynamicCast<DrawObject>(result);
        if (drawObj) {
            auto scene = Scene::New();
            drawObj->SetViewStyle(IG_SURFACE);
            scene->AddModel(result);

            RenderWindow::Pointer window = RenderWindow::New();
            window->SetSize(800, 600);
            window->SetScene(scene);
            auto interactor = Interactor::New();
            interactor->Initialize(scene);
            interactor->CreateDefaultStyle();
            window->SetInteractor(interactor);
            window->Show();
        } else {
            std::cout << "Test 1 result is not a DrawObject, skipping visualization" << std::endl;
        }
    }

    std::cout << "\n===== All tests complete =====" << std::endl;
    return 0;
}
