#include <DataProcessing/iGameDecimatePolylineFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <cstdio>
#include <limits>
#include <vector>

using namespace iGame;

namespace {

int g_CheckCount = 0;
int g_FailureCount = 0;

void Expect(bool condition, const char* message) {
    ++g_CheckCount;
    if (!condition) ++g_FailureCount;
    std::printf("  [%s] %s\n", condition ? "PASS" : "FAIL", message);
}

IntArray::Pointer MakeIntArray(const char* name, const std::vector<int>& values) {
    auto array = IntArray::New();
    array->SetName(name);
    array->SetDimension(1);
    array->Resize(static_cast<IGsize>(values.size()));
    for (IGsize i = 0; i < static_cast<IGsize>(values.size()); ++i) array->SetValue(i, values[i]);
    return array;
}

IntArray::Pointer FindIntAttribute(DataObject::Pointer object, const char* name) {
    if (!object || !object->GetAttributeSet()) return nullptr;
    auto& attribute = object->GetAttributeSet()->GetAttribute(name);
    if (attribute.IsNone()) return nullptr;
    return DynamicCast<IntArray>(attribute.pointer);
}

bool Matches(const IntArray::Pointer& array, const std::vector<int>& expected) {
    if (!array || array->GetNumberOfElements() != static_cast<IGsize>(expected.size())) return false;
    for (IGsize i = 0; i < static_cast<IGsize>(expected.size()); ++i) {
        if (array->GetValue(i) != expected[i]) {
            std::printf("    actual values:");
            for (IGsize j = 0; j < array->GetNumberOfElements(); ++j) {
                std::printf(" %.0f", array->GetValue(j));
            }
            std::printf("\n");
            return false;
        }
    }
    return true;
}

SurfaceMesh::Pointer MakeOpenSurfacePolyline() {
    auto mesh = SurfaceMesh::New();
    mesh->SetName("open_curve");
    auto points = mesh->GetPoints();
    const float y[10] = {0.0f, 0.8f, -0.3f, 1.1f, -0.7f, 0.4f, -1.0f, 0.6f, -0.2f, 0.9f};
    for (int i = 0; i < 10; ++i) points->AddPoint(Point(static_cast<float>(i), y[i], 0.1f * i));

    auto edges = CellArray::New();
    std::vector<igIndex> ids(10);
    for (int i = 0; i < 10; ++i) ids[i] = i;
    edges->AddCellIds(ids.data(), static_cast<int>(ids.size()));
    mesh->SetEdges(edges);

    auto attributes = AttributeSet::New();
    attributes->AddAttribute(IG_SCALAR, IG_POINT,
                             MakeIntArray("OriginalPointId", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    attributes->AddAttribute(IG_SCALAR, IG_CELL, MakeIntArray("CurveKind", {7}));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

SurfaceMesh::Pointer MakeMultiPolyline() {
    auto mesh = SurfaceMesh::New();
    mesh->SetName("multi_curve");
    for (int i = 0; i <= 10; ++i) {
        mesh->GetPoints()->AddPoint(Point(static_cast<float>(i),
                                         (i % 2 == 0) ? 0.7f : -0.5f,
                                         0.05f * static_cast<float>(i * i)));
    }

    auto edges = CellArray::New();
    igIndex first[6] = {0, 1, 2, 3, 4, 5};
    igIndex second[6] = {5, 6, 7, 8, 9, 10};
    edges->AddCellIds(first, 6);
    edges->AddCellIds(second, 6);
    mesh->SetEdges(edges);

    auto attributes = AttributeSet::New();
    attributes->AddAttribute(IG_SCALAR, IG_POINT,
                             MakeIntArray("OriginalPointId", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    attributes->AddAttribute(IG_SCALAR, IG_CELL, MakeIntArray("CurveKind", {11, 22}));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

SurfaceMesh::Pointer MakeClosedPolyline() {
    auto mesh = SurfaceMesh::New();
    mesh->GetPoints()->AddPoint(Point(0.f, 0.f, 0.f));
    mesh->GetPoints()->AddPoint(Point(1.f, 0.f, 0.f));
    mesh->GetPoints()->AddPoint(Point(1.f, 1.f, 0.f));
    mesh->GetPoints()->AddPoint(Point(0.f, 1.f, 0.f));
    igIndex loop[5] = {0, 1, 2, 3, 0};
    auto edges = CellArray::New();
    edges->AddCellIds(loop, 5);
    mesh->SetEdges(edges);
    return mesh;
}

UnstructuredMesh::Pointer Execute(DataObject::Pointer input,
                                  double reduction,
                                  double maximumError = std::numeric_limits<double>::max(),
                                  DecimatePolylineFilter::DecimationStrategy strategy =
                                          DecimatePolylineFilter::DecimationStrategy::Distance,
                                  const char* customFieldName = nullptr) {
    auto filter = DecimatePolylineFilter::New();
    filter->SetInput(input);
    filter->SetTargetReduction(reduction);
    filter->SetMaximumError(maximumError);
    filter->SetDecimationStrategy(strategy);
    if (customFieldName) filter->SetCustomFieldName(customFieldName);
    if (!filter->Execute()) return nullptr;
    return DynamicCast<UnstructuredMesh>(filter->GetOutput());
}

void TestOpenPolylineAndAttributes() {
    std::puts("Test 1: open SurfaceMesh polyline and attribute copying");
    auto input = MakeOpenSurfacePolyline();
    auto output = Execute(input, 0.5);
    Expect(output != nullptr, "SurfaceMesh edge cell is accepted");
    if (!output) return;

    Expect(output->GetNumberOfCells() == 1, "one input polyline produces one output polyline");
    Expect(output->GetNumberOfPoints() == 5, "target reduction 0.5 changes 10 points to 5 points");
    Expect(output->GetCellType(0) == IG_POLY_LINE, "five-point output keeps polyline cell type");

    const igIndex* ids = nullptr;
    const int size = output->GetCellPointIds(0, ids);
    auto originalIds = FindIntAttribute(output, "OriginalPointId");
    Expect(size == 5, "output connectivity has five entries");
    Expect(originalIds && originalIds->GetNumberOfElements() == 5,
           "point attribute is compacted to retained points");
    if (originalIds && size == 5) {
        Expect(originalIds->GetValue(ids[0]) == 0, "first endpoint is preserved");
        Expect(originalIds->GetValue(ids[size - 1]) == 9, "last endpoint is preserved");
    }
    auto curveKind = FindIntAttribute(output, "CurveKind");
    Expect(curveKind && curveKind->GetNumberOfElements() == 1 && curveKind->GetValue(0) == 7,
           "cell attribute is copied from the source line");
}

void TestMaximumErrorAndParameterClamping() {
    std::puts("Test 3: maximum error and clamped parameters");
    auto output = Execute(MakeOpenSurfacePolyline(), 1.0, 0.0);
    Expect(output != nullptr, "zero maximum error execution succeeds");
    if (output) Expect(output->GetNumberOfPoints() == 10, "non-collinear points exceed zero maximum error");

    auto filter = DecimatePolylineFilter::New();
    filter->SetTargetReduction(-3.0);
    Expect(filter->GetTargetReduction() == 0.0, "target reduction clamps to zero");
    filter->SetTargetReduction(3.0);
    Expect(filter->GetTargetReduction() == 1.0, "target reduction clamps to one");
    filter->SetMaximumError(-1.0);
    Expect(filter->GetMaximumError() == 0.0, "maximum error clamps to zero");
    filter->SetMaximumError(std::numeric_limits<double>::infinity());
    Expect(filter->GetMaximumError() == std::numeric_limits<double>::max(),
           "maximum error clamps to the largest finite double");
}

void TestMultiplePolylines() {
    std::puts("Test 4: multiple polylines are decimated independently");
    auto output = Execute(MakeMultiPolyline(), 0.5);
    Expect(output != nullptr, "multiple SurfaceMesh polyline cells are accepted");
    if (!output) return;

    Expect(output->GetNumberOfCells() == 2, "both polylines remain separate output cells");
    const igIndex* first = nullptr;
    const igIndex* second = nullptr;
    const int firstSize = output->GetCellPointIds(0, first);
    const int secondSize = output->GetCellPointIds(1, second);
    Expect(firstSize == 3 && secondSize == 3, "each six-point polyline is reduced to three points");
    if (firstSize == 3 && secondSize == 3) {
        Expect(first[2] == second[0], "shared retained endpoint uses one output point instance");
    }
    Expect(output->GetNumberOfPoints() == 5, "global point map avoids duplicating the shared endpoint");

    auto cellValues = FindIntAttribute(output, "CurveKind");
    Expect(cellValues && cellValues->GetNumberOfElements() == 2,
           "cell data contains one tuple per output polyline");
    if (cellValues && cellValues->GetNumberOfElements() == 2) {
        Expect(cellValues->GetValue(0) == 11 && cellValues->GetValue(1) == 22,
           "cell data maps from the corresponding input polyline cells");
    }
}

void TestClosedLoopAndMinimumSizes() {
    std::puts("Test 5: closed and two-point polyline minimum sizes");
    auto closedOutput = Execute(MakeClosedPolyline(), 1.0);
    Expect(closedOutput != nullptr, "closed loop execution succeeds");
    if (closedOutput) {
        const igIndex* ids = nullptr;
        const int size = closedOutput->GetCellPointIds(0, ids);
        Expect(size == 3, "closed loop stops at three connectivity entries like VTK");
        if (size == 3) Expect(ids[0] == ids[2], "closed loop keeps repeated start/end point");
    }

    auto line = SurfaceMesh::New();
    line->GetPoints()->AddPoint(Point(0.f, 0.f, 0.f));
    line->GetPoints()->AddPoint(Point(1.f, 0.f, 0.f));
    igIndex ids[2] = {0, 1};
    auto lineEdges = CellArray::New();
    lineEdges->AddCellIds(ids, 2);
    line->SetEdges(lineEdges);
    auto lineOutput = Execute(line, 1.0);
    Expect(lineOutput && lineOutput->GetNumberOfPoints() == 2, "a two-point line cannot be reduced further");
    if (lineOutput) Expect(lineOutput->GetCellType(0) == IG_LINE, "two-point output uses line cell type");
}

void TestUnsupportedInput() {
    std::puts("Test 6: ParaView-compatible input domain is enforced");
    auto mesh = UnstructuredMesh::New();
    mesh->GetPoints()->AddPoint(Point(0.f, 0.f, 0.f));
    mesh->GetPoints()->AddPoint(Point(1.f, 0.f, 0.f));
    igIndex lineIds[2] = {0, 1};
    mesh->AddCell(lineIds, 2, IG_LINE);
    auto filter = DecimatePolylineFilter::New();
    filter->SetInput(mesh);
    Expect(!DecimatePolylineFilter::CanProcessInput(mesh),
           "input support check rejects UnstructuredMesh line cells");
    Expect(!filter->Execute(), "UnstructuredMesh is rejected even when it contains line cells");

    auto surface = SurfaceMesh::New();
    surface->GetPoints()->AddPoint(Point(0.f, 0.f, 0.f));
    surface->GetPoints()->AddPoint(Point(1.f, 0.f, 0.f));
    surface->GetPoints()->AddPoint(Point(0.f, 1.f, 0.f));
    igIndex triangle[3] = {0, 1, 2};
    auto faces = CellArray::New();
    faces->AddCellIds(triangle, 3);
    surface->SetFaces(faces);
    surface->BuildEdges();
    Expect(!DecimatePolylineFilter::CanProcessInput(surface),
           "input support check rejects edges derived from polygons");
    auto surfaceFilter = DecimatePolylineFilter::New();
    surfaceFilter->SetInput(surface);
    Expect(!surfaceFilter->Execute(), "derived face edges are not mistaken for explicit polylines");

    auto mixed = SurfaceMesh::New();
    mixed->GetPoints()->AddPoint(Point(0.f, 0.f, 0.f));
    mixed->GetPoints()->AddPoint(Point(1.f, 0.f, 0.f));
    mixed->GetPoints()->AddPoint(Point(0.f, 1.f, 0.f));
    mixed->GetPoints()->AddPoint(Point(2.f, 0.5f, 0.f));
    mixed->GetPoints()->AddPoint(Point(3.f, 0.f, 0.f));
    auto mixedFaces = CellArray::New();
    mixedFaces->AddCellIds(triangle, 3);
    mixed->SetFaces(mixedFaces);
    igIndex explicitLine[3] = {2, 3, 4};
    auto mixedLines = CellArray::New();
    mixedLines->AddCellIds(explicitLine, 3);
    mixed->SetEdges(mixedLines);
    Expect(DecimatePolylineFilter::CanProcessInput(mixed),
           "input support check accepts explicit PolyData lines");
    auto mixedOutput = Execute(mixed, 0.5);
    Expect(mixedOutput != nullptr, "PolyData with both explicit lines and polygons is accepted");
    if (mixedOutput) {
        Expect(mixedOutput->GetNumberOfCells() == 1,
               "only the explicit line cell is emitted from mixed PolyData");
        Expect(mixedOutput->GetNumberOfPoints() == 2,
               "the explicit three-point line is decimated independently of polygons");
    }

    auto strategyFilter = DecimatePolylineFilter::New();
    strategyFilter->SetDecimationStrategy(DecimatePolylineFilter::DecimationStrategy::Angle);
    Expect(strategyFilter->GetDecimationStrategy() ==
                   DecimatePolylineFilter::DecimationStrategy::Angle,
           "ParaView angle strategy is exposed by the filter API");
    strategyFilter->SetDecimationStrategy(DecimatePolylineFilter::DecimationStrategy::CustomField);
    Expect(strategyFilter->GetDecimationStrategy() ==
                   DecimatePolylineFilter::DecimationStrategy::CustomField,
           "ParaView custom-field strategy is exposed by the filter API");
    strategyFilter->SetDecimationStrategy(DecimatePolylineFilter::DecimationStrategy::Distance);
    Expect(strategyFilter->GetDecimationStrategy() ==
                   DecimatePolylineFilter::DecimationStrategy::Distance,
           "ParaView distance strategy is exposed by the filter API");
}

void TestAllParaViewStrategies() {
    std::puts("Test 7: all ParaView 6.1.1 decimation strategies");
    auto angleOutput = Execute(MakeOpenSurfacePolyline(), 0.5,
                               std::numeric_limits<double>::max(),
                               DecimatePolylineFilter::DecimationStrategy::Angle);
    Expect(angleOutput && angleOutput->GetNumberOfPoints() == 5,
           "angle strategy reaches the requested reduction");

    auto customOutput = Execute(MakeOpenSurfacePolyline(), 0.5,
                                std::numeric_limits<double>::max(),
                                DecimatePolylineFilter::DecimationStrategy::CustomField,
                                "OriginalPointId");
    Expect(customOutput && customOutput->GetNumberOfPoints() == 5,
           "custom-field strategy reaches the requested reduction");

    auto missingFieldFilter = DecimatePolylineFilter::New();
    missingFieldFilter->SetInput(MakeOpenSurfacePolyline());
    missingFieldFilter->SetDecimationStrategy(
            DecimatePolylineFilter::DecimationStrategy::CustomField);
    missingFieldFilter->SetCustomFieldName("MissingPointArray");
    Expect(!missingFieldFilter->Execute(),
           "custom-field strategy rejects a missing point-data array");
}

void TestParaViewReferenceSequences() {
    std::puts("Test 8: retained vertices match ParaView 6.1.1");
    auto input = FileIO::ReadFile("Models/DecimatePolyline_Curve.vtk");
    Expect(input != nullptr, "ParaView reference POLYDATA can be read");
    if (!input) return;
    Expect(input->GetDataObjectType() == IG_SURFACE_MESH,
           "POLYDATA reference model is loaded as SurfaceMesh");
    Expect(DecimatePolylineFilter::CanProcessInput(input),
           "SurfaceMesh with explicit LINES is accepted by the filter");

    struct Reference {
        DecimatePolylineFilter::DecimationStrategy strategy;
        const char* fieldName;
        std::vector<int> retainedPointIds;
        const char* message;
    };

    const std::vector<Reference> references{
        {DecimatePolylineFilter::DecimationStrategy::Angle, nullptr,
         {0, 9, 39, 43, 57, 62, 72, 90, 93, 107, 121, 138, 141, 145, 150},
         "angle strategy retains the same vertices as ParaView"},
        {DecimatePolylineFilter::DecimationStrategy::CustomField, "OriginalPointId",
         {0, 8, 20, 34, 48, 64, 76, 88, 96, 106, 114, 124, 132, 143, 150},
         "custom-field strategy retains the same vertices as ParaView"},
        {DecimatePolylineFilter::DecimationStrategy::Distance, nullptr,
         {0, 10, 27, 37, 42, 56, 63, 73, 87, 92, 108, 123, 139, 144, 150},
         "distance strategy retains the same vertices as ParaView"},
    };

    for (const auto& reference : references) {
        auto output = Execute(input, 0.9, std::numeric_limits<double>::max(),
                              reference.strategy, reference.fieldName);
        Expect(output != nullptr, "reference decimation succeeds");
        if (!output) continue;
        Expect(output->GetNumberOfCells() == 1,
               "reference output contains one polyline");
        Expect(Matches(FindIntAttribute(output, "OriginalPointId"),
                       reference.retainedPointIds),
               reference.message);
        Expect(Matches(FindIntAttribute(output, "CurveKind"), {1}),
               "reference cell data is preserved");
    }
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    TestOpenPolylineAndAttributes();
    TestMaximumErrorAndParameterClamping();
    TestMultiplePolylines();
    TestClosedLoopAndMinimumSizes();
    TestUnsupportedInput();
    TestAllParaViewStrategies();
    TestParaViewReferenceSequences();
    std::printf("DecimatePolyline checks: %d, failures: %d\n",
                g_CheckCount, g_FailureCount);
    return g_FailureCount == 0 ? 0 : 1;
}
