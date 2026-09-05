#include <iostream>
#include <iGameCellArray.h>
#include <iGameFileIO.h>
#include <iGamePoints.h>
#include <iGamePointSet.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>

IGAME_NAMESPACE_BEGIN
// 模拟多进程分区场景：第 i 个点/单元属于进程 (i % 2)。
// 未来接入真实并行/分区机制时，子类以同样方式重写这两个方法即可，Execute 无需改动。
class MockPartitionedProcessIdsFilter : public GenerateProcessIdsFilter {
    I_OBJECT(MockPartitionedProcessIdsFilter)

public:
    static Pointer New() { return new MockPartitionedProcessIdsFilter; }
    MockPartitionedProcessIdsFilter() = default;

protected:
    long long GetPointProcessId(IGsize index) override { return index % 2; }
    long long GetCellProcessId(IGsize index) override { return index % 2; }
};
IGAME_NAMESPACE_END

namespace {

bool VerifyConstant(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                    IGsize expectCount, int expectValue) {
    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    filter->SetProcessId(expectValue);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute\n";
        return false;
    }
    auto& attr = mesh->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == expectValue);
    return ok;
}

bool VerifyPartitioned(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                       IGsize expectCount) {
    auto filter = iGame::MockPartitionedProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (partitioned)\n";
        return false;
    }
    auto& attr = mesh->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == static_cast<long long>(i % 2));
    return ok;
}

bool VerifyIdempotent(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                      IGsize expectCount, int expectValue) {
    for (int run = 0; run < 2; ++run) {
        auto filter = iGame::GenerateProcessIdsFilter::New();
        filter->SetInput(mesh);
        filter->SetGeneratePointData(pointData);
        filter->SetGenerateCellData(!pointData);
        filter->SetProcessId(expectValue);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (idempotent run " << run << ")\n";
            return false;
        }
    }
    auto attrs = pointData ? mesh->GetAttributeSet()->GetAllPointAttributes()
                           : mesh->GetAttributeSet()->GetAllCellAttributes();
    int count = 0;
    if (attrs != nullptr) {
        for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
            auto arr = attrs->GetElement(i).pointer;
            if (arr != nullptr && arr->GetName() == arrayName) ++count;
        }
    }
    if (count != 1) {
        std::cout << "FAIL: idempotent array count=" << count << " (expect 1)\n";
        return false;
    }
    auto& attr = mesh->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == expectValue);
    return ok;
}

bool VerifyExternalProcessId(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                             IGsize expectCount) {
    auto pidArray = iGame::LongLongArray::New();
    pidArray->SetName("process_id");
    pidArray->Resize(expectCount);
    for (IGsize i = 0; i < expectCount; ++i) pidArray->SetValue(i, static_cast<long long>(i % 3));
    if (pointData) {
        mesh->GetAttributeSet()->AddScalar(IG_POINT, pidArray);
    } else {
        mesh->GetAttributeSet()->AddScalar(IG_CELL, pidArray);
    }

    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (external process_id)\n";
        return false;
    }
    auto& attr = mesh->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == static_cast<long long>(i % 3));
    return ok;
}
}  // namespace

iGame::UnstructuredMesh::Pointer CreateMesh(int argc, char* argv[]) {
    if (argc > 1) {
        auto obj = iGame::FileIO::ReadFile(argv[1]);
        auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
        if (mesh == nullptr) {
            std::cout << "FAIL: read model " << argv[1] << "\n";
            return nullptr;
        }
        return mesh;
    }
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex cell[4] = {0, 1, 2, 3};
    mesh->AddCell(cell, 4, iGame::IG_TETRA);
    return mesh;
}

iGame::SurfaceMesh::Pointer CreateSurfaceMesh() {
    auto mesh = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    points->AddPoint(0.f, 0.f, 1.f);
    mesh->SetPoints(points);
    // 与 FileIO 一致：直接注入 CellArray，不走 AddFace（AddFace 依赖未初始化的 m_Edges 等成员，会崩溃）
    auto faces = iGame::CellArray::New();
    igIndex tri1[3]{0, 1, 2};
    igIndex tri2[3]{0, 2, 3};
    faces->AddCellIds(tri1, 3);
    faces->AddCellIds(tri2, 3);
    mesh->SetFaces(faces);
    return mesh;
}

iGame::VolumeMesh::Pointer CreateVolumeMesh() {
    auto mesh = iGame::VolumeMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    points->AddPoint(0.f, 0.f, 1.f);
    mesh->SetPoints(points);
    auto volumes = iGame::CellArray::New();
    igIndex volume[4] = {0, 1, 2, 3};
    volumes->AddCellIds(volume, 4);
    mesh->SetVolumes(volumes);
    return mesh;
}

bool VerifyUnsupportedCellData() {
    auto mesh = iGame::PointSet::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));

    auto cellOnly = iGame::GenerateProcessIdsFilter::New();
    cellOnly->SetInput(mesh);
    cellOnly->SetGeneratePointData(false);
    cellOnly->SetGenerateCellData(true);
    if (cellOnly->Execute()) {
        std::cout << "FAIL: cell data on PointSet should fail\n";
        return false;
    }
    if (cellOnly->GetMessage().empty()) {
        std::cout << "FAIL: GetMessage should be non-empty\n";
        return false;
    }

    auto pointOnly = iGame::GenerateProcessIdsFilter::New();
    pointOnly->SetInput(mesh);
    pointOnly->SetGeneratePointData(true);
    pointOnly->SetGenerateCellData(false);
    pointOnly->SetProcessId(3);
    if (!pointOnly->Execute()) {
        std::cout << "FAIL: point data on PointSet should succeed\n";
        return false;
    }
    auto& attr = mesh->GetAttributeSet()->GetScalar("PointProcessIds");
    auto arr = attr.pointer;
    return (arr != nullptr) && (arr->GetNumberOfElements() == 1) && (arr->GetValue(0) == 3);
}

