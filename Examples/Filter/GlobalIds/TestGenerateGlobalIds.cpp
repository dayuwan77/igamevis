#include <GlobalIds/iGameGenerateGlobalIdsFilter.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameDataObject.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

using namespace iGame;

std::string TestModelFilePath = "Models/ContourExtraction_cylinder_UnstructedGrid.vtk";

void Check(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

Points::Pointer MakePoints(IGsize count) {
    auto points = Points::New();
    for (IGsize i = 0; i < count; ++i) {
        points->AddPoint(static_cast<float>(i), static_cast<float>(i % 2), 0.0f);
    }
    return points;
}

SurfaceMesh::Pointer MakeSurfaceMesh(
        const Points::Pointer& points,
        const std::vector<std::array<igIndex, 3>>& triangles) {
    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);

    auto faces = CellArray::New();
    for (const auto& triangle: triangles) {
        faces->AddCellId3(triangle[0], triangle[1], triangle[2]);
    }
    mesh->SetFaces(faces);
    return mesh;
}

AttributeSet::Attribute* FindAttribute(DataObject::Pointer object, const std::string& name,
                                       IGenum attachmentType) {
    if (!object || !object->GetAttributeSet()) { return nullptr; }

    auto* attributes = object->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.isDeleted || !attribute.pointer) { continue; }
        if (attribute.attachmentType != attachmentType) { continue; }
        if (attribute.pointer->GetName() == name) { return &attribute; }
    }
    return nullptr;
}

DoubleArray::Pointer FindDoubleArray(DataObject::Pointer object, const std::string& name,
                                     IGenum attachmentType) {
    auto* attribute = FindAttribute(object, name, attachmentType);
    if (!attribute) { return nullptr; }
    return DynamicCast<DoubleArray>(attribute->pointer);
}

void PrintIdArray(const std::string& label, const DoubleArray::Pointer& ids) {
    std::cout << "  " << label << " = ";
    if (!ids) {
        std::cout << "<missing>\n";
        return;
    }

    const auto previousPrecision = std::cout.precision();
    std::cout << '[' << std::setprecision(17);
    const auto* values = ids->RawPointer();
    for (IGsize i = 0; i < ids->GetNumberOfElements(); ++i) {
        if (i != 0) { std::cout << ", "; }
        std::cout << values[i];
    }
    std::cout << "]\n";
    std::cout.precision(previousPrecision);
}

void PrintIdRange(const std::string& label, const DoubleArray::Pointer& ids) {
    std::cout << "  " << label << " = ";
    if (!ids) {
        std::cout << "<missing>\n";
        return;
    }

    const IGsize count = ids->GetNumberOfElements();
    if (count == 0) {
        std::cout << "[] (count=0)\n";
        return;
    }

    const auto previousPrecision = std::cout.precision();
    const auto* values = ids->RawPointer();
    std::cout << '[' << std::setprecision(17) << values[0] << ", " << values[count - 1]
              << "] (count=" << count << ")\n";
    std::cout.precision(previousPrecision);
}

void CheckIdRange(const DoubleArray::Pointer& ids, IGsize count, iguIndex64 start,
                  const std::string& label) {
    Check(ids != nullptr, label + " is missing or is not a DoubleArray.");
    Check(ids->GetDimension() == 1, label + " must have dimension 1.");
    Check(ids->GetNumberOfElements() == count, label + " has an unexpected element count.");

    const auto* values = ids->RawPointer();
    for (IGsize i = 0; i < count; ++i) {
        const auto expected = start + static_cast<iguIndex64>(i);
        Check(values[i] == static_cast<double>(expected),
              label + " has an unexpected value at index " + std::to_string(i) + ".");
    }
}
/**
 * @brief 读取真实 UnstructuredGrid，生成带偏移的全局 ID，并测试 ExistingIdPolicy。
 */
