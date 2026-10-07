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
    auto originalAttrCount = mesh->GetAttributeSet()->GetNumberOfAttributes();

    auto filter = iGame::PointCoordinatesFilter::New();
    filter->SetInput(mesh);
    if (!Check(filter->Execute(), "valid mesh must be processed")) { return false; }

    // Output must be a different object from the input (independent output node)
    auto output = filter->GetOutput();
    if (!Check(output.get() != mesh.get(),
               "filter must produce an independent output, not modify the input")) {
        return false;
    }
    if (!Check(mesh->GetAttributeSet()->GetNumberOfAttributes() == originalAttrCount,
               "the original input must not be modified by the filter")) {
        return false;
    }

    // Output must have the Coordinates attribute
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(output);
    if (!Check(outMesh != nullptr, "output must be an unstructured mesh")) { return false; }
    if (!Check(outMesh->GetCells().get() != originalCells.get(),
               "output must have its own cells (deep copied)")) {
        return false;
    }

    auto outAttributes = outMesh->GetAttributeSet();
    const int coordinateIndex = outAttributes->GetAttributeIndex("Coordinates");
    if (!Check(coordinateIndex >= 0, "Coordinates attribute must be present on output")) { return false; }

    auto& attribute = outAttributes->GetAttribute(coordinateIndex);
    if (!Check(attribute.GetType() == IG_VECTOR, "Coordinates must be a vector attribute")) { return false; }
    if (!Check(attribute.GetAttachmentType() == IG_POINT, "Coordinates must be attached to points")) { return false; }

    auto coordinates = iGame::DynamicCast<iGame::FloatArray>(attribute.GetPointer());
    if (!Check(coordinates && coordinates->GetDimension() == 3, "Coordinates must be a three-component FloatArray")) {
        return false;
    }
    if (!Check(coordinates->GetNumberOfElements() == outMesh->GetNumberOfPoints(),
               "coordinate tuple count must equal output point count")) {
        return false;
    }

    const float expected[15]{-2.5f, 1.25f, 0.0f, 0.0f, -3.5f, 2.0f, 4.75f, 0.5f,
                             -1.25f, 1.5f, 2.5f, 3.25f, -1.0f, 0.75f, 4.5f};
    for (IGsize i = 0; i < 15; ++i) {
        if (!Check(NearlyEqual(coordinates->GetValue(i), expected[i]), "coordinate values must match mesh points")) {
            return false;
        }
    }

    // Verify that repeated execution on the same input still produces valid independent output
    const auto outputAttrCount = outAttributes->GetNumberOfAttributes();
    auto filter2 = iGame::PointCoordinatesFilter::New();
    filter2->SetInput(mesh);
    if (!Check(filter2->Execute(), "repeated execution on original input must succeed")) { return false; }
    auto output2 = filter2->GetOutput();
    if (!Check(output2.get() != output.get(),
               "repeated execution must produce a new independent output")) {
        return false;
    }
    if (!Check(mesh->GetAttributeSet()->GetNumberOfAttributes() == originalAttrCount,
               "repeated execution must not modify the original input")) {
        return false;
    }

    return true;
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
