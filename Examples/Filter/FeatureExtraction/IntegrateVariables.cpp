#include "IntegrateVariables/iGameIntegrateVariablesFilter.h"
#include "iGameAttributeSet.h"
#include "iGameFileIO.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kTolerance = 1e-9;

bool Near(double actual, double expected, double tolerance = kTolerance) {
    return std::abs(actual - expected) <= tolerance;
}

void Check(bool condition, const std::string& message, bool& passed) {
    if (!condition) {
        std::cerr << "[IntegrateVariables] FAILED: " << message << '\n';
        passed = false;
    }
}

iGame::ArrayObject* FindArray(iGame::DataObject* data, const std::string& name,
                              IGenum attachment) {
    if (!data || !data->GetAttributeSet()) return nullptr;
    auto* attributes = data->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (!attribute.IsNone() && attribute.pointer &&
            attribute.attachmentType == attachment &&
            attribute.pointer->GetName() == name) {
            return attribute.pointer.GetPointer();
        }
    }
    return nullptr;
}

void AddScalarAttribute(const iGame::UnstructuredMesh::Pointer& mesh,
                        const std::string& name, IGenum attachment,
                        std::initializer_list<double> values) {
    auto array = iGame::DoubleArray::New();
    array->SetName(name);
    array->SetDimension(1);
    for (double value : values) array->AddValue(value);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, attachment, array);
}

void AddScalarAttribute(const iGame::UnstructuredMesh::Pointer& mesh,
                        const std::string& name, IGenum attachment,
                        const std::vector<double>& values) {
    auto array = iGame::DoubleArray::New();
    array->SetName(name);
    array->SetDimension(1);
    for (double value : values) array->AddValue(value);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, attachment, array);
}

void AddVectorAttribute(const iGame::UnstructuredMesh::Pointer& mesh,
                        const std::string& name, IGenum attachment,
                        std::initializer_list<double> values) {
    auto array = iGame::DoubleArray::New();
    array->SetName(name);
    array->SetDimension(2);
    auto it = values.begin();
    while (it != values.end()) {
        double tuple[2] = {*it++, *it++};
        array->AddElement(tuple);
    }
    mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, attachment, array);
}

// Expected values in these cases are from ParaView/VTK's documented linear
// integration decompositions, not from CellSizeFilter.  In particular, the
// 3-D reference values retain signed tetrahedron volumes.
bool TestSingleCellAgainstVTKReference(
        const std::string& label, IGenum cellType, int dimension,
        const std::vector<iGame::Point>& cellPoints,
        double expectedMeasure, double expectedPointIntegral,
        const std::array<double, 3>& expectedCenter,
        bool divideCellData = false) {
    bool passed = true;
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    std::vector<igIndex> pointIds;
    std::vector<double> pointValues;
    pointIds.reserve(cellPoints.size());
    pointValues.reserve(cellPoints.size());
    for (size_t i = 0; i < cellPoints.size(); ++i) {
        points->AddPoint(cellPoints[i]);
        pointIds.push_back(static_cast<igIndex>(i));
        pointValues.push_back(static_cast<double>(i + 1));
    }
    mesh->SetPoints(points);
    mesh->AddCell(pointIds.data(), static_cast<int>(pointIds.size()), cellType);
    AddScalarAttribute(mesh, "PointOrdinal", IG_POINT, pointValues);
    AddScalarAttribute(mesh, "CellValue", IG_CELL, {7.0});

    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetDivideAllCellDataByMeasure(divideCellData);
    filter->SetInput(mesh);
    Check(filter->Execute(), label + ": " + filter->GetMessage(), passed);
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(output != nullptr, label + ": output is missing", passed);
    if (!output) return false;

    const std::string measureName = dimension == 1 ? "Length" :
                                    dimension == 2 ? "Area" : "Volume";
    auto* measure = FindArray(output, measureName, IG_CELL);
    auto* pointIntegral = FindArray(output, "PointOrdinal", IG_POINT);
    auto* cellIntegral = FindArray(output, "CellValue", IG_CELL);
    constexpr double tolerance = 5e-6;
    Check(filter->GetIntegrationDimension() == dimension &&
                  filter->GetIntegratedCellCount() == 1,
          label + ": wrong integration dimension or cell count", passed);
    Check(measure && Near(measure->GetElementValue(0, 0),
                          expectedMeasure, tolerance),
          label + ": geometric measure differs from VTK", passed);
    Check(pointIntegral && Near(pointIntegral->GetElementValue(0, 0),
                                expectedPointIntegral, tolerance),
          label + ": point-data integral differs from VTK", passed);
    const double expectedCellIntegral =
            divideCellData ? 7.0 : 7.0 * expectedMeasure;
    Check(cellIntegral && Near(cellIntegral->GetElementValue(0, 0),
                               expectedCellIntegral, tolerance),
          label + ": cell-data integral differs from VTK", passed);
    const auto& center = output->GetPoints()->GetPoint(0);
    Check(Near(center[0], expectedCenter[0], tolerance) &&
                  Near(center[1], expectedCenter[1], tolerance) &&
                  Near(center[2], expectedCenter[2], tolerance),
          label + ": weighted center differs from VTK", passed);
    return passed;
}