void TestFileModelAndExistingPolicies() {
    constexpr iguIndex64 initialPointOffset = 10000;
    constexpr iguIndex64 initialCellOffset = 2000;
    constexpr IGsize expectedPointCount = 8499;
    constexpr IGsize expectedCellCount = 7472;

    std::cout << "  Model file: " << TestModelFilePath << '\n';
    auto mesh = FileIO::ReadFile(TestModelFilePath);
    Check(mesh != nullptr, "FileIO::ReadFile failed for the requested VTK model.");
    std::cout << "  Loaded DataObject type: " << mesh->GetDataObjectType() << '\n';
    Check(mesh->GetDataObjectType() == IG_UNSTRUCTURED_MESH,
          "The requested VTK model is not an UnstructuredMesh.");

    IGsize pointCount = 0;
    IGsize cellCount = 0;
    const bool counted = GenerateGlobalIdsFilter::CountEntities(mesh, pointCount, cellCount);
    std::cout << "  Model counts: points=" << pointCount << ", cells=" << cellCount
              << ", result=" << (counted ? "success" : "failure") << '\n';
    Check(counted, "CountEntities failed for the requested VTK model.");
    Check(pointCount == expectedPointCount && cellCount == expectedCellCount,
          "The requested VTK model has unexpected entity counts.");

    auto initial = GenerateGlobalIdsFilter::New();
    initial->SetInput(mesh);
    initial->SetOffsets(initialPointOffset, initialCellOffset);
    const bool initialSucceeded = initial->Execute();
    std::cout << "  Initial generation with offsets (" << initialPointOffset << ", "
              << initialCellOffset << "): "
              << (initialSucceeded ? "success" : "failure") << '\n';
    Check(initialSucceeded, "Initial generation failed: " + initial->GetMessage());
    Check(initial->GetOutput().get() == mesh.get(), "The filter must return its in-place input.");

    auto pointIds = FindDoubleArray(mesh, "GlobalPointIds", IG_POINT);
    auto cellIds = FindDoubleArray(mesh, "GlobalCellIds", IG_CELL);
    PrintIdRange("Initial GlobalPointIds range", pointIds);
    PrintIdRange("Initial GlobalCellIds range", cellIds);
    CheckIdRange(pointIds, pointCount, initialPointOffset, "Point IDs");
    CheckIdRange(cellIds, cellCount, initialCellOffset, "Cell IDs");
    Check(FindAttribute(mesh, "GlobalPointIds", IG_POINT)->type == IG_SCALAR,
          "Point IDs must be registered as a scalar attribute.");
    Check(FindAttribute(mesh, "GlobalCellIds", IG_CELL)->type == IG_SCALAR,
          "Cell IDs must be registered as a scalar attribute.");

    const auto nextPointOffset =
            initialPointOffset + static_cast<iguIndex64>(pointCount);
    const auto nextCellOffset = initialCellOffset + static_cast<iguIndex64>(cellCount);
    std::cout << "  Configured start offsets after Execute: point=" << initial->GetPointOffset()
              << ", cell=" << initial->GetCellOffset() << '\n';
    std::cout << "  Completed next offsets: point=" << nextPointOffset
              << ", cell=" << nextCellOffset << '\n';
    Check(initial->GetPointOffset() == initialPointOffset &&
                  initial->GetCellOffset() == initialCellOffset,
          "Execute unexpectedly modified the configured start offsets.");
    Check(nextPointOffset == 18499 && nextCellOffset == 9472,
          "The completed next offsets are incorrect.");

    auto rejectExisting = GenerateGlobalIdsFilter::New();
    rejectExisting->SetInput(mesh);
    rejectExisting->SetOffsets(initialPointOffset, initialCellOffset);
    const bool errorPolicySucceeded = rejectExisting->Execute();
    std::cout << "  ExistingIdPolicy::Error result: "
              << (errorPolicySucceeded ? "success" : "failure (expected)") << '\n';
    Check(!errorPolicySucceeded, "ExistingIdPolicy::Error must reject existing IDs.");
    Check(rejectExisting->GetOutput() == nullptr, "A failed execution must clear its output.");
    Check(FindDoubleArray(mesh, "GlobalPointIds", IG_POINT).get() == pointIds.get(),
          "A rejected execution changed the point array.");
    Check(FindDoubleArray(mesh, "GlobalCellIds", IG_CELL).get() == cellIds.get(),
          "A rejected execution changed the cell array.");

    auto keep = GenerateGlobalIdsFilter::New();
    keep->SetInput(mesh);
    keep->SetOffsets(initialPointOffset, initialCellOffset);
    keep->SetExistingIdPolicy(GenerateGlobalIdsFilter::ExistingIdPolicy::KeepExisting);
    const bool keepSucceeded = keep->Execute();
    std::cout << "  ExistingIdPolicy::KeepExisting matching result: "
              << (keepSucceeded ? "success" : "failure") << '\n';
    Check(keepSucceeded, "KeepExisting rejected valid arrays: " + keep->GetMessage());
    Check(FindDoubleArray(mesh, "GlobalPointIds", IG_POINT).get() == pointIds.get(),
          "KeepExisting replaced the valid point array.");
    Check(FindDoubleArray(mesh, "GlobalCellIds", IG_CELL).get() == cellIds.get(),
          "KeepExisting replaced the valid cell array.");

    auto rejectMismatchedRange = GenerateGlobalIdsFilter::New();
    rejectMismatchedRange->SetInput(mesh);
    rejectMismatchedRange->SetOffsets(initialPointOffset + 1, initialCellOffset);
    rejectMismatchedRange->SetExistingIdPolicy(
            GenerateGlobalIdsFilter::ExistingIdPolicy::KeepExisting);
    const bool mismatchedKeepSucceeded = rejectMismatchedRange->Execute();
    std::cout << "  KeepExisting with mismatched point offset " << initialPointOffset + 1 << ": "
              << (mismatchedKeepSucceeded ? "success" : "failure (expected)") << '\n';
    Check(!mismatchedKeepSucceeded, "KeepExisting accepted a mismatched point range.");
    Check(FindDoubleArray(mesh, "GlobalPointIds", IG_POINT).get() == pointIds.get(),
          "A failed KeepExisting execution changed the input.");

    auto replace = GenerateGlobalIdsFilter::New();
    replace->SetInput(mesh);
    replace->SetOffsets(nextPointOffset, nextCellOffset);
    replace->SetExistingIdPolicy(GenerateGlobalIdsFilter::ExistingIdPolicy::Replace);
    const bool replaceSucceeded = replace->Execute();
    std::cout << "  ExistingIdPolicy::Replace with offsets (" << nextPointOffset << ", "
              << nextCellOffset << "): "
              << (replaceSucceeded ? "success" : "failure") << '\n';
    Check(replaceSucceeded, "Replace failed: " + replace->GetMessage());

    auto replacementPointIds = FindDoubleArray(mesh, "GlobalPointIds", IG_POINT);
    auto replacementCellIds = FindDoubleArray(mesh, "GlobalCellIds", IG_CELL);
    PrintIdRange("Replacement GlobalPointIds range", replacementPointIds);
    PrintIdRange("Replacement GlobalCellIds range", replacementCellIds);
    Check(replacementPointIds.get() != pointIds.get(), "Replace retained the old point array.");
    Check(replacementCellIds.get() != cellIds.get(), "Replace retained the old cell array.");
    CheckIdRange(replacementPointIds, pointCount, nextPointOffset, "Replacement point IDs");
    CheckIdRange(replacementCellIds, cellCount, nextCellOffset, "Replacement cell IDs");
}
/**
 * @brief 模拟多进程调用场景：对多个不相交的 SurfaceMesh 分区生成全局 ID，并测试偏移量的正确性。
 */
