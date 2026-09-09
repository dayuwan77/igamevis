#include <PointCoordinates/iGamePointCoordinatesFilter.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

constexpr const char* PointCoordinatesModelPath = "./Models/PointCoordinatesFilter_Test.vtk";

bool Check(bool condition, const std::string& message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; }
    return condition;
}

bool NearlyEqual(float lhs, float rhs) { return std::abs(lhs - rhs) < 1.0e-6f; }

bool TestInvalidInput() {
    auto filter = iGame::PointCoordinatesFilter::New();
    if (!Check(!filter->Execute(), "null input must be rejected")) { return false; }

    filter->SetInput(iGame::DataObject::New());
    return Check(!filter->Execute(), "input without points must be rejected");
}

bool TestEmptyPointSet() {
    auto mesh = iGame::UnstructuredMesh::New();
    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);

    if (!Check(filter->Execute(), "an empty point set should produce an empty coordinate array")) { return false; }

    auto coordinates = filter->GetCoordinatesArray();
    return Check(coordinates && coordinates->GetDimension() == 3 && coordinates->GetNumberOfElements() == 0,
                 "empty coordinate array must have zero tuples and three components");
}

bool TestCoordinatesArray() {
    std::cout << "Loading model: " << PointCoordinatesModelPath << '\n';
    auto dataObject = iGame::FileIO::ReadFile(PointCoordinatesModelPath);
    if (!Check(dataObject != nullptr, "the PointCoordinates test model must load automatically")) { return false; }

    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObject);
    if (!Check(mesh != nullptr, "the PointCoordinates test model must be an unstructured mesh")) { return false; }
    if (!Check(mesh->GetNumberOfPoints() == 5 && mesh->GetNumberOfCells() == 2,
               "the PointCoordinates test model must contain five points and two tetrahedra")) {
        return false;
    }

    auto originalCells = mesh->GetCells();

    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);
    if (!Check(filter->Execute(), "valid mesh must be processed")) { return false; }
    if (!Check(filter->GetOutput().get() == mesh.get(), "filter must preserve the input data object")) { return false; }
    if (!Check(mesh->GetCells().get() == originalCells.get(), "filter must preserve mesh topology")) { return false; }

    auto attributes = mesh->GetAttributeSet();
    const int coordinateIndex = attributes->GetAttributeIndex("Coordinates");
    if (!Check(coordinateIndex >= 0, "Coordinates attribute must be present")) { return false; }

    auto& attribute = attributes->GetAttribute(coordinateIndex);
    if (!Check(attribute.GetType() == IG_VECTOR, "Coordinates must be a vector attribute")) { return false; }
    if (!Check(attribute.GetAttachmentType() == IG_POINT, "Coordinates must be attached to points")) { return false; }

    auto coordinates = iGame::DynamicCast<iGame::FloatArray>(attribute.GetPointer());
    if (!Check(coordinates && coordinates->GetDimension() == 3, "Coordinates must be a three-component FloatArray")) {
        return false;
    }
    if (!Check(coordinates->GetNumberOfElements() == mesh->GetNumberOfPoints(),
               "coordinate tuple count must equal point count")) {
        return false;
    }

    const float expected[15]{-2.5f, 1.25f, 0.0f, 0.0f, -3.5f, 2.0f, 4.75f, 0.5f,
                             -1.25f, 1.5f, 2.5f, 3.25f, -1.0f, 0.75f, 4.5f};
    for (IGsize i = 0; i < 15; ++i) {
        if (!Check(NearlyEqual(coordinates->GetValue(i), expected[i]), "coordinate values must match mesh points")) {
            return false;
        }
    }

    const auto attributeCount = attributes->GetNumberOfAttributes();
    mesh->GetPoints()->SetPoint(0, 10.0f, 20.0f, 30.0f);
    if (!Check(filter->Execute(), "repeated execution after point edits must succeed")) { return false; }
    if (!Check(attributes->GetNumberOfAttributes() == attributeCount,
               "repeated execution must not create duplicate attributes")) {
        return false;
    }
    if (!Check(NearlyEqual(coordinates->GetValue(0), 10.0f) && NearlyEqual(coordinates->GetValue(1), 20.0f) &&
                       NearlyEqual(coordinates->GetValue(2), 30.0f),
               "coordinate array must stay synchronized with point edits")) {
        return false;
    }

    auto refreshedRange = attribute.GetDataRange();
    const float expectedMagnitude = std::sqrt(10.0f * 10.0f + 20.0f * 20.0f + 30.0f * 30.0f);
    return Check(refreshedRange && NearlyEqual(refreshedRange->GetValue(1), expectedMagnitude) &&
                         NearlyEqual(refreshedRange->GetValue(3), 10.0f) &&
                         NearlyEqual(refreshedRange->GetValue(5), 20.0f) &&
                         NearlyEqual(refreshedRange->GetValue(7), 30.0f),
                 "repeated execution must refresh magnitude and component ranges");
}

bool TestNameCollision() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));

    auto conflictingArray = iGame::FloatArray::New();
    conflictingArray->SetName("Coordinates");
    conflictingArray->SetDimension(1);
    conflictingArray->AddValue(1.0f);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, conflictingArray);

    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);
    return Check(!filter->Execute() && mesh->GetAttributeSet()->GetNumberOfAttributes() == 1,
                 "an existing unrelated array with the same name must be preserved and reported");
}

} // namespace

int main() {
    const bool passed = TestInvalidInput() && TestEmptyPointSet() && TestCoordinatesArray() && TestNameCollision();
    if (!passed) { return 1; }

    std::cout << "PointCoordinatesFilter tests passed.\n";
    return 0;
}
