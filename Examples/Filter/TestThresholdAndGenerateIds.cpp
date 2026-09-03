#include "Threshold/iGameThresholdFilter.h"
#include "iGameFileIO.h"
#include "GenerateIds/iGameGenerateIdsFilter.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

const char* AssociationName(iGame::ThresholdFilter::Association association) {
    return association == iGame::ThresholdFilter::Association::Cell ? "Cell" : "Point";
}

const char* DataTypeName(IGenum type) {
    switch (type) {
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_SURFACE_MESH: return "SurfaceMesh";
        case IG_VOLUME_MESH: return "VolumeMesh";
        case IG_POINT_SET: return "PointSet";
        default: return "Other";
    }
}

void PrintMeshSummary(const char* title, iGame::DataObject::Pointer obj) {
    std::cout << title << "\n";
    if (!obj) {
        std::cout << "  (null)\n";
        return;
    }

    auto mesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(obj);
    std::cout << "  type: " << DataTypeName(obj->GetDataObjectType()) << "\n";
    if (mesh) {
        std::cout << "  points: " << mesh->GetNumberOfPoints() << "\n";
        std::cout << "  cells: " << mesh->GetNumberOfCells() << "\n";
    } else if (obj->GetPoints()) {
        std::cout << "  points: " << obj->GetPoints()->GetNumberOfPoints() << "\n";
    }

    auto attrs = obj->GetAttributeSet();
    if (!attrs) {
        std::cout << "  attributes: none\n";
        return;
    }

    std::cout << "  attributes: " << attrs->GetNumberOfAttributes() << "\n";
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        std::cout << "    [" << i << "] " << attr.pointer->GetName()
                  << " type=" << (attr.type == IG_VECTOR ? "vector" : "scalar")
                  << " attach=" << (attr.attachmentType == IG_CELL ? "cell" : "point")
                  << " dim=" << attr.pointer->GetDimension()
                  << " count=" << attr.pointer->GetNumberOfElements() << "\n";
    }
}

iGame::AttributeSet::Attribute* FindAttribute(iGame::AttributeSet* attrs, const std::string& name) {
    if (!attrs) return nullptr;
    for (IGsize i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || !attr.pointer) continue;
        if (attr.pointer->GetName() == name) return &attr;
    }
    return nullptr;
}

bool ComputeScalarRange(iGame::ArrayObject::Pointer array, int dimension, double& minValue, double& maxValue) {
    if (!array || dimension < 0 || dimension >= array->GetDimension()) return false;
    const IGsize count = array->GetNumberOfElements();
    if (count == 0) return false;

    minValue = 1e300;
    maxValue = -1e300;
    IGsize finiteCount = 0;
    for (IGsize i = 0; i < count; ++i) {
        const double value = array->GetElementValue(i, dimension);
        if (!std::isfinite(value)) continue;
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
        ++finiteCount;
    }
    return finiteCount > 0;
}

iGame::DataObject::Pointer RunGenerateIds(iGame::DataObject::Pointer input, IGenum dataType,
                                          const std::string& arrayName) {
    auto filter = iGame::iGameGenerateIdsFilter::New(dataType);
    filter->SetInput(input);
    filter->SetArrayName(arrayName);
    filter->SetStartId(0);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] iGameGenerateIdsFilter Execute() failed for " << arrayName << "\n";
        return nullptr;
    }
    auto output = filter->GetOutput();
    if (!output) {
        std::cerr << "[FAIL] iGameGenerateIdsFilter produced null output for " << arrayName << "\n";
        return nullptr;
    }
    return output;
}

} // namespace

int main(int argc, char** argv) {
    // 默认使用仓库内置测试数据(相对 Examples 运行目录的 ./Models/,由
    // iGameCopyExampleAssets 自动拷贝);也可通过 argv[1] 传入任意数据集。
    const std::string fileName = argc > 1 ? argv[1] : "./Models/ThresholdTestData.vtk";

    std::cout << "Reading: " << fileName << std::endl;
    auto input = iGame::FileIO::ReadFile(fileName);
    if (!input) {
        std::cerr << "[FAIL] Failed to read VTK file.\n";
        return 1;
    }
    PrintMeshSummary("[Input]", input);

    auto withPointIds = RunGenerateIds(input, IG_POINT, "PointIds");
    if (!withPointIds) return 1;
    PrintMeshSummary("[GenerateIds PointIds]", withPointIds);

    auto withCellIds = RunGenerateIds(withPointIds, IG_CELL, "CellIds");
    if (!withCellIds) return 1;
    PrintMeshSummary("[GenerateIds CellIds]", withCellIds);

    auto attrs = withCellIds->GetAttributeSet();
    auto curvature = FindAttribute(attrs, "Curvature");
    if (!curvature) {
        std::cerr << "[FAIL] Point scalar 'Curvature' not found.\n";
        return 1;
    }

    double minValue = 0.0;
    double maxValue = 0.0;
    if (!ComputeScalarRange(curvature->pointer, 0, minValue, maxValue)) {
        std::cerr << "[FAIL] Could not compute Curvature range.\n";
        return 1;
    }
    const double span = maxValue - minValue;
    const double lower = minValue + 0.2 * span;
    const double upper = minValue + 0.8 * span;
    std::cout << "[Threshold] scalar=Curvature attach=" << AssociationName(iGame::ThresholdFilter::Association::Point)
              << " range=[" << lower << ", " << upper << "] (data min=" << minValue << ", max=" << maxValue << ")\n";

    auto threshold = iGame::ThresholdFilter::New();
    threshold->SetInput(withCellIds);
    threshold->SetScalarData(curvature->pointer, iGame::ThresholdFilter::Association::Point, 0);
    threshold->SetThreshold(lower, upper);
    threshold->SetBoundaryMode(iGame::ThresholdFilter::BoundaryMode::Closed);
    threshold->SetPointEvaluation(iGame::ThresholdFilter::PointEvaluation::AllScalars);
    if (!threshold->Execute()) {
        std::cerr << "[FAIL] ThresholdFilter Execute() failed.\n";
        return 1;
    }

    auto output = threshold->GetOutput();
    PrintMeshSummary("[Threshold Curvature]", output);
    if (!output) {
        std::cerr << "[FAIL] ThresholdFilter produced null output.\n";
        return 1;
    }

    auto outMesh = iGame::UnstructuredMesh::TransDataObjToUnstructuredMesh(output);
    if (!outMesh || outMesh->GetNumberOfCells() == 0) {
        std::cerr << "[FAIL] Threshold output has no cells.\n";
        return 1;
    }

    auto outPointIds = FindAttribute(output->GetAttributeSet(), "PointIds");
    auto outCellIds = FindAttribute(output->GetAttributeSet(), "CellIds");
    if (!outPointIds || !outCellIds) {
        std::cerr << "[FAIL] Threshold output is missing generated Id arrays.\n";
        return 1;
    }

    std::cout << "[PASS] GenerateIds + Threshold completed.\n";
    return 0;
}
