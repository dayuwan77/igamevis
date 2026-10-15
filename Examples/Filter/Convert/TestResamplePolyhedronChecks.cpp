// Regression: NGON/NFACE polyhedra were rejected, so CGNS resampling produced no output.
// Verify native and face-stream topology, cell-field snapshots, boundaries,
// and explicit rejection of unsupported point-field interpolation (never silently drop fields).
// BUG (2026-10-14): all valid grid points were drawn instead of
// ParaView's extracted-surface points. Pure Points also dereferenced absent surface edges.
// Fix commit: first addition of this test, subject:
// fix: correct CGNS resampling and align Resample To Image UI
// Resolve SHA: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestResamplePolyhedronChecks.cpp
#include <Convert/iGameResampleToImageFilter.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iostream>

using namespace iGame;

struct DrawArrayProbe : DrawObject {
    static IGsize Indices(DrawObject* object) {
        auto member = &DrawArrayProbe::m_PointIndices;
        return (object->*member)->GetNumberOfValues();
    }
    static IGsize Positions(DrawObject* object) {
        auto member = &DrawArrayProbe::m_Positions;
        return (object->*member)->GetNumberOfElements();
    }
    static IGsize DisplayedPoints(DrawObject* object) {
        object->ConvertToDrawableData();
        return Indices(object) ? Indices(object) : Positions(object);
    }
    static void MarkDerived(DrawObject* object) {
        auto member = &DrawArrayProbe::m_IsMainRenderableObject;
        object->*member = false;
    }
};

