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

// 统计指定挂载类型上同名数组的个数：用于校验输入未被写入、结果中同名数组唯一
int CountArrays(iGame::DataObject::Pointer object, bool pointData, const std::string& arrayName) {
    if (object == nullptr || object->GetAttributeSet() == nullptr) return 0;
    auto attrs = pointData ? object->GetAttributeSet()->GetAllPointAttributes()
                           : object->GetAttributeSet()->GetAllCellAttributes();
    int count = 0;
    if (attrs != nullptr) {
        for (int i = 0; i < attrs->GetNumberOfElements(); ++i) {
            auto arr = attrs->GetElement(i).pointer;
            if (arr != nullptr && arr->GetName() == arrayName) ++count;
        }
    }
    return count;
}

// 独立输出节点的公共校验：结果非空、是新对象、类型不变、几何共享、输入未被修改、结果数组唯一
bool VerifyIndependentOutput(iGame::DataObject::Pointer input, iGame::DataObject::Pointer output, bool pointData,
                             const std::string& arrayName) {
    const char* label = pointData ? "point" : "cell";
    if (output == nullptr) {
        std::cout << "FAIL: " << label << " output is null\n";
        return false;
    }
    if (output == input) {
        std::cout << "FAIL: " << label << " output should be a new DataObject\n";
        return false;
    }
    if (output->GetDataObjectType() != input->GetDataObjectType()) {
        std::cout << "FAIL: " << label << " output type should stay unchanged\n";
        return false;
    }
    auto inputPointSet = iGame::DynamicCast<iGame::PointSet>(input);
    auto outputPointSet = iGame::DynamicCast<iGame::PointSet>(output);
    if (inputPointSet == nullptr || outputPointSet == nullptr ||
        inputPointSet->GetPoints() != outputPointSet->GetPoints()) {
        std::cout << "FAIL: " << label << " output should share input geometry\n";
        return false;
    }
    if (CountArrays(input, pointData, arrayName) != 0) {
        std::cout << "FAIL: " << label << " input should not be modified\n";
        return false;
    }
    if (CountArrays(output, pointData, arrayName) != 1) {
        std::cout << "FAIL: " << label << " result array should be unique\n";
        return false;
    }
    return true;
}

// 结果数组逐元素校验（含挂载类型）
template <typename ExpectValue>
bool VerifyResultValues(iGame::DataObject::Pointer output, bool pointData, const std::string& arrayName,
                        IGsize expectCount, ExpectValue expectValue) {
    auto& attr = output->GetAttributeSet()->GetScalar(arrayName);
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (arr->GetNumberOfElements() == expectCount) &&
              (attr.attachmentType == (pointData ? IG_POINT : IG_CELL));
    for (IGsize i = 0; ok && i < expectCount; ++i) ok = (arr->GetValue(i) == expectValue(i));
    return ok;
}

// 常数进程号：结果为独立节点，值全部等于常数
bool VerifyConstant(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName, IGsize expectCount,
                    int expectValue) {
    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(pointData);
    filter->SetGenerateCellData(!pointData);
    filter->SetProcessId(expectValue);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute\n";
        return false;
    }
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
    return VerifyResultValues(output, pointData, arrayName, expectCount,
                              [expectValue](IGsize) { return static_cast<long long>(expectValue); });
}

// 分区进程号：派生类按 index % 2 分配
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
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
    return VerifyResultValues(output, pointData, arrayName, expectCount,
                              [](IGsize i) { return static_cast<long long>(i % 2); });
}

// 重复执行：每次都得到新的独立结果，输入始终不被写入、结果中同名数组始终唯一
bool VerifyRepeatedExecution(iGame::DataObject::Pointer mesh, bool pointData, const std::string& arrayName,
                             IGsize expectCount, int expectValue) {
    for (int run = 0; run < 2; ++run) {
        auto filter = iGame::GenerateProcessIdsFilter::New();
        filter->SetInput(mesh);
        filter->SetGeneratePointData(pointData);
        filter->SetGenerateCellData(!pointData);
        filter->SetProcessId(expectValue);
        if (!filter->Execute()) {
            std::cout << "FAIL: Execute (repeated run " << run << ")\n";
            return false;
        }
        auto output = filter->GetOutput();
        if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
        if (!VerifyResultValues(output, pointData, arrayName, expectCount,
                                [expectValue](IGsize) { return static_cast<long long>(expectValue); })) {
            std::cout << "FAIL: repeated run " << run << " values\n";
            return false;
        }
    }
    return true;
}

// 外部 process_id 数组：结果沿用其值，该数组被拷入结果，输入保持不变
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
    auto output = filter->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, pointData, arrayName)) return false;
    if (!VerifyResultValues(output, pointData, arrayName, expectCount,
                            [](IGsize i) { return static_cast<long long>(i % 3); })) {
        return false;
    }

    // 输入上的 process_id 未被修改，且被拷贝到结果属性集中
    auto& inputPid = mesh->GetAttributeSet()->GetScalar("process_id");
    auto& outputPid = output->GetAttributeSet()->GetScalar("process_id");
    bool ok = (inputPid.pointer != nullptr) && (outputPid.pointer != nullptr) &&
              (outputPid.pointer->GetNumberOfElements() == expectCount);
    for (IGsize i = 0; ok && i < expectCount; ++i) {
        ok = (inputPid.pointer->GetValue(i) == static_cast<long long>(i % 3)) &&
             (outputPid.pointer->GetValue(i) == static_cast<long long>(i % 3));
    }
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
    if (cellOnly->GetOutput() != nullptr) {
        std::cout << "FAIL: failed execution should not keep an output\n";
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
    // 点云只生成点进程号：结果仍是独立对象（PointSet -> PointSet）
    auto output = pointOnly->GetOutput();
    if (!VerifyIndependentOutput(mesh, output, true, "PointProcessIds")) return false;
    return VerifyResultValues(output, true, "PointProcessIds", 1, [](IGsize) { return 3LL; });
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

    bool pointRepeatOk = VerifyRepeatedExecution(mesh, true, "PointProcessIds", pointNum, 7);
    std::cout << (pointRepeatOk ? "PASS" : "FAIL") << ": repeated point PointProcessIds count=" << pointNum << "\n";
    allOk = allOk && pointRepeatOk;

    bool cellRepeatOk = VerifyRepeatedExecution(mesh, false, "CellProcessIds", cellNum, 7);
    std::cout << (cellRepeatOk ? "PASS" : "FAIL") << ": repeated cell CellProcessIds count=" << cellNum << "\n";
    allOk = allOk && cellRepeatOk;

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
