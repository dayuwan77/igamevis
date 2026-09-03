#include <PointAndCellIds/iGamePointAndCellIdsFilter.h>

#include "iGameAttributeSet.h"
#include "iGameDataObject.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGameUnstructuredMesh.h"

#include <iostream>
#include <string>

namespace
{

bool Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

iGame::LongLongArray::Pointer FindIds(iGame::DataObject::Pointer object,
                                      const std::string& name,
                                      IGenum attachmentType) {
    if (!object || !object->GetAttributeSet()) {
        return nullptr;
    }

    auto attributes = object->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);

        if (attribute.IsNone() ||
            attribute.GetType() != IG_SCALAR ||
            attribute.GetAttachmentType() != attachmentType ||
            attribute.GetPointer()->GetName() != name) {
            continue;
        }

        return iGame::DynamicCast<iGame::LongLongArray>(attribute.GetPointer());
    }

    return nullptr;
}

bool CheckIds(const iGame::LongLongArray::Pointer& ids, IGsize count) {
    if (!Check(ids != nullptr, "ID array must exist")) {
        return false;
    }

    if (!Check(ids->GetDimension() == 1, "ID array must have one component") ||
        !Check(ids->GetNumberOfElements() == count, "ID array size is incorrect")) {
        return false;
    }

    for (IGsize i = 0; i < count; ++i) {
        if (!Check(ids->GetValue(i) == static_cast<long long>(i),
                   "ID value is incorrect")) {
            return false;
        }
    }

    return true;
}

iGame::UnstructuredMesh::Pointer CreateMesh() {
    auto mesh = iGame::UnstructuredMesh::New();

    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f));
    mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f));

    igIndex cell[4]{0, 1, 2, 3};
    mesh->AddCell(cell, 4, iGame::IG_TETRA);

    return mesh;
}

bool TestInvalidInput() {
    auto filter = iGame::PointAndCellIdsFilter::New();

    if (!Check(!filter->Execute(), "null input must fail")) {
        return false;
    }

    filter->SetInput(iGame::DataObject::New());
    return Check(!filter->Execute(), "non-PointSet input must fail");
}

bool TestDefaultGeneration() {
    auto mesh = CreateMesh();
    auto cells = mesh->GetCells();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(mesh);

    if (!Check(filter->Execute(), "default generation must succeed") ||
        !Check(filter->GetOutput().get() == mesh.get(), "output must preserve input object") ||
        !Check(mesh->GetCells().get() == cells.get(), "mesh topology must remain unchanged")) {
        return false;
    }

    auto pointIds = FindIds(mesh, "vtkPointIds", IG_POINT);
    auto cellIds = FindIds(mesh, "vtkCellIds", IG_CELL);

    return CheckIds(pointIds, mesh->GetNumberOfPoints()) &&
           CheckIds(cellIds, mesh->GetNumberOfCells());
}

bool TestGenerationOptions() {
    {
        auto mesh = CreateMesh();

        auto filter = iGame::PointAndCellIdsFilter::New();
        filter->SetInput(mesh);
        filter->SetGenerateCellIds(false);

        if (!Check(filter->Execute(), "point-only generation must succeed")) {
            return false;
        }

        if (!Check(FindIds(mesh, "vtkPointIds", IG_POINT) != nullptr,
                   "point IDs must exist")) {
            return false;
        }

        if (!Check(FindIds(mesh, "vtkCellIds", IG_CELL) == nullptr,
                   "cell IDs must not exist")) {
            return false;
        }
    }

    {
        auto mesh = CreateMesh();

        auto filter = iGame::PointAndCellIdsFilter::New();
        filter->SetInput(mesh);
        filter->SetGeneratePointIds(false);

        if (!Check(filter->Execute(), "cell-only generation must succeed")) {
            return false;
        }

        if (!Check(FindIds(mesh, "vtkPointIds", IG_POINT) == nullptr,
                   "point IDs must not exist")) {
            return false;
        }

        if (!Check(FindIds(mesh, "vtkCellIds", IG_CELL) != nullptr,
                   "cell IDs must exist")) {
            return false;
        }
    }

    return true;
}

bool TestCustomNames() {
    auto mesh = CreateMesh();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetPointIdsArrayName("PointIds");
    filter->SetCellIdsArrayName("CellIds");

    if (!Check(filter->Execute(), "custom names must succeed")) {
        return false;
    }

    return CheckIds(FindIds(mesh, "PointIds", IG_POINT), mesh->GetNumberOfPoints()) &&
           CheckIds(FindIds(mesh, "CellIds", IG_CELL), mesh->GetNumberOfCells());
}

bool TestRepeatedExecution() {
    auto mesh = CreateMesh();

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(mesh);

    if (!Check(filter->Execute(), "first execution must succeed")) {
        return false;
    }

    const auto attributeCount = mesh->GetAttributeSet()->GetNumberOfAttributes();

    if (!Check(filter->Execute(), "second execution must succeed")) {
        return false;
    }

    return Check(mesh->GetAttributeSet()->GetNumberOfAttributes() == attributeCount,
                 "repeated execution must not create duplicate attributes");
}

bool TestNameCollision() {
    auto mesh = CreateMesh();

    auto array = iGame::FloatArray::New();
    array->SetName("vtkPointIds");
    array->SetDimension(1);
    array->Resize(mesh->GetNumberOfPoints());

    mesh->GetAttributeSet()->AddScalar(IG_POINT, array);

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(mesh);

    return Check(!filter->Execute(),
                 "conflicting attribute type must be rejected");
}

bool TestPointSetCellIds() {
    auto points = iGame::PointSet::New();
    points->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(points);
    filter->SetGeneratePointIds(false);
    filter->SetGenerateCellIds(true);

    return Check(!filter->Execute(),
                 "cell IDs on plain PointSet must fail");
}

bool TestRealModel() {
    auto object = iGame::FileIO::ReadFile(
            "Models/ClipTest_Plane_UnstructuredGrid.vtk");

    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (!Check(mesh != nullptr, "real test model must load")) {
        return false;
    }

    auto filter = iGame::PointAndCellIdsFilter::New();
    filter->SetInput(mesh);

    if (!Check(filter->Execute(), "real model generation must succeed")) {
        return false;
    }

    auto pointIds = FindIds(mesh, "vtkPointIds", IG_POINT);
    auto cellIds = FindIds(mesh, "vtkCellIds", IG_CELL);

    return CheckIds(pointIds, mesh->GetNumberOfPoints()) &&
           CheckIds(cellIds, mesh->GetNumberOfCells());
}

}

int main() {
    const bool passed =
            TestInvalidInput() &&
            TestDefaultGeneration() &&
            TestGenerationOptions() &&
            TestCustomNames() &&
            TestRepeatedExecution() &&
            TestNameCollision() &&
            TestPointSetCellIds() &&
            TestRealModel();

    if (!passed) {
        return 1;
    }

    std::cout << "PointAndCellIdsFilter tests passed.\n";
    return 0;
}