void TestSimulatedProcessOffsets() {
    // Simulate three disjoint process partitions. In a real parallel caller,
    // these prefixes would be produced by a process-wide exclusive scan.
    const std::vector<SurfaceMesh::Pointer> partitions{
            MakeSurfaceMesh(MakePoints(3), {{{0, 1, 2}}}),
            MakeSurfaceMesh(MakePoints(4), {{{0, 1, 2}}, {{0, 2, 3}}}),
            MakeSurfaceMesh(MakePoints(5), {{{0, 1, 2}}, {{0, 2, 3}}, {{0, 3, 4}}}),
    };

    constexpr iguIndex64 pointBase = 5000;
    constexpr iguIndex64 cellBase = 7000;
    iguIndex64 pointPrefix = 0;
    iguIndex64 cellPrefix = 0;

    for (IGsize rank = 0; rank < partitions.size(); ++rank) {
        IGsize localPointCount = 0;
        IGsize localCellCount = 0;
        const bool counted = GenerateGlobalIdsFilter::CountEntities(
                partitions[rank], localPointCount, localCellCount);
        std::cout << "  Simulated rank " << rank << ": local points=" << localPointCount
                  << ", local cells=" << localCellCount
                  << ", count result=" << (counted ? "success" : "failure") << '\n';
        Check(counted,
              "CountEntities failed for simulated rank " + std::to_string(rank) + ".");

        const auto pointOffset = pointBase + pointPrefix;
        const auto cellOffset = cellBase + cellPrefix;
        std::cout << "    process offsets: point=" << pointOffset << ", cell=" << cellOffset
                  << '\n';

        auto filter = GenerateGlobalIdsFilter::New();
        filter->SetInput(partitions[rank]);
        filter->SetOffsets(pointOffset, cellOffset);
        const bool generated = filter->Execute();
        std::cout << "    generation result: " << (generated ? "success" : "failure") << '\n';
        Check(generated,
              "Generation failed for simulated rank " + std::to_string(rank) + ": " +
                      filter->GetMessage());

        auto pointIds = FindDoubleArray(partitions[rank], "GlobalPointIds", IG_POINT);
        auto cellIds = FindDoubleArray(partitions[rank], "GlobalCellIds", IG_CELL);
        PrintIdArray("rank " + std::to_string(rank) + " GlobalPointIds", pointIds);
        PrintIdArray("rank " + std::to_string(rank) + " GlobalCellIds", cellIds);
        CheckIdRange(pointIds, localPointCount, pointOffset,
                     "Simulated rank " + std::to_string(rank) + " point IDs");
        CheckIdRange(cellIds, localCellCount, cellOffset,
                     "Simulated rank " + std::to_string(rank) + " cell IDs");

        pointPrefix += static_cast<iguIndex64>(localPointCount);
        cellPrefix += static_cast<iguIndex64>(localCellCount);
    }

    std::cout << "  Simulated global totals: points=" << pointPrefix
              << ", cells=" << cellPrefix << '\n';
    Check(pointPrefix == 12, "The simulated global point count is incorrect.");
    Check(cellPrefix == 6, "The simulated global cell count is incorrect.");
}
/**
 * @brief 测试对共享 Points 对象的多个 SurfaceMesh 生成全局 ID，并测试父容器的修改时间是否晚于叶节点。
 */
