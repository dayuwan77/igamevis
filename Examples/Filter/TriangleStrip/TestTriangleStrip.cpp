#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <DataProcessing/iGameMeshTriangulationFilter.h>
#include <TriangleStrip/iGameTriangleStripFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{

using namespace iGame;

constexpr const char* ModelFileName =
        "ContourExtraction_cylinder_UnstructedGrid.vtk";
constexpr const char* PointScalarName = "TriangleStripTestPointScalar";
constexpr const char* PointVectorName = "TriangleStripTestPointVector";
constexpr const char* CellScalarName = "TriangleStripTestCellScalar";

void Check(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

void AddAttributeFixtures(const SurfaceMesh::Pointer& mesh) {
    auto pointScalars = DoubleArray::New();
    pointScalars->SetName(PointScalarName);
    pointScalars->SetDimension(1);
    pointScalars->Reserve(mesh->GetNumberOfPoints());
    for (IGsize pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId) {
        pointScalars->AddValue(static_cast<double>(pointId) + 0.25);
    }

    auto pointVectors = FloatArray::New();
    pointVectors->SetName(PointVectorName);
    pointVectors->SetDimension(3);
    pointVectors->Reserve(mesh->GetNumberOfPoints());
    for (IGsize pointId = 0; pointId < mesh->GetNumberOfPoints(); ++pointId) {
        const float id = static_cast<float>(pointId);
        pointVectors->AddElement3(id, id + 1.0f, id + 2.0f);
    }

    auto cellScalars = IntArray::New();
    cellScalars->SetName(CellScalarName);
    cellScalars->SetDimension(1);
    cellScalars->Reserve(mesh->GetNumberOfFaces());
    for (IGsize faceId = 0; faceId < mesh->GetNumberOfFaces(); ++faceId) {
        cellScalars->AddValue(static_cast<int>(faceId) + 1000);
    }

    auto* attributes = mesh->GetAttributeSet();
    attributes->AddAttribute(IG_SCALAR, IG_POINT, pointScalars);
    attributes->AddAttribute(IG_VECTOR, IG_POINT, pointVectors);
    attributes->AddAttribute(IG_SCALAR, IG_CELL, cellScalars);
}

void ValidateAttributes(const TriangleStripFilter::Pointer& filter,
                        const SurfaceMesh::Pointer& input,
                        const SurfaceMesh::Pointer& output) {
    auto* inputAttributes = input->GetAttributeSet();
    auto* outputAttributes = output->GetAttributeSet();
    Check(inputAttributes != nullptr && outputAttributes != nullptr,
          "Input or output AttributeSet is null.");
    Check(inputAttributes != outputAttributes,
          "TriangleStripFilter reused the input AttributeSet container.");

    for (const char* name: {PointScalarName, PointVectorName}) {
        const int inputIndex = inputAttributes->GetAttributeIndex(name);
        const int outputIndex = outputAttributes->GetAttributeIndex(name);
        Check(inputIndex >= 0 && outputIndex >= 0,
              std::string("Missing point attribute: ") + name);

        auto& inputAttribute = inputAttributes->GetAttribute(inputIndex);
        auto& outputAttribute = outputAttributes->GetAttribute(outputIndex);
        Check(outputAttribute.attachmentType == IG_POINT,
              std::string("Wrong point attachment for: ") + name);
        Check(outputAttribute.type == inputAttribute.type,
              std::string("Point attribute type changed for: ") + name);
        Check(outputAttribute.pointer->GetArrayType() ==
                      inputAttribute.pointer->GetArrayType(),
              std::string("Point array storage type changed for: ") + name);
        Check(outputAttribute.pointer->GetDimension() ==
                      inputAttribute.pointer->GetDimension(),
              std::string("Point array dimension changed for: ") + name);
        Check(outputAttribute.pointer->GetNumberOfElements() ==
                      inputAttribute.pointer->GetNumberOfElements(),
              std::string("Point tuple count changed for: ") + name);

        for (IGsize pointId = 0;
             pointId < inputAttribute.pointer->GetNumberOfElements(); ++pointId) {
            for (int component = 0;
                 component < inputAttribute.pointer->GetDimension(); ++component) {
                const double expected =
                        inputAttribute.pointer->GetElementValue(pointId, component);
                const double actual =
                        outputAttribute.pointer->GetElementValue(pointId, component);
                Check(std::abs(expected - actual) < 1e-9,
                      std::string("Point attribute value changed for: ") + name);
            }
        }
    }

    const int inputCellIndex = inputAttributes->GetAttributeIndex(CellScalarName);
    const int outputCellIndex = outputAttributes->GetAttributeIndex(CellScalarName);
    Check(inputCellIndex >= 0 && outputCellIndex >= 0,
          "Missing remapped cell attribute.");
    auto& inputCellAttribute = inputAttributes->GetAttribute(inputCellIndex);
    auto& outputCellAttribute = outputAttributes->GetAttribute(outputCellIndex);
    Check(outputCellAttribute.attachmentType == IG_CELL,
          "Cell attribute has the wrong attachment type.");
    Check(outputCellAttribute.pointer.get() != inputCellAttribute.pointer.get(),
          "Cell attribute array was shared instead of remapped.");
    Check(outputCellAttribute.pointer->GetArrayType() == IG_IntArray,
          "Cell attribute storage type was not preserved.");
    Check(outputCellAttribute.pointer->GetNumberOfElements() ==
                  output->GetNumberOfFaces(),
          "Cell attribute tuple count does not match output faces.");

    std::vector<igIndex> outputSourceFaceIds;
    for (const auto& stripFaceIds: filter->GetStripSourceFaceIds()) {
        outputSourceFaceIds.insert(outputSourceFaceIds.end(),
                                   stripFaceIds.begin(), stripFaceIds.end());
    }
    Check(outputSourceFaceIds.size() ==
                  static_cast<std::size_t>(output->GetNumberOfFaces()),
          "Test could not reconstruct the output/source face mapping.");
    for (IGsize outputFaceId = 0;
         outputFaceId < output->GetNumberOfFaces(); ++outputFaceId) {
        const igIndex sourceFaceId =
                outputSourceFaceIds[static_cast<std::size_t>(outputFaceId)];
        const igIndex* sourcePointIds = nullptr;
        const igIndex* outputPointIds = nullptr;
        const int sourcePointCount =
                input->GetFaces()->GetCellIds(sourceFaceId, sourcePointIds);
        const int outputPointCount =
                output->GetFaces()->GetCellIds(outputFaceId, outputPointIds);
        Check(sourcePointIds != nullptr && outputPointIds != nullptr &&
                      sourcePointCount == 3 && outputPointCount == 3,
              "Cannot compare source and output triangle geometry.");
        std::array<igIndex, 3> sourceTriangle{
                sourcePointIds[0], sourcePointIds[1], sourcePointIds[2]};
        std::array<igIndex, 3> outputTriangle{
                outputPointIds[0], outputPointIds[1], outputPointIds[2]};
        std::sort(sourceTriangle.begin(), sourceTriangle.end());
        std::sort(outputTriangle.begin(), outputTriangle.end());
        Check(sourceTriangle == outputTriangle,
              "Cell attribute mapping does not match output triangle geometry.");

        const double expected =
                inputCellAttribute.pointer->GetElementValue(sourceFaceId, 0);
        const double actual =
                outputCellAttribute.pointer->GetElementValue(outputFaceId, 0);
        Check(expected == actual,
              "Cell attribute value does not match its source face.");
    }
}

std::filesystem::path ResolveModelPath(int argc, char* argv[]) {
    if (argc == 2) { return std::filesystem::path(argv[1]); }

    const std::vector<std::filesystem::path> candidates{
            std::filesystem::path("Models") / ModelFileName,
            std::filesystem::path("Examples") / "Models" / ModelFileName,
    };

    for (const auto& candidate: candidates) {
        if (std::filesystem::exists(candidate)) { return candidate; }
    }
    return candidates.front();
}

SurfaceMesh::Pointer ExtractAndTriangulate(DataObject::Pointer input) {
    auto surfaceFilter = ConvertToSurfaceMeshFilter::New();
    surfaceFilter->SetInput(input);
    surfaceFilter->SetConvertMethod(
            ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
    Check(surfaceFilter->Execute(), "Failed to extract the volume boundary.");

    auto surface = DynamicCast<SurfaceMesh>(surfaceFilter->GetOutput());
    Check(surface != nullptr, "Surface extraction returned a non-surface object.");
    Check(surface->GetNumberOfFaces() > 0,
          "Surface extraction returned no boundary faces.");

    std::cout << "  Extracted surface: points=" << surface->GetNumberOfPoints()
              << ", faces=" << surface->GetNumberOfFaces() << '\n';

    auto triangulation = MeshTriangulationFilter::New();
    triangulation->SetInput(surface);
    Check(triangulation->Execute(), "Failed to triangulate the extracted surface.");

    auto triangles = DynamicCast<SurfaceMesh>(triangulation->GetOutput());
    Check(triangles != nullptr, "Triangulation returned a non-surface object.");
    Check(triangles->GetNumberOfFaces() > 0,
          "Triangulation returned no triangles.");

    for (IGsize faceId = 0; faceId < triangles->GetNumberOfFaces(); ++faceId) {
        Check(triangles->GetFaces()->GetCellSize(faceId) == 3,
              "Triangulation output still contains a non-triangle face.");
    }

    std::cout << "  Triangulated surface: points=" << triangles->GetNumberOfPoints()
              << ", triangles=" << triangles->GetNumberOfFaces() << '\n';
    return triangles;
}

void ValidateStripCoverage(const TriangleStripFilter::Pointer& filter,
                           const SurfaceMesh::Pointer& input) {
    auto* strips = filter->GetStrips();
    Check(strips != nullptr, "Triangle-strip array is null.");
    Check(strips->GetNumberOfCells() > 0, "No triangle strips were generated.");
    Check(filter->GetNumberOfStrips() == strips->GetNumberOfCells(),
          "GetNumberOfStrips disagrees with the strip cell array.");
    Check(filter->GetLongestStripLength() > 1,
          "The model was reduced only to one-triangle strips.");
    Check(filter->GetLongestStripLength() <=
                  static_cast<IGsize>(filter->GetMaximumLength()),
          "A generated strip exceeds MaximumLength.");

    const auto& sourceFaceIds = filter->GetStripSourceFaceIds();
    Check(sourceFaceIds.size() ==
                  static_cast<std::size_t>(strips->GetNumberOfCells()),
          "Strip/source-face mapping count is inconsistent.");

    std::vector<bool> covered(
            static_cast<std::size_t>(input->GetNumberOfFaces()), false);
    IGsize coveredTriangleCount = 0;

    for (IGsize stripId = 0; stripId < strips->GetNumberOfCells(); ++stripId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = strips->GetCellIds(stripId, pointIds);
        Check(pointIds != nullptr && pointCount >= 3,
              "A triangle strip has fewer than three points.");
        Check(pointCount <= filter->GetMaximumLength() + 2,
              "A triangle strip point sequence exceeds MaximumLength + 2.");

        const IGsize stripTriangleCount = static_cast<IGsize>(pointCount - 2);
        const auto& mappedFaces =
                sourceFaceIds[static_cast<std::size_t>(stripId)];
        Check(mappedFaces.size() ==
                      static_cast<std::size_t>(stripTriangleCount),
              "A strip's source-face mapping has the wrong length.");

        for (int i = 0; i + 2 < pointCount; ++i) {
            Check(pointIds[i] != pointIds[i + 1] &&
                          pointIds[i] != pointIds[i + 2] &&
                          pointIds[i + 1] != pointIds[i + 2],
                  "A generated strip contains a degenerate triangle.");
        }

        for (const igIndex faceId: mappedFaces) {
            Check(faceId >= 0 &&
                          static_cast<IGsize>(faceId) < input->GetNumberOfFaces(),
                  "A strip references an invalid source face.");
            const auto faceIndex = static_cast<std::size_t>(faceId);
            Check(!covered[faceIndex],
                  "A source triangle is used by more than one strip.");
            covered[faceIndex] = true;
        }
        coveredTriangleCount += stripTriangleCount;
    }

    std::cout << "  Triangles before strip conversion: "
              << input->GetNumberOfFaces() << '\n'
              << "  Triangles after strip conversion (sum(pointCount - 2)): "
              << coveredTriangleCount << '\n';

    Check(coveredTriangleCount == input->GetNumberOfFaces(),
          "The strips do not cover every input triangle exactly once.");
    for (const bool wasCovered: covered) {
        Check(wasCovered, "At least one input triangle is missing from the strips.");
    }

    Check(filter->GetPassThroughPolys() != nullptr,
          "Pass-through polygon array is null.");
    Check(filter->GetPassThroughPolys()->GetNumberOfCells() == 0,
          "A triangulated input unexpectedly produced pass-through polygons.");

    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Check(output != nullptr, "TriangleStripFilter returned no SurfaceMesh output.");
    Check(output->GetNumberOfFaces() == input->GetNumberOfFaces(),
          "Reconstructed output has a different triangle count.");
    Check(output->GetPoints().get() == input->GetPoints().get(),
          "TriangleStripFilter did not preserve the shared point array.");
    ValidateAttributes(filter, input, output);
    std::cout << "  Attributes: new AttributeSet, PointData preserved, "
                 "CellData remapped\n";
}

void TestTriangleStripGeneration(const SurfaceMesh::Pointer& triangles) {
    AddAttributeFixtures(triangles);
    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(false);

    Check(filter->Execute(), "TriangleStripFilter::Execute failed.");
    ValidateStripCoverage(filter, triangles);

    Check(filter->GetNumberOfStrips() < triangles->GetNumberOfFaces(),
          "The filter did not combine any adjacent triangles.");

    const double averageTrianglesPerStrip =
            static_cast<double>(triangles->GetNumberOfFaces()) /
            static_cast<double>(filter->GetNumberOfStrips());
    std::cout << "  Triangle strips: count=" << filter->GetNumberOfStrips()
              << ", longest=" << filter->GetLongestStripLength()
              << ", average triangles/strip=" << averageTrianglesPerStrip
              << '\n';
}

SurfaceMesh::Pointer MakeTrianglePatch(const SurfaceMesh::Pointer& triangles) {
    const igIndex* pointIds = nullptr;
    const int pointCount = triangles->GetFaces()->GetCellIds(0, pointIds);
    Check(pointIds != nullptr && pointCount == 3,
          "Cannot obtain the first triangle for the polyline test.");

    auto patchFaces = CellArray::New();
    patchFaces->AddCellIds(pointIds, pointCount);

    auto patch = SurfaceMesh::New();
    patch->SetName("TriangleStripPolylinePatch");
    patch->SetPoints(triangles->GetPoints());
    patch->SetFaces(patchFaces);
    return patch;
}

void TestContiguousPolylineJoining(const SurfaceMesh::Pointer& triangles) {
    auto patch = MakeTrianglePatch(triangles);

    auto separate = TriangleStripFilter::New();
    separate->SetInput(patch);
    separate->SetJoinContiguousSegments(false);
    Check(separate->Execute(),
          "TriangleStripFilter failed with separate boundary segments.");

    auto* separateLines = separate->GetPolyLines();
    Check(separateLines != nullptr, "Separate polyline array is null.");
    Check(separateLines->GetNumberOfCells() == 3,
          "A one-triangle patch must have three boundary segments.");
    for (IGsize lineId = 0; lineId < separateLines->GetNumberOfCells();
         ++lineId) {
        Check(separateLines->GetCellSize(lineId) == 2,
              "Join disabled: a boundary segment is not a two-point line.");
    }

    auto joined = TriangleStripFilter::New();
    joined->SetInput(patch);
    joined->SetJoinContiguousSegments(true);
    Check(joined->Execute(),
          "TriangleStripFilter failed while joining boundary segments.");

    auto* joinedLines = joined->GetPolyLines();
    Check(joinedLines != nullptr, "Joined polyline array is null.");
    Check(joinedLines->GetNumberOfCells() == 1,
          "The three contiguous boundary segments were not joined.");

    const igIndex* joinedPointIds = nullptr;
    const int joinedPointCount = joinedLines->GetCellIds(0, joinedPointIds);
    Check(joinedPointIds != nullptr && joinedPointCount == 4,
          "The joined triangle boundary must contain four point IDs.");
    Check(joinedPointIds[0] == joinedPointIds[joinedPointCount - 1],
          "The joined triangle boundary is not closed.");

    std::unordered_set<igIndex> distinctPointIds;
    for (int i = 0; i + 1 < joinedPointCount; ++i) {
        Check(joinedPointIds[i] != joinedPointIds[i + 1],
              "The joined polyline contains a zero-length segment.");
        distinctPointIds.insert(joinedPointIds[i]);
    }
    Check(distinctPointIds.size() == 3,
          "The joined triangle boundary does not contain three distinct points.");

    std::cout << "  Boundary polylines: before join="
              << separateLines->GetNumberOfCells()
              << ", after join=" << joinedLines->GetNumberOfCells()
              << ", joined point IDs=" << joinedPointCount << '\n';
}

void TestPassThroughPolygonAttributes(const SurfaceMesh::Pointer& triangles) {
    Check(triangles->GetNumberOfPoints() >= 4,
          "Not enough points for the pass-through polygon test.");

    const igIndex pointIds[4]{0, 1, 2, 3};
    auto faces = CellArray::New();
    faces->AddCellIds(pointIds, 4);

    auto polygon = SurfaceMesh::New();
    polygon->SetName("TriangleStripPassThroughPolygon");
    polygon->SetPoints(triangles->GetPoints());
    polygon->SetFaces(faces);
    AddAttributeFixtures(polygon);

    auto filter = TriangleStripFilter::New();
    filter->SetInput(polygon);
    Check(filter->Execute(),
          "TriangleStripFilter failed for a pass-through polygon.");
    Check(filter->GetNumberOfStrips() == 0,
          "A non-triangle polygon unexpectedly generated a strip.");
    Check(filter->GetPassThroughPolys()->GetNumberOfCells() == 1,
          "The non-triangle polygon was not passed through.");

    auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Check(output != nullptr && output->GetNumberOfFaces() == 1,
          "Pass-through polygon output is invalid.");
    Check(output->GetAttributeSet() != polygon->GetAttributeSet(),
          "Pass-through output reused the input AttributeSet container.");

    auto* outputAttributes = output->GetAttributeSet();
    const int pointAttributeId =
            outputAttributes->GetAttributeIndex(PointScalarName);
    const int cellAttributeId =
            outputAttributes->GetAttributeIndex(CellScalarName);
    Check(pointAttributeId >= 0 && cellAttributeId >= 0,
          "Pass-through output lost point or cell attributes.");
    auto& pointAttribute = outputAttributes->GetAttribute(pointAttributeId);
    auto& cellAttribute = outputAttributes->GetAttribute(cellAttributeId);
    Check(pointAttribute.pointer->GetNumberOfElements() ==
                  polygon->GetNumberOfPoints(),
          "Pass-through point attribute tuple count changed.");
    Check(cellAttribute.pointer->GetArrayType() == IG_IntArray &&
                  cellAttribute.pointer->GetNumberOfElements() == 1 &&
                  cellAttribute.pointer->GetElementValue(0, 0) == 1000.0,
          "Pass-through cell attribute was not copied from its source face.");
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [model.vtk]\n";
        return 2;
    }

    try {
        const auto modelPath = ResolveModelPath(argc, argv);
        std::cout << "[RUN] TriangleStripFilter integration test\n"
                  << "  Model: " << modelPath.string() << '\n';
        Check(std::filesystem::exists(modelPath),
              "The requested VTK model does not exist.");

        auto input = FileIO::ReadFile(modelPath.string());
        Check(input != nullptr, "FileIO::ReadFile returned null.");
        Check(input->GetDataObjectType() == IG_UNSTRUCTURED_MESH,
              "The requested model is not an UnstructuredMesh.");

        auto unstructured = DynamicCast<UnstructuredMesh>(input);
        Check(unstructured != nullptr, "Failed to cast the input mesh.");
        Check(unstructured->GetNumberOfCells() > 0,
              "The input UnstructuredMesh contains no cells.");
        std::cout << "  Input: points=" << unstructured->GetNumberOfPoints()
                  << ", cells=" << unstructured->GetNumberOfCells() << '\n';

        auto triangles = ExtractAndTriangulate(input);

        std::cout << "[RUN] triangle-strip generation\n";
        TestTriangleStripGeneration(triangles);
        std::cout << "[PASS] triangle-strip generation\n";

        std::cout << "[RUN] contiguous-polyline joining\n";
        TestContiguousPolylineJoining(triangles);
        std::cout << "[PASS] contiguous-polyline joining\n";

        std::cout << "[RUN] pass-through polygon attributes\n";
        TestPassThroughPolygonAttributes(triangles);
        std::cout << "[PASS] pass-through polygon attributes\n";

        std::cout << "All TriangleStripFilter tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