bool TestWarpedQuad() {
    return TestSingleCellAgainstVTKReference(
            "warped quad", iGame::IG_QUAD, 2,
            {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
             {2.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
            2.532247551122990, 5.809851094745910,
            {1.038987706591832, 0.480506146704084, 1.0 / 3.0});
}

bool TestPolyLine() {
    // PolyLine is intentionally covered in this filter without changing the
    // existing CellSize implementation.
    return TestSingleCellAgainstVTKReference(
            "polyline", iGame::IG_POLY_LINE, 1,
            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
             {1.0f, 2.0f, 0.0f}, {4.0f, 2.0f, 0.0f}},
            6.0, 17.0,
            {5.0 / 3.0, 4.0 / 3.0, 0.0});
}

bool TestSurfaceMeshPolyLine() {
    bool passed = true;
    auto mesh = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 2.0f, 0.0f));
    points->AddPoint(iGame::Point(4.0f, 2.0f, 0.0f));
    mesh->SetPoints(points);
    mesh->SetFaces(iGame::CellArray::New());
    auto edges = iGame::CellArray::New();
    const igIndex ids[4] = {0, 1, 2, 3};
    edges->AddCellIds(ids, 4);
    mesh->SetEdges(edges);

    auto pointValue = iGame::DoubleArray::New();
    pointValue->SetName("PointOrdinal");
    pointValue->SetDimension(1);
    for (double value : {1.0, 2.0, 3.0, 4.0}) pointValue->AddValue(value);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, pointValue);
    auto cellValue = iGame::DoubleArray::New();
    cellValue->SetName("CellValue");
    cellValue->SetDimension(1);
    cellValue->AddValue(7.0);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, cellValue);

    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "SurfaceMesh polyline: " + filter->GetMessage(), passed);
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(output != nullptr, "SurfaceMesh polyline: output is missing", passed);
    if (!output) return false;

    auto* length = FindArray(output, "Length", IG_CELL);
    auto* pointIntegral = FindArray(output, "PointOrdinal", IG_POINT);
    auto* cellIntegral = FindArray(output, "CellValue", IG_CELL);
    Check(length && Near(length->GetElementValue(0, 0), 6.0),
          "SurfaceMesh polyline: Length should be 6", passed);
    Check(pointIntegral && Near(pointIntegral->GetElementValue(0, 0), 17.0),
          "SurfaceMesh polyline: point integral should be 17", passed);
    Check(cellIntegral && Near(cellIntegral->GetElementValue(0, 0), 42.0),
          "SurfaceMesh polyline: cell integral should be 42", passed);
    return passed;
}

bool TestPolygon() {
    return TestSingleCellAgainstVTKReference(
            "polygon", iGame::IG_POLYGON, 2,
            {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
             {3.0f, 1.0f, 0.0f}, {1.5f, 3.0f, 0.0f},
             {0.0f, 1.0f, 0.0f}},
            5.5, 14.5,
            {1.393939393939394, 1.151515151515151, 0.0});
}

bool TestWarpedPyramid() {
    // The 1--3 base diagonal is shorter.  VTK therefore uses tetrahedra
    // (0,1,3,4) and (1,2,3,4); the other diagonal gives a different result.
    return TestSingleCellAgainstVTKReference(
            "warped pyramid", iGame::IG_PYRAMID, 3,
            {{0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f},
             {4.0f, 4.0f, 1.0f}, {0.0f, 1.0f, 0.0f},
             {1.0f, 1.0f, 3.0f}},
            9.833333333333332, 33.416666666666664,
            {2.046610169491526, 1.296610169491526, 0.949152542372881});
}