void TestCompositeSharedPointsAndParentTimes() {
    auto sharedPoints = MakePoints(4);
    auto first = MakeSurfaceMesh(sharedPoints, {{{0, 1, 2}}});
    auto second = MakeSurfaceMesh(sharedPoints, {{{0, 2, 3}}});

    auto middle = DataObject::New();
    middle->AddSubDataObject(first);
    middle->AddSubDataObject(second);

    auto root = DataObject::New();
    root->AddSubDataObject(middle);

    IGsize pointCount = 0;
    IGsize cellCount = 0;
    const bool counted = GenerateGlobalIdsFilter::CountEntities(root, pointCount, cellCount);
    std::cout << "  Composite counts: unique points=" << pointCount << ", cells=" << cellCount
              << ", result=" << (counted ? "success" : "failure") << '\n';
    Check(counted,
          "CountEntities failed for a composite hierarchy.");
    Check(pointCount == 4, "A shared Points object was counted more than once.");
    Check(cellCount == 2, "Composite cell count is incorrect.");

    const auto rootTimeBefore = root->GetMTime().GetMTime();
    const auto middleTimeBefore = middle->GetMTime().GetMTime();

    auto filter = GenerateGlobalIdsFilter::New();
    filter->SetInput(root);
    filter->SetOffsets(1000, 2000);
    const bool generated = filter->Execute();
    std::cout << "  Composite generation with offsets (1000, 2000): "
              << (generated ? "success" : "failure") << '\n';
    Check(generated, "Composite generation failed: " + filter->GetMessage());
    Check(filter->GetOutput().get() == root.get(), "Composite output is not the input root.");

    auto firstPointIds = FindDoubleArray(first, "GlobalPointIds", IG_POINT);
    auto secondPointIds = FindDoubleArray(second, "GlobalPointIds", IG_POINT);
    auto firstCellIds = FindDoubleArray(first, "GlobalCellIds", IG_CELL);
    auto secondCellIds = FindDoubleArray(second, "GlobalCellIds", IG_CELL);
    PrintIdArray("first leaf GlobalPointIds", firstPointIds);
    PrintIdArray("second leaf GlobalPointIds", secondPointIds);
    PrintIdArray("first leaf GlobalCellIds", firstCellIds);
    PrintIdArray("second leaf GlobalCellIds", secondCellIds);
    CheckIdRange(firstPointIds, 4, 1000,
                 "First shared point IDs");
    CheckIdRange(secondPointIds, 4, 1000,
                 "Second shared point IDs");
    CheckIdRange(firstCellIds, 1, 2000,
                 "First cell IDs");
    CheckIdRange(secondCellIds, 1, 2001,
                 "Second cell IDs");

    const auto rootTime = root->GetMTime().GetMTime();
    const auto middleTime = middle->GetMTime().GetMTime();
    std::cout << "  Modification times: first=" << first->GetMTime().GetMTime()
              << ", second=" << second->GetMTime().GetMTime() << ", middle=" << middleTime
              << ", root=" << rootTime << '\n';
    Check(rootTime > rootTimeBefore && middleTime > middleTimeBefore,
          "Composite containers were not marked modified.");
    Check(rootTime > middleTime, "The root must be updated after its child container.");
    Check(middleTime > first->GetMTime().GetMTime() && middleTime > second->GetMTime().GetMTime(),
          "A container must be updated after its modified leaves.");
    Check(root->GetAttributeSet()->GetNumberOfAttributes() == 0 &&
                  middle->GetAttributeSet()->GetNumberOfAttributes() == 0,
          "Global ID arrays must only be stored on leaf objects.");
}
/**
 * @brief 测试是否会出现后续叶节点的验证失败但前面叶节点已被修改的情况。
 */
