#include <DataProcessing/OverlappingCellsDetector/iGameOverlappingCellsDetectorFilter.h>
#include <iGameStructuredMesh.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

using CellPoints = std::vector<iGame::Point>;

void AddCell(const iGame::UnstructuredMesh::Pointer& mesh, const CellPoints& points, IGenum cellType) {
    std::vector<igIndex> pointIds(points.size());
    for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
        pointIds[pointIndex] = mesh->AddPoint(points[pointIndex]);
    }
    mesh->AddCell(pointIds.data(), static_cast<int>(pointIds.size()), cellType);
}

void AddVolumeCell(const iGame::Points::Pointer& points, const iGame::CellArray::Pointer& volumes,
                   const CellPoints& cellPoints) {
    std::vector<igIndex> pointIds(cellPoints.size());
    for (std::size_t pointIndex = 0; pointIndex < cellPoints.size(); ++pointIndex) {
        pointIds[pointIndex] = points->AddPoint(cellPoints[pointIndex]);
    }
    volumes->AddCellIds(pointIds.data(), static_cast<int>(pointIds.size()));
}

bool RunCase(const char* caseName, const std::vector<std::pair<CellPoints, IGenum>>& cells,
             const std::vector<igIndex>& expectedCounts) {
    auto mesh = iGame::UnstructuredMesh::New();
    for (const auto& cell : cells) AddCell(mesh, cell.first, cell.second);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        std::cerr << caseName << ": filter execution failed\n";
        return false;
    }
    if (filter->GetNumberOfOverlapsPerCell() != expectedCounts) {
        std::cerr << caseName << ": unexpected NumberOfOverlapsPerCell values\n";
        return false;
    }

    // 验证正式输出：输入网格上附加了与 VTK 同名的 cell scalar。
    auto* attribute = mesh->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_CELL,
            iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
    if (attribute == nullptr || attribute->GetNumberOfElements() != expectedCounts.size()) {
        std::cerr << caseName << ": output cell scalar is missing\n";
        return false;
    }

    std::cout << "[PASS] " << caseName << '\n';
    return true;
}

bool RunUnsupportedCase() {
    auto mesh = iGame::UnstructuredMesh::New();
    AddCell(mesh, {{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                    iGame::Point(0.0f, 1.0f, 0.0f)}},
            iGame::IG_TRIANGLE);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (filter->Execute() || filter->GetLastError().empty()) {
        std::cerr << "unsupported triangle: expected a safe failure with an error message\n";
        return false;
    }
    std::cout << "[PASS] unsupported triangle reports an error\n";
    return true;
}

bool RunVolumeMeshCase(const CellPoints& firstCell, const CellPoints& secondCell) {
    auto mesh = iGame::VolumeMesh::New();
    auto points = iGame::Points::New();
    auto volumes = iGame::CellArray::New();
    AddVolumeCell(points, volumes, firstCell);
    AddVolumeCell(points, volumes, secondCell);
    mesh->SetPoints(points);
    mesh->SetVolumes(volumes);

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() || filter->GetNumberOfOverlapsPerCell() != std::vector<igIndex>{1, 1}) {
        std::cerr << "VolumeMesh: expected overlapping cells to produce [1, 1]\n";
        return false;
    }
    auto* attribute = mesh->GetAttributeSet()->GetArrayPointer(
            IG_SCALAR, IG_CELL,
            iGame::OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
    if (attribute == nullptr || attribute->GetNumberOfElements() != 2) {
        std::cerr << "VolumeMesh: output cell scalar is missing\n";
        return false;
    }
    std::cout << "[PASS] overlapping VolumeMesh hexahedra\n";
    return true;
}

bool RunStructuredMeshCase() {
    // 3 x 2 x 2 个点构成两个仅共享面的六面体：这是三维 StructuredMesh 的标准连接关系。
    auto mesh = iGame::StructuredMesh::New();
    igIndex dimensions[3]{3, 2, 2};
    mesh->SetDimensionSize(dimensions);
    auto points = iGame::Points::New();
    for (int z = 0; z < 2; ++z)
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x) points->AddPoint(iGame::Point(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
    mesh->SetPoints(points);
    mesh->GenStructuredCellConnectivities();

    auto filter = iGame::OverlappingCellsDetectorFilter::New();
    filter->SetInput(mesh);
    if (!filter->Execute() || filter->GetNumberOfOverlapsPerCell() != std::vector<igIndex>{0, 0}) {
        std::cerr << "StructuredMesh: face-touching hexahedra must produce [0, 0]\n";
        return false;
    }
    std::cout << "[PASS] StructuredMesh face-touching hexahedra\n";
    return true;
}

} // namespace