bool TestWarpedPrism() {
    return TestSingleCellAgainstVTKReference(
            "warped prism", iGame::IG_PRISM, 3,
            {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
             {0.0f, 1.0f, 0.0f}, {0.2f, -0.1f, 1.1f},
             {2.4f, 0.2f, 1.5f}, {-0.2f, 1.2f, 0.8f}},
            1.391000000000000, 5.118416666666667,
            {0.824179247543733, 0.342253774263120, 0.616654684878984});
}

bool TestWarpedHexahedron() {
    return TestSingleCellAgainstVTKReference(
            "warped hexahedron", iGame::IG_HEXAHEDRON, 3,
            {{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
             {2.0f, 1.0f, 0.2f}, {0.0f, 1.0f, 0.0f},
             {0.1f, -0.1f, 1.0f}, {2.2f, 0.0f, 1.3f},
             {1.7f, 1.3f, 1.8f}, {-0.2f, 1.1f, 0.7f}},
            2.784000000000000, 12.546333333333333,
            {1.041349377394636, 0.541615780651341, 0.697820881226054});
}

bool TestInvertedTetraAndDivide() {
    // Swapping vertices 1 and 2 makes the VTK tetrahedron volume negative.
    // DivideAllCellDataByMeasure must still recover the original cell value.
    return TestSingleCellAgainstVTKReference(
            "inverted tetra", iGame::IG_TETRA, 3,
            {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
             {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            -1.0 / 6.0, -5.0 / 12.0,
            {0.25, 0.25, 0.25}, true);
}

bool TestLineIntegration() {
    bool passed = true;
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(2.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(3.0f, 0.0f, 0.0f));
    mesh->SetPoints(points);
    igIndex line0[2] = {0, 1};
    igIndex line1[2] = {1, 2};
    mesh->AddCell(line0, 2, iGame::IG_LINE);
    mesh->AddCell(line1, 2, iGame::IG_LINE);
    AddScalarAttribute(mesh, "PointValue", IG_POINT, {1.0, 3.0, 5.0});
    AddVectorAttribute(mesh, "PointVector", IG_POINT,
                       {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    AddScalarAttribute(mesh, "CellValue", IG_CELL, {10.0, 20.0});

    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), filter->GetMessage(), passed);
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(output != nullptr, "line output is not an UnstructuredMesh", passed);
    if (!output) return false;

    auto* pointValue = FindArray(output, "PointValue", IG_POINT);
    auto* pointVector = FindArray(output, "PointVector", IG_POINT);
    auto* cellValue = FindArray(output, "CellValue", IG_CELL);
    auto* length = FindArray(output, "Length", IG_CELL);
    Check(pointValue && Near(pointValue->GetElementValue(0, 0), 8.0),
          "line point scalar integral should be 8", passed);
    Check(pointVector && Near(pointVector->GetElementValue(0, 0), 8.0) &&
                              Near(pointVector->GetElementValue(0, 1), 11.0),
          "line point vector integral should be (8, 11)", passed);
    Check(cellValue && Near(cellValue->GetElementValue(0, 0), 40.0),
          "line cell scalar integral should be 40", passed);
    Check(length && Near(length->GetElementValue(0, 0), 3.0),
          "total Length should be 3", passed);
    Check(output->GetNumberOfPoints() == 1 && output->GetNumberOfCells() == 1,
          "ParaView-style output should contain one point and one vertex", passed);
    Check(Near(output->GetPoints()->GetPoint(0)[0], 1.5),
          "line weighted center should be x=1.5", passed);
    return passed;
}

bool TestMixedDimensionUsesHighest() {
    bool passed = true;
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(2.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 2.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 0.0f, 10.0f));
    mesh->SetPoints(points);
    igIndex triangle[3] = {0, 1, 2};
    igIndex line[2] = {0, 3};
    mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);
    mesh->AddCell(line, 2, iGame::IG_LINE);
    AddScalarAttribute(mesh, "PointValue", IG_POINT, {1.0, 3.0, 5.0, 100.0});
    AddScalarAttribute(mesh, "CellValue", IG_CELL, {7.0, 100.0});

    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), filter->GetMessage(), passed);
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(output != nullptr, "mixed-dimensional output is missing", passed);
    if (!output) return false;

    auto* pointValue = FindArray(output, "PointValue", IG_POINT);
    auto* cellValue = FindArray(output, "CellValue", IG_CELL);
    auto* area = FindArray(output, "Area", IG_CELL);
    Check(filter->GetIntegrationDimension() == 2 &&
                  filter->GetIntegratedCellCount() == 1,
          "only the highest-dimensional triangle should be integrated", passed);
    Check(pointValue && Near(pointValue->GetElementValue(0, 0), 6.0),
          "triangle point scalar integral should be 6", passed);
    Check(cellValue && Near(cellValue->GetElementValue(0, 0), 14.0),
          "lower-dimensional line cell data must be ignored", passed);
    Check(area && Near(area->GetElementValue(0, 0), 2.0),
          "total Area should be 2", passed);
    const auto& center = output->GetPoints()->GetPoint(0);
    Check(Near(center[0], 2.0 / 3.0, 1e-6) &&
                  Near(center[1], 2.0 / 3.0, 1e-6),
          "triangle weighted center should be its centroid", passed);
    return passed;
}

