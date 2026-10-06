/**
 * ResampleWithDataSet 自检示例
 *
 * 用 Examples/Models/RenameResample_test.vtk 里的六面体网格（8 个角点，点标量 value = 0..7）
 * 作为「被采样网格」，
 * 用自建点云 / 网格作为「采样点网格」，逐条断言：
 *   1) 输出类型与「采样点网格」一致，几何与拓扑是它的深拷贝；
 *   2) 源点数据被插值：六面体体心的三线性 / 均值坐标插值由对称性等于 8 个角点均值 3.5；
 *      自采样（探针 = 被采样网格本身）时有效点的插值结果应等于该点原值；
 *   3) validpointmask 与 GetSampleValidMask() 一致；网格外的点标记无效、插值填 0；
 *   4) 吸附半径关闭时近距网格外的点无效，开启后被吸附为有效，而远处的点仍无效；
 *   5) 「采样点网格」自身的数组被保留；与采样结果同名时以采样结果为准（不重复出现）；
 *   6) 缺少输入时 Execute() 返回 false 并给出消息。
 *
 * 「被采样网格」从 Examples/Models/RenameResample_test.vtk 读入（单位六面体，
 * 点数组 value = 0..7、alpha = 100..107，单元数组 cid = 42），不依赖渲染上下文，可直接运行。
 */
#include <ResampleWithDataset/iGameResampleWithDataSet.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameUnstructuredMesh.h"

#include <iGameFileIO.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Failed = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << std::endl;
    if (!ok) { ++g_Failed; }
}

void CheckNear(double actual, double expected, double tol, const std::string& what) {
    const bool ok = std::fabs(actual - expected) <= tol;
    std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << "  (actual=" << actual << ", expected=" << expected
              << ")" << std::endl;
    if (!ok) { ++g_Failed; }
}

/** 测试网格文件名（与 TestRenameArrays 共用，位于 Examples/Models/ 下） */
const char* kTestMeshName = "RenameResample_test.vtk";

/**
 * 定位测试网格：兼容从 <build>/Examples（ctest 的工作目录）、构建根目录、
 * 源码根目录等不同工作目录下运行。
 */
std::string ResolveTestMeshPath() {
    const std::vector<std::string> candidates = {
            std::string("Models/") + kTestMeshName,
            std::string("./Models/") + kTestMeshName,
            std::string("../Models/") + kTestMeshName,
            std::string("../../Models/") + kTestMeshName,
            std::string("Examples/Models/") + kTestMeshName,
            std::string("../../../Examples/Models/") + kTestMeshName,
    };
    for (const std::string& candidate : candidates) {
        if (std::filesystem::exists(candidate)) { return candidate; }
    }
    return std::string();
}

/**
 * 读入「被采样网格」：单位六面体，点数组 value(0..7)、alpha(100..107)，单元数组 cid=42。
 * 见 Examples/Models/RenameResample_test.vtk；体心的对称插值 = 8 个角点均值 3.5。
 */
iGame::UnstructuredMesh::Pointer MakeUnitHexahedron() {
    const std::string path = ResolveTestMeshPath();
    if (path.empty()) {
        Check(false, std::string("找到测试网格 ") + kTestMeshName +
                             "（请在 <build>/Examples 下运行，或确认 Examples/Models 已拷到构建目录）");
        return nullptr;
    }

    iGame::DataObject::Pointer object = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(object);
    if (mesh == nullptr) {
        Check(false, "读取测试网格失败：" + path);
        return nullptr;
    }
    return mesh;
}

/** 5 个采样点 + 一个探针自带的标量数组 "probe_marker" */
iGame::PointSet::Pointer MakeProbePoints() {
    auto probe = iGame::PointSet::New();
    probe->SetName("probe_points");

    probe->AddPoint(iGame::Point(0.5f, 0.5f, 0.5f));    // 0: 体心（对称 -> 3.5）
    probe->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f));    // 1: 角点
    probe->AddPoint(iGame::Point(-0.001f, -0.001f, -0.001f)); // 2: 刚出盒（近）
    probe->AddPoint(iGame::Point(10.0f, 10.0f, 10.0f)); // 3: 远处盒外
    probe->AddPoint(iGame::Point(0.5f, 0.5f, 0.0f));    // 4: 面心

    auto attrs = iGame::AttributeSet::New();
    auto marker = iGame::FloatArray::New();
    marker->SetName("probe_marker");
    marker->SetDimension(1);
    marker->Resize(5);
    for (int i = 0; i < 5; ++i) {
        const double v = 100.0 + static_cast<double>(i);
        marker->SetElement(static_cast<IGsize>(i), &v);
    }
    attrs->AddScalar(IG_POINT, marker);
    probe->SetAttributeSet(attrs);
    return probe;
}