int main() {
    auto points = Points::New();
    for (int z = 0; z < 2; ++z)
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) points->AddPoint(x, y, z);
    const igIndex faceIds[6][4] = {{0, 2, 3, 1}, {4, 5, 7, 6}, {0, 1, 5, 4}, {2, 6, 7, 3}, {0, 4, 6, 2}, {1, 3, 7, 5}};
    auto faces = CellArray::New();
    auto volumes = CellArray::New();
    igIndex faceList[] = {0, 1, 2, 3, 4, 5};
    for (const auto& f: faceIds) faces->AddCellIds(f, 4);
    volumes->AddCellIds(faceList, 6);
    auto volume = VolumeMesh::New();
    volume->SetPoints(points);
    volume->InitVolumesWithPolyhedron(faces, volumes);
    auto scalar = DoubleArray::New();
    scalar->SetName("Pressure");
    scalar->AddValue(42.0);
    volume->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, scalar);
    auto unstructured = UnstructuredMesh::New();
    unstructured->GenerateFromVolumeMesh(volume);
    int passed = 0, failed = 0;
    auto check = [&](bool ok, const char* name) {
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
        ok ? ++passed : ++failed;
    };
    for (DataObject::Pointer input: {DataObject::Pointer(volume), DataObject::Pointer(unstructured)}) {
        auto f = ResampleToImageFilter::New();
        f->SetInput(input);
        f->SetSamplingDimensions(5, 5, 5);
        f->SetUseInputBounds(false);
        f->SetSamplingBounds(-0.5, 1.5, -0.5, 1.5, -0.5, 1.5);
        check(f->Execute(), "polyhedron probe succeeds");
        auto out = DynamicCast<StructuredMesh>(f->GetOutput());
        if (!out) continue;
        auto attrs = out->GetAttributeSet();
        auto mask = attrs->GetAttribute(attrs->GetAttributeIndex("vtkValidPointMask")).pointer;
        auto pressure = attrs->GetAttribute(attrs->GetAttributeIndex("Pressure")).pointer;
        int valid = 0;
        bool snapshots = true;
        for (IGsize i = 0; i < mask->GetNumberOfElements(); ++i) {
            valid += mask->GetElementValue(i, 0) != 0;
            snapshots &= pressure->GetElementValue(i, 0) == (mask->GetElementValue(i, 0) ? 42.0 : 0.0);
        }
        check(valid == 27, "inside and boundary points valid, outside points blank");
        check(snapshots, "cell fields become output point snapshots");
        out->SetViewStyle(IG_POINTS);
        out->ConvertToDrawableData();
        auto drawable = out->GetRenderableObject();
        check(DrawArrayProbe::DisplayedPoints(drawable.get()) == 26, "points use the visible-cell boundary, not the interior");
        check(DrawArrayProbe::Positions(drawable.get()) == 26 && out->GetNumberOfPoints() == 125,
              "surface point compaction does not change the image dataset");
        check(out->GetPointSize() == 2, "default point size matches ParaView");
        out->SetViewStyle(IG_SURFACE);
        out->ConvertToDrawableData();
        out->SetViewStyle(IG_POINTS);
        out->ConvertToDrawableData();
        check(DrawArrayProbe::DisplayedPoints(out->GetRenderableObject().get()) == 26,
              "surface to points switch retains the boundary");
        f->SetSamplingBounds(2, 3, 2, 3, 2, 3);
        check(f->Execute(), "outside sampling bounds produce a blank image");
        out = DynamicCast<StructuredMesh>(f->GetOutput());
        out->SetViewStyle(IG_POINTS);
        out->ConvertToDrawableData();
        out->GetRenderableObject()->ConvertToDrawableData();
        check(out->GetNumberOfPoints() == 125 && DrawArrayProbe::Positions(out->GetRenderableObject().get()) == 0,
              "blank image draws no points without clearing the dataset");
    }
    auto pointField = DoubleArray::New();
    pointField->SetName("PointField");
    pointField->Resize(8);
    volume->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, pointField);
    auto f = ResampleToImageFilter::New();
    f->SetInput(volume);
    check(!f->Execute() && !f->GetMessage().empty(), "point interpolation limitation is reported");

    auto tetraPoints = Points::New();
    tetraPoints->AddPoint(0, 0, 0);
    tetraPoints->AddPoint(1, 0, 0);
    tetraPoints->AddPoint(0, 1, 0);
    tetraPoints->AddPoint(0, 0, 1);
    auto tetraFaces = CellArray::New();
    const igIndex tetraIds[4][3] = {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}};
    for (const auto& face: tetraIds) tetraFaces->AddCellIds(face, 3);
    auto tetraVolumes = CellArray::New();
    const igIndex tetraFaceIds[] = {0, 1, 2, 3};
    tetraVolumes->AddCellIds(tetraFaceIds, 4);
    auto tetra = VolumeMesh::New();
    tetra->SetPoints(tetraPoints);
    tetra->InitVolumesWithPolyhedron(tetraFaces, tetraVolumes);
    tetra->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, scalar);
    f->SetInput(tetra);
    f->SetUseInputBounds(false);
    f->SetSamplingDimensions(1, 1, 1);
    f->SetSamplingBounds(0.75, 0.75, 0.75, 0.75, 0.75, 0.75);
    auto validCount = [&]() {
        auto output = f->GetOutput();
        auto attributes = output->GetAttributeSet();
        return attributes->GetAttribute(attributes->GetAttributeIndex("vtkValidPointMask"))
                .pointer->GetElementValue(0, 0);
    };
    check(f->Execute() && validCount() == 1, "legacy VTK mode accepts a box point outside a slanted polyhedron");
    f->SetUseVTKPolyhedronCompatibility(false);
    check(f->Execute() && validCount() == 0, "strict containment rejects the same outside point");

    auto surface = SurfaceMesh::New();
    // Match an extracted shell: a main surface also initializes its meshlet topology.
    DrawArrayProbe::MarkDerived(surface.get());
    surface->SetRemoteRenderingEnabled(true);
    surface->SetPoints(tetraPoints);
    auto triangle = CellArray::New();
    const igIndex triangleIds[] = {0, 1, 2};
    triangle->AddCellIds(triangleIds, 3);
    surface->SetFaces(triangle);
    surface->SetViewStyle(IG_POINTS);
    surface->ConvertToDrawableData();
    check(surface->GetEdges() == nullptr && DrawArrayProbe::Positions(surface.get()) == 4,
          "pure surface points do not require an edge array");
    surface->GetClipper()->m_Box.m_Use = true;
    surface->GetClipper()->Modified();
    surface->ConvertToDrawableData();
    check(surface->GetEdges() == nullptr && DrawArrayProbe::Positions(surface.get()) == 4,
          "clipped surface points tolerate an absent edge array");
    surface->GetClipper()->DisableAll();
    surface->SetViewStyle(IG_WIREFRAME);
    surface->ConvertToDrawableData();
    check(surface->GetNumberOfEdges() == 3, "wireframe still creates triangle edges lazily");
    surface->ReleaseDrawableResources();
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