void TestFailureIsAtomicAcrossLeaves() {
    auto first = MakeSurfaceMesh(MakePoints(3), {{{0, 1, 2}}});
    auto second = MakeSurfaceMesh(MakePoints(3), {{{0, 1, 2}}});
    auto root = DataObject::New();
    root->AddSubDataObject(first);
    root->AddSubDataObject(second);

    auto existing = DoubleArray::New();
    existing->SetName("GlobalPointIds");
    existing->SetDimension(1);
    existing->Resize(3);
    existing->RawPointer()[0] = 0.0;
    existing->RawPointer()[1] = 1.0;
    existing->RawPointer()[2] = 2.0;
    second->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, existing);
    PrintIdArray("pre-existing second-leaf GlobalPointIds", existing);

    auto filter = GenerateGlobalIdsFilter::New();
    filter->SetInput(root);
    filter->SetGenerateCellIds(false);
    const bool generated = filter->Execute();
    auto firstIdsAfterFailure = FindDoubleArray(first, "GlobalPointIds", IG_POINT);
    auto secondIdsAfterFailure = FindDoubleArray(second, "GlobalPointIds", IG_POINT);
    std::cout << "  Composite generation with a later conflict: "
              << (generated ? "success" : "failure (expected)") << '\n';
    PrintIdArray("first leaf after rejected execution", firstIdsAfterFailure);
    PrintIdArray("second leaf after rejected execution", secondIdsAfterFailure);
    Check(!generated, "The default policy accepted an existing ID array.");
    Check(FindAttribute(first, "GlobalPointIds", IG_POINT) == nullptr,
          "A later validation failure partially modified an earlier leaf.");
    Check(secondIdsAfterFailure.get() == existing.get(),
          "A failed composite execution changed the pre-existing array.");
}

void CheckSupportedLeaf(DataObject::Pointer object, IGsize expectedPointCount,
                        IGsize expectedCellCount, const std::string& label) {
    IGsize pointCount = 0;
    IGsize cellCount = 0;
    const bool counted = GenerateGlobalIdsFilter::CountEntities(object, pointCount, cellCount);
    std::cout << "  " << label << " counts: points=" << pointCount << ", cells=" << cellCount
              << ", result=" << (counted ? "success" : "failure") << '\n';
    Check(counted,
          label + " was rejected by CountEntities.");
    Check(pointCount == expectedPointCount && cellCount == expectedCellCount,
          label + " entity counts are incorrect.");

    auto filter = GenerateGlobalIdsFilter::New();
    filter->SetInput(object);
    filter->SetOffsets(300, 400);
    const bool generated = filter->Execute();
    std::cout << "    generation with offsets (300, 400): "
              << (generated ? "success" : "failure") << '\n';
    Check(generated, label + " generation failed: " + filter->GetMessage());
    auto pointIds = FindDoubleArray(object, "GlobalPointIds", IG_POINT);
    auto cellIds = FindDoubleArray(object, "GlobalCellIds", IG_CELL);
    PrintIdArray(label + " GlobalPointIds", pointIds);
    PrintIdArray(label + " GlobalCellIds", cellIds);
    CheckIdRange(pointIds, expectedPointCount, 300,
                 label + " point IDs");
    CheckIdRange(cellIds, expectedCellCount, 400,
                 label + " cell IDs");
}
/**
 * @brief 对不同网格类型进行测试
 */