/** 找输出里附着在点上的数组；hits 返回同名数组出现的次数（用于检查是否重复） */
iGame::ArrayObject::Pointer FindPointArray(iGame::AttributeSet* attrs, const std::string& name, int* hits) {
    if (hits != nullptr) { *hits = 0; }
    if (attrs == nullptr) { return nullptr; }

    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }

    iGame::ArrayObject::Pointer found;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }
        if (attr.pointer->GetName() != name) { continue; }
        if (hits != nullptr) { ++(*hits); }
        found = attr.pointer;
    }
    return found;
}

/* ------------------------------------------------------------------ */
/* 用例 1：自采样（探针 = 被采样网格本身，网格类型）                    */
/* ------------------------------------------------------------------ */
void TestSelfProbeOnMesh() {
    std::cout << "== 用例 1：网格自采样（检查几何/拓扑拷贝 + 插值还原 + 同名去重）" << std::endl;

    auto mesh = MakeUnitHexahedron();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    filter->SetProbeData(mesh);
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetResampledData();
    if (output == nullptr) {
        Check(false, "输出非空");
        return;
    }
    Check(output->GetDataObjectType() == IG_UNSTRUCTURED_MESH, "输出类型与采样点网格一致（UnstructuredMesh）");

    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(output);
    if (outMesh == nullptr) {
        Check(false, "输出可转换为 UnstructuredMesh");
        return;
    }
    Check(outMesh->GetNumberOfPoints() == 8, "输出点数 = 8");
    Check(outMesh->GetNumberOfCells() == 1, "输出单元数 = 1");
    Check(outMesh->GetPoint(7)[2] == 1.0f && outMesh->GetPoint(7)[0] == 1.0f, "输出几何是采样点网格的拷贝");

    int hits = 0;
    auto outValue = FindPointArray(output->GetAttributeSet(), "value", &hits);
    Check(outValue != nullptr, "输出含插值数组 value");
    Check(hits == 1, "value 只出现一次（与探针自身同名数组不重复）");

    // 探针点即原网格点：有效点的插值结果应等于该点原值
    const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
    int validCount = 0;
    int mismatch = 0;
    for (int i = 0; i < 8 && outValue != nullptr; ++i) {
        if (mask[static_cast<size_t>(i)] == 0) { continue; }
        ++validCount;
        const double interpolated = outValue->GetElementValue(static_cast<IGsize>(i), 0);
        if (std::fabs(interpolated - static_cast<double>(i)) > 1e-3) {
            ++mismatch;
            std::cout << "        点 " << i << " 插值=" << interpolated << " 原值=" << i << std::endl;
        }
    }
    Check(validCount >= 1, "至少有 1 个采样点有效（validCount=" + std::to_string(validCount) + "）");
    Check(mismatch == 0, "所有有效点的插值结果等于原值");
}

/* ------------------------------------------------------------------ */
/* 用例 2：点云探针（检查输出类型/坐标/mask/自带数组保留/数值）          */
/* ------------------------------------------------------------------ */
void TestPointCloudProbe() {
    std::cout << "\n== 用例 2：点云探针（输出为点云拷贝 + 采样属性）" << std::endl;

    auto mesh = MakeUnitHexahedron();
    auto probe = MakeProbePoints();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    filter->SetProbeData(probe);
    Check(filter->Execute(), "Execute() 成功");

    auto output = filter->GetResampledData();
    if (output == nullptr) {
        Check(false, "输出非空");
        return;
    }
    Check(output->GetDataObjectType() == IG_POINT_SET, "输出类型与采样点网格一致（PointSet）");

    auto outPoints = iGame::DynamicCast<iGame::PointSet>(output);
    if (outPoints == nullptr) {
        Check(false, "输出可转换为 PointSet");
        return;
    }
    Check(outPoints->GetNumberOfPoints() == 5, "输出点数 = 5");

    bool coordsEqual = true;
    for (int i = 0; i < 5; ++i) {
        const iGame::Point a = outPoints->GetPoint(static_cast<IGsize>(i));
        const iGame::Point b = probe->GetPoint(static_cast<IGsize>(i));
        for (int c = 0; c < 3; ++c) {
            if (std::fabs(a[c] - b[c]) > 1e-6f) { coordsEqual = false; }
        }
    }
    Check(coordsEqual, "输出坐标与采样点网格逐点一致");

    const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
    Check(mask.size() == 5, "GetSampleValidMask() 长度为 5");

    int maskHits = 0;
    auto outMask = FindPointArray(output->GetAttributeSet(), "validpointmask", &maskHits);
    Check(outMask != nullptr && maskHits == 1, "输出含 validpointmask（且只有一个）");
    bool maskMatches = (outMask != nullptr) && (outMask->GetNumberOfValues() == 5);
    if (maskMatches) {
        for (int i = 0; i < 5; ++i) {
            if (static_cast<unsigned char>(outMask->GetElementValue(static_cast<IGsize>(i), 0)) !=
                mask[static_cast<size_t>(i)]) {
                maskMatches = false;
            }
        }
    }
    Check(maskMatches, "validpointmask 属性与 GetSampleValidMask() 一致");

    // 体心：对称性 -> 8 个角点均值
    Check(mask[0] == 1, "体心（0）有效");
    auto outValue = FindPointArray(output->GetAttributeSet(), "value", nullptr);
    if (outValue != nullptr && mask[0] == 1) { CheckNear(outValue->GetElementValue(0, 0), 3.5, 1e-3, "体心插值 = 3.5"); }

    // 远处点：无效且插值填 0
    Check(mask[3] == 0, "远处盒外点（3）无效");
    if (outValue != nullptr) { CheckNear(outValue->GetElementValue(3, 0), 0.0, 1e-6, "无效点的插值填 0"); }

    // 探针自带的数组被保留
    int markerHits = 0;
    auto outMarker = FindPointArray(output->GetAttributeSet(), "probe_marker", &markerHits);
    Check(outMarker != nullptr && markerHits == 1, "探针自身的数组 probe_marker 被保留");
    if (outMarker != nullptr) { CheckNear(outMarker->GetElementValue(0, 0), 100.0, 1e-6, "probe_marker 数值保持"); }

    std::cout << "        有效点数 = " << static_cast<int>(filter->GetSampleValidMask().size()) << " 中 "
              << "有效=" << (int)mask[0] << (int)mask[1] << (int)mask[2] << (int)mask[3] << (int)mask[4] << std::endl;
}