bool TestVolumeIntegration() {
    bool passed = true;
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    points->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));
    mesh->SetPoints(points);
    igIndex tetra[4] = {0, 1, 2, 3};
    igIndex triangle[3] = {0, 1, 2};
    mesh->AddCell(tetra, 4, iGame::IG_TETRA);
    mesh->AddCell(triangle, 3, iGame::IG_TRIANGLE);
    AddScalarAttribute(mesh, "PointValue", IG_POINT, {0.0, 1.0, 2.0, 3.0});
    AddScalarAttribute(mesh, "CellValue", IG_CELL, {12.0, 999.0});

    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), filter->GetMessage(), passed);
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(output != nullptr, "volume output is missing", passed);
    if (!output) return false;

    auto* pointValue = FindArray(output, "PointValue", IG_POINT);
    auto* cellValue = FindArray(output, "CellValue", IG_CELL);
    auto* volume = FindArray(output, "Volume", IG_CELL);
    Check(pointValue && Near(pointValue->GetElementValue(0, 0), 0.25),
          "tetra point scalar integral should be 0.25", passed);
    Check(cellValue && Near(cellValue->GetElementValue(0, 0), 2.0),
          "tetra cell scalar integral should be 2", passed);
    Check(volume && Near(volume->GetElementValue(0, 0), 1.0 / 6.0),
          "total Volume should be 1/6", passed);
    const auto& center = output->GetPoints()->GetPoint(0);
    Check(Near(center[0], 0.25, 1e-6) && Near(center[1], 0.25, 1e-6) &&
                  Near(center[2], 0.25, 1e-6),
          "tetra weighted center should be (0.25, 0.25, 0.25)", passed);
    return passed;
}

bool RunModelFile(const std::string& path) {
    auto input = iGame::FileIO::ReadFile(path);
    if (!input) {
        std::cerr << "[IntegrateVariables] cannot read: " << path << '\n';
        return false;
    }
    auto filter = iGame::IntegrateVariablesFilter::New();
    filter->SetInput(input);
    if (!filter->Execute()) {
        std::cerr << "[IntegrateVariables] " << path << ": "
                  << filter->GetMessage() << '\n';
        return false;
    }
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (!output || !output->GetPoints() || output->GetNumberOfPoints() != 1) {
        std::cerr << "[IntegrateVariables] invalid output: " << path << '\n';
        return false;
    }

    std::cout << std::setprecision(17);
    std::cout << "FILE\t" << path << '\n';
    const auto& center = output->GetPoints()->GetPoint(0);
    std::cout << "CENTER\t" << center[0] << '\t' << center[1] << '\t'
              << center[2] << '\n';
    auto* attributes = output->GetAttributeSet();
    for (IGsize i = 0; attributes && i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.IsNone() || !attribute.pointer || attribute.isDeleted) continue;
        std::cout << (attribute.attachmentType == IG_POINT ? "POINT" : "CELL")
                  << '\t' << attribute.pointer->GetName();
        for (int component = 0; component < attribute.pointer->GetDimension(); ++component) {
            std::cout << '\t' << attribute.pointer->GetElementValue(0, component);
        }
        std::cout << '\n';
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1) {
        bool passed = true;
        for (int i = 1; i < argc; ++i) passed = RunModelFile(argv[i]) && passed;
        return passed ? 0 : 1;
    }
    const bool passed = TestLineIntegration() &&
                        TestMixedDimensionUsesHighest() &&
                        TestVolumeIntegration() &&
                        TestPolyLine() &&
                        TestSurfaceMeshPolyLine() &&
                        TestPolygon() &&
                        TestWarpedQuad() &&
                        TestWarpedPyramid() &&
                        TestWarpedPrism() &&
                        TestWarpedHexahedron() &&
                        TestInvertedTetraAndDivide();
    std::cout << "[IntegrateVariables] " << (passed ? "ALL TESTS PASSED" : "TESTS FAILED")
              << '\n';
    return passed ? 0 : 1;
}