void TestAllSupportedMeshTypes() {
    auto volume = VolumeMesh::New();
    volume->SetPoints(MakePoints(4));
    auto volumes = CellArray::New();
    volumes->AddCellId4(0, 1, 2, 3);
    volume->SetVolumes(volumes);
    CheckSupportedLeaf(volume, 4, 1, "VolumeMesh");

    auto unstructured = UnstructuredMesh::New();
    unstructured->SetPoints(MakePoints(4));
    igIndex tetraIds[4]{0, 1, 2, 3};
    unstructured->AddCell(tetraIds, 4, IG_TETRA);
    CheckSupportedLeaf(unstructured, 4, 1, "UnstructuredMesh");

    auto structured = StructuredMesh::New();
    structured->SetPoints(MakePoints(4));
    igIndex dimensions[3]{2, 2, 1};
    structured->SetDimensionSize(dimensions);
    auto structuredFaces = CellArray::New();
    structuredFaces->AddCellId4(0, 1, 2, 3);
    structured->SetFaces(structuredFaces);
    CheckSupportedLeaf(structured, 4, 1, "StructuredMesh");

    auto lagrange = LagrangeUnstructuredMesh::New();
    lagrange->SetPoints(MakePoints(2));
    igIndex lineIds[2]{0, 1};
    lagrange->AddCell(lineIds, 2, IG_LAGRANGE_CURVE, 1);
    CheckSupportedLeaf(lagrange, 2, 1, "LagrangeUnstructuredMesh");
}
/**
 * @brief 测试位于数值边界时会不会出现问题
 */
void TestDoublePrecisionBoundary() {
    constexpr iguIndex64 maximumExactDoubleInteger = iguIndex64{1} << 53;

    auto onePoint = PointSet::New();
    onePoint->GetPoints()->AddPoint(0.0f, 0.0f, 0.0f);

    auto accepted = GenerateGlobalIdsFilter::New();
    accepted->SetInput(onePoint);
    accepted->SetGenerateCellIds(false);
    accepted->SetPointOffset(maximumExactDoubleInteger);
    const bool boundaryAccepted = accepted->Execute();
    auto boundaryIds = FindDoubleArray(onePoint, "GlobalPointIds", IG_POINT);
    std::cout << "  One point at DoubleArray boundary 2^53=" << maximumExactDoubleInteger
              << ": " << (boundaryAccepted ? "success" : "failure") << '\n';
    PrintIdArray("boundary GlobalPointIds", boundaryIds);
    Check(boundaryAccepted,
          "The exact 2^53 boundary was rejected: " + accepted->GetMessage());
    CheckIdRange(boundaryIds, 1,
                 maximumExactDoubleInteger, "Boundary point IDs");

    auto twoPoints = PointSet::New();
    twoPoints->GetPoints()->AddPoint(0.0f, 0.0f, 0.0f);
    twoPoints->GetPoints()->AddPoint(1.0f, 0.0f, 0.0f);

    auto rejected = GenerateGlobalIdsFilter::New();
    rejected->SetInput(twoPoints);
    rejected->SetGenerateCellIds(false);
    rejected->SetPointOffset(maximumExactDoubleInteger);
    const bool inexactRangeAccepted = rejected->Execute();
    auto rejectedIds = FindDoubleArray(twoPoints, "GlobalPointIds", IG_POINT);
    std::cout << "  Two points starting at 2^53: "
              << (inexactRangeAccepted ? "success" : "failure (expected)")
              << ", message=\"" << rejected->GetMessage() << "\"\n";
    PrintIdArray("rejected inexact GlobalPointIds", rejectedIds);
    Check(!inexactRangeAccepted, "An inexact DoubleArray ID range was accepted.");
    Check(rejected->GetMessage().find("represented exactly") != std::string::npos,
          "The precision failure did not report its cause.");
    Check(FindAttribute(twoPoints, "GlobalPointIds", IG_POINT) == nullptr,
          "A rejected precision range modified its input.");
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [model.vtk]\n";
        return 2;
    }
    if (argc == 2) { TestModelFilePath = argv[1]; }

    const std::vector<std::pair<std::string, std::function<void()>>> tests{
            {"VTK file model and existing-ID policies", TestFileModelAndExistingPolicies},
            {"simulated process-prefix offsets", TestSimulatedProcessOffsets},
            {"composite shared points and parent times", TestCompositeSharedPointsAndParentTimes},
            {"atomic composite failure", TestFailureIsAtomicAcrossLeaves},
            {"all supported mesh types", TestAllSupportedMeshTypes},
            {"DoubleArray precision boundary", TestDoublePrecisionBoundary},
    };

    int failures = 0;
    for (const auto& [name, test]: tests) {
        try {
            std::cout << "\n[RUN] " << name << '\n';
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test group(s) failed.\n";
        return 1;
    }

    std::cout << "All GenerateGlobalIdsFilter tests passed.\n";
    return 0;
}