int main(int argc, char* argv[]) {
    bool allOk = true;

    auto mesh = CreateMesh(argc, argv);
    if (mesh == nullptr) return 1;

    IGsize pointNum = mesh->GetNumberOfPoints();
    bool pointOk = VerifyConstant(mesh, true, "PointProcessIds", pointNum, 7);
    std::cout << (pointOk ? "PASS" : "FAIL") << ": point PointProcessIds count=" << pointNum << " value=7\n";
    allOk = allOk && pointOk;

    IGsize cellNum = mesh->GetNumberOfCells();
    bool cellOk = VerifyConstant(mesh, false, "CellProcessIds", cellNum, 7);
    std::cout << (cellOk ? "PASS" : "FAIL") << ": cell CellProcessIds count=" << cellNum << " value=7\n";
    allOk = allOk && cellOk;

    auto partMesh = CreateMesh(argc, argv);
    if (partMesh == nullptr) return 1;

    IGsize partPointNum = partMesh->GetNumberOfPoints();
    bool pointPartOk = VerifyPartitioned(partMesh, true, "PointProcessIds", partPointNum);
    std::cout << (pointPartOk ? "PASS" : "FAIL") << ": partitioned point PointProcessIds count=" << partPointNum
              << "\n";
    allOk = allOk && pointPartOk;

    IGsize partCellNum = partMesh->GetNumberOfCells();
    bool cellPartOk = VerifyPartitioned(partMesh, false, "CellProcessIds", partCellNum);
    std::cout << (cellPartOk ? "PASS" : "FAIL") << ": partitioned cell CellProcessIds count=" << partCellNum << "\n";
    allOk = allOk && cellPartOk;

    bool pointIdemOk = VerifyIdempotent(mesh, true, "PointProcessIds", pointNum, 7);
    std::cout << (pointIdemOk ? "PASS" : "FAIL") << ": idempotent point PointProcessIds count=" << pointNum << "\n";
    allOk = allOk && pointIdemOk;

    bool cellIdemOk = VerifyIdempotent(mesh, false, "CellProcessIds", cellNum, 7);
    std::cout << (cellIdemOk ? "PASS" : "FAIL") << ": idempotent cell CellProcessIds count=" << cellNum << "\n";
    allOk = allOk && cellIdemOk;

    auto extMesh = CreateMesh(argc, argv);
    if (extMesh == nullptr) return 1;

    IGsize extPointNum = extMesh->GetNumberOfPoints();
    bool extPointOk = VerifyExternalProcessId(extMesh, true, "PointProcessIds", extPointNum);
    std::cout << (extPointOk ? "PASS" : "FAIL") << ": external point PointProcessIds count=" << extPointNum << "\n";
    allOk = allOk && extPointOk;

    IGsize extCellNum = extMesh->GetNumberOfCells();
    bool extCellOk = VerifyExternalProcessId(extMesh, false, "CellProcessIds", extCellNum);
    std::cout << (extCellOk ? "PASS" : "FAIL") << ": external cell CellProcessIds count=" << extCellNum << "\n";
    allOk = allOk && extCellOk;

    auto surfMesh = CreateSurfaceMesh();
    IGsize surfFaceNum = surfMesh->GetNumberOfFaces();
    bool surfCellOk = VerifyConstant(surfMesh, false, "CellProcessIds", surfFaceNum, 7);
    std::cout << (surfCellOk ? "PASS" : "FAIL") << ": surface cell CellProcessIds count=" << surfFaceNum << "\n";
    allOk = allOk && surfCellOk;

    bool surfCellPartOk = VerifyPartitioned(surfMesh, false, "CellProcessIds", surfFaceNum);
    std::cout << (surfCellPartOk ? "PASS" : "FAIL") << ": surface partitioned cell CellProcessIds count="
              << surfFaceNum << "\n";
    allOk = allOk && surfCellPartOk;

    auto volMesh = CreateVolumeMesh();
    IGsize volNum = volMesh->GetNumberOfVolumes();
    bool volCellOk = VerifyConstant(volMesh, false, "CellProcessIds", volNum, 7);
    std::cout << (volCellOk ? "PASS" : "FAIL") << ": volume cell CellProcessIds count=" << volNum << "\n";
    allOk = allOk && volCellOk;

    bool volCellPartOk = VerifyPartitioned(volMesh, false, "CellProcessIds", volNum);
    std::cout << (volCellPartOk ? "PASS" : "FAIL") << ": volume partitioned cell CellProcessIds count=" << volNum
              << "\n";
    allOk = allOk && volCellPartOk;

    bool unsupportedOk = VerifyUnsupportedCellData();
    std::cout << (unsupportedOk ? "PASS" : "FAIL") << ": unsupported cell data on PointSet\n";
    allOk = allOk && unsupportedOk;

    return allOk ? 0 : 1;
}