/* ------------------------------------------------------------------ */
/* 用例 3：吸附半径                                                    */
/* ------------------------------------------------------------------ */
void TestSnappingRadius() {
    std::cout << "\n== 用例 3：吸附半径（默认关；开启后近距盒外点被吸附）" << std::endl;

    // 关闭吸附
    {
        auto mesh = MakeUnitHexahedron();
        auto probe = MakeProbePoints();
        auto filter = iGame::ResampleWithDataSet::New();
        filter->SetSourceData(mesh);
        filter->SetProbeData(probe);
        filter->SetSnappingRadius(0.0);
        Check(filter->Execute(), "radius=0: Execute() 成功");
        const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
        Check(mask.size() == 5 && mask[2] == 0, "radius=0: 近距盒外点（2）无效");
        Check(filter->GetSnappedPointCount() == 0, "radius=0: 吸附点数为 0");
    }

    // 开启吸附：对角线 ≈ 1.732，半径 0.01 * diag ≈ 0.0173 > 0.001*sqrt(3)
    {
        auto mesh = MakeUnitHexahedron();
        auto probe = MakeProbePoints();
        auto filter = iGame::ResampleWithDataSet::New();
        filter->SetSourceData(mesh);
        filter->SetProbeData(probe);
        filter->SetSnappingRadius(0.01);
        Check(filter->Execute(), "radius=0.01: Execute() 成功");
        const std::vector<unsigned char>& mask = filter->GetSampleValidMask();
        Check(mask.size() == 5 && mask[2] == 1, "radius=0.01: 近距盒外点（2）被吸附为有效");
        Check(filter->GetSnappedPointCount() >= 1, "radius=0.01: 吸附点数 >= 1");
        Check(mask[3] == 0, "radius=0.01: 远处盒外点（3）仍然无效");
    }
}

/* ------------------------------------------------------------------ */
/* 用例 4：错误路径                                                    */
/* ------------------------------------------------------------------ */
void TestErrorPaths() {
    std::cout << "\n== 用例 4：错误路径" << std::endl;

    auto mesh = MakeUnitHexahedron();

    auto filter = iGame::ResampleWithDataSet::New();
    filter->SetSourceData(mesh);
    Check(!filter->Execute(), "缺少「采样点网格」时 Execute() 返回 false");
    Check(!filter->GetMessage().empty(), "失败时给出消息：" + filter->GetMessage());

    auto filter2 = iGame::ResampleWithDataSet::New();
    filter2->SetProbeData(MakeProbePoints());
    Check(!filter2->Execute(), "缺少「被采样网格」时 Execute() 返回 false");
    Check(!filter2->GetMessage().empty(), "失败时给出消息：" + filter2->GetMessage());
}

} // namespace

int main() {
    std::cout << "==== ResampleWithDataSet 自检 ====" << std::endl;
    std::cout << "测试网格：" << ResolveTestMeshPath() << std::endl;

    // 先确认测试网格可读，避免后续用例在空指针上崩溃
    if (MakeUnitHexahedron() == nullptr) {
        std::cout << "\n==== 结果：测试网格不可用 ====" << std::endl;
        return 1;
    }

    TestSelfProbeOnMesh();
    TestPointCloudProbe();
    TestSnappingRadius();
    TestErrorPaths();

    std::cout << "\n==== 结果：" << (g_Failed == 0 ? "全部通过" : std::to_string(g_Failed) + " 项失败") << " ===="
              << std::endl;
    return g_Failed == 0 ? 0 : 1;
}