int main() {
    const CellPoints referenceTetra{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                     iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, 1.0f)}};
    const CellPoints overlappingTetra{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                       iGame::Point(0.2f, 1.2f, 0.2f), iGame::Point(0.2f, 0.2f, 1.2f)}};
    const CellPoints referenceHex{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                   iGame::Point(1.0f, 1.0f, 0.0f), iGame::Point(0.0f, 1.0f, 0.0f),
                                   iGame::Point(0.0f, 0.0f, 1.0f), iGame::Point(1.0f, 0.0f, 1.0f),
                                   iGame::Point(1.0f, 1.0f, 1.0f), iGame::Point(0.0f, 1.0f, 1.0f)}};
    const CellPoints overlappingHex{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                     iGame::Point(1.2f, 1.2f, 0.2f), iGame::Point(0.2f, 1.2f, 0.2f),
                                     iGame::Point(0.2f, 0.2f, 1.2f), iGame::Point(1.2f, 0.2f, 1.2f),
                                     iGame::Point(1.2f, 1.2f, 1.2f), iGame::Point(0.2f, 1.2f, 1.2f)}};
    const CellPoints referencePrism{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                     iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, 1.0f),
                                     iGame::Point(1.0f, 0.0f, 1.0f), iGame::Point(0.0f, 1.0f, 1.0f)}};
    const CellPoints overlappingPrism{{iGame::Point(0.2f, 0.2f, 0.2f), iGame::Point(1.2f, 0.2f, 0.2f),
                                       iGame::Point(0.2f, 1.2f, 0.2f), iGame::Point(0.2f, 0.2f, 1.2f),
                                       iGame::Point(1.2f, 0.2f, 1.2f), iGame::Point(0.2f, 1.2f, 1.2f)}};
    const CellPoints referencePyramid{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                       iGame::Point(1.0f, 1.0f, 0.0f), iGame::Point(0.0f, 1.0f, 0.0f),
                                       iGame::Point(0.5f, 0.5f, 1.0f)}};
    const CellPoints overlappingPyramid{{iGame::Point(0.2f, 0.2f, 0.1f), iGame::Point(1.2f, 0.2f, 0.1f),
                                         iGame::Point(1.2f, 1.2f, 0.1f), iGame::Point(0.2f, 1.2f, 0.1f),
                                         iGame::Point(0.7f, 0.7f, 1.1f)}};

    bool passed = true;
    passed &= RunCase("disjoint tetrahedra", {{referenceTetra, iGame::IG_TETRA},
                                               {{{iGame::Point(2.0f, 0.0f, 0.0f), iGame::Point(3.0f, 0.0f, 0.0f),
                                                  iGame::Point(2.0f, 1.0f, 0.0f), iGame::Point(2.0f, 0.0f, 1.0f)}},
                                                iGame::IG_TETRA}},
                      {0, 0});
    passed &= RunCase("overlapping tetrahedra", {{referenceTetra, iGame::IG_TETRA}, {overlappingTetra, iGame::IG_TETRA}}, {1, 1});
    passed &= RunCase("face-touching tetrahedra", {{referenceTetra, iGame::IG_TETRA},
                                                     {{{iGame::Point(0.0f, 0.0f, 0.0f), iGame::Point(1.0f, 0.0f, 0.0f),
                                                        iGame::Point(0.0f, 1.0f, 0.0f), iGame::Point(0.0f, 0.0f, -1.0f)}},
                                                      iGame::IG_TETRA}},
                      {0, 0});
    passed &= RunCase("overlapping hexahedra", {{referenceHex, iGame::IG_HEXAHEDRON}, {overlappingHex, iGame::IG_HEXAHEDRON}}, {1, 1});
    passed &= RunCase("overlapping prisms", {{referencePrism, iGame::IG_PRISM}, {overlappingPrism, iGame::IG_PRISM}}, {1, 1});
    passed &= RunCase("overlapping pyramids", {{referencePyramid, iGame::IG_PYRAMID}, {overlappingPyramid, iGame::IG_PYRAMID}}, {1, 1});
    passed &= RunVolumeMeshCase(referenceHex, overlappingHex);
    passed &= RunStructuredMeshCase();
    passed &= RunUnsupportedCase();
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
