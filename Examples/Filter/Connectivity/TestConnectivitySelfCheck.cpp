// 连通区域过滤器回归自检（无 GUI、无外部模型依赖）。
// 覆盖：6 种抽取模式、RegionId 编号染色、按单元数重编号、
//       标量连通、共享点判连通、失败路径。
#include <Connectivity/iGameConnectivityFilter.h>

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <cstdio>
#include <vector>

using namespace iGame;

namespace {

int g_checks = 0;
int g_failures = 0;

void Expect(bool ok, const char* what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  -> %s : %s\n", ok ? "PASS" : "FAIL", what);
}

// 三个互不连通的分量：区域 0 = 2 个三角形，区域 1 = 1 个三角形，区域 2 = 1 个三角形。
SurfaceMesh::Pointer MakeThreeIslands() {
    auto mesh = SurfaceMesh::New();
    auto pts = Points::New();
    // 区域 0：四边形 (0,1,2,3) 拆成 2 个三角形
    pts->AddPoint(Point(0, 0, 0));
    pts->AddPoint(Point(1, 0, 0));
    pts->AddPoint(Point(1, 1, 0));
    pts->AddPoint(Point(0, 1, 0));
    // 区域 1：三角形 (4,5,6)，远离区域 0
    pts->AddPoint(Point(5, 0, 0));
    pts->AddPoint(Point(6, 0, 0));
    pts->AddPoint(Point(5, 1, 0));
    // 区域 2：三角形 (7,8,9)，远离前两者
    pts->AddPoint(Point(10, 0, 0));
    pts->AddPoint(Point(11, 0, 0));
    pts->AddPoint(Point(10, 1, 0));
    mesh->SetPoints(pts);

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2); // f0 -> 区域 0
    faces->AddCellId3(0, 2, 3); // f1 -> 区域 0
    faces->AddCellId3(4, 5, 6); // f2 -> 区域 1
    faces->AddCellId3(7, 8, 9); // f3 -> 区域 2
    mesh->SetFaces(faces);
    return mesh;
}

// 链式三角带：f0=(0,1,2) 与 f1=(2,3,4) 仅共享点 2，用于标量连通测试。
SurfaceMesh::Pointer MakeChainWithScalar(const std::vector<double>& scalar) {
    auto mesh = SurfaceMesh::New();
    auto pts = Points::New();
    pts->AddPoint(Point(0, 0, 0));
    pts->AddPoint(Point(1, 0, 0));
    pts->AddPoint(Point(1, 1, 0));
    pts->AddPoint(Point(2, 1, 0));
    pts->AddPoint(Point(2, 2, 0));
    mesh->SetPoints(pts);
    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(2, 3, 4);
    mesh->SetFaces(faces);

    auto arr = FloatArray::New();
    arr->SetName("v");
    arr->SetDimension(1);
    arr->Resize(scalar.size());
    for (size_t i = 0; i < scalar.size(); ++i) {
        arr->RawPointer(static_cast<IGsize>(i))[0] = static_cast<float>(scalar[i]);
    }
    auto attrs = AttributeSet::New();
    attrs->AddAttribute(IG_SCALAR, IG_POINT, arr);
    mesh->SetAttributeSet(attrs);
    return mesh;
}

ArrayObject::Pointer FindRegionId(const SurfaceMesh::Pointer& mesh, IGenum attachment) {
    if (!mesh || !mesh->GetAttributeSet()) return nullptr;
    auto all = mesh->GetAttributeSet()->GetAllAttributes();
    if (!all) return nullptr;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& a = all->GetElement(i);
        if (a.isDeleted || a.pointer.IsNull()) continue;
        if (a.attachmentType == attachment && a.pointer->GetName() == ConnectivityFilter::RegionIdName) {
            return a.pointer;
        }
    }
    return nullptr;
}

void TestAllRegions() {
    std::printf("[case] ALL_REGIONS + 编号染色\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    filter->SetColorRegions(true);
    Expect(filter->Execute(), "Execute returns true");

    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Expect(out != nullptr, "output is a SurfaceMesh");
    if (!out) return;
    Expect(out->GetNumberOfFaces() == 4, "all 4 faces kept");
    Expect(filter->GetNumberOfExtractedRegions() == 3, "3 regions extracted");
    const auto& sizes = filter->GetRegionSizes();
    Expect(sizes.size() == 3 && sizes[0] == 2 && sizes[1] == 1 && sizes[2] == 1,
           "region sizes = {2,1,1}");

    auto cr = DynamicCast<IntArray>(FindRegionId(out, IG_CELL));
    Expect(cr != nullptr, "cell RegionId present");
    if (cr) {
        Expect(cr->GetValue(0) == 0 && cr->GetValue(1) == 0, "cells 0/1 -> region 0");
        Expect(cr->GetValue(2) == 1, "cell 2 -> region 1");
        Expect(cr->GetValue(3) == 2, "cell 3 -> region 2");
    }
    auto pr = DynamicCast<IntArray>(FindRegionId(out, IG_POINT));
    Expect(pr != nullptr, "point RegionId present");
    if (pr) { Expect(pr->GetNumberOfElements() == 10, "point RegionId size = 10"); }
}

void TestLargest() {
    std::printf("[case] LARGEST_REGION\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::LARGEST_REGION);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (out) {
        Expect(out->GetNumberOfFaces() == 2, "largest region has 2 faces");
        Expect(out->GetNumberOfPoints() == 4, "largest region has 4 points");
    }
}

void TestSpecified() {
    std::printf("[case] SPECIFIED_REGIONS\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::SPECIFIED_REGIONS);
    filter->InitializeSpecifiedRegionList();
    filter->AddSpecifiedRegion(1);
    filter->AddSpecifiedRegion(2);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (out) { Expect(out->GetNumberOfFaces() == 2, "regions 1+2 -> 2 faces"); }
}

void TestSeeded() {
    std::printf("[case] CELL/POINT/CLOSEST 种子模式\n");
    auto src = MakeThreeIslands();

    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS);
        filter->InitializeSeedList();
        filter->AddSeed(0);
        Expect(filter->Execute(), "cell-seeded Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 2, "cell 0 seed -> region of 2 faces"); }
    }
    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::POINT_SEEDED_REGIONS);
        filter->InitializeSeedList();
        filter->AddSeed(7); // 仅属于 f3（区域 2）
        Expect(filter->Execute(), "point-seeded Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 1, "point 7 seed -> 1 face"); }
    }
    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CLOSEST_POINT_REGION);
        filter->SetClosestPoint(5.2, 0.1, 0.0); // 最近点 4（区域 1）
        Expect(filter->Execute(), "closest-point Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 1, "closest to (5.2,0.1,0) -> 1 face"); }
    }
}

void TestReassignment() {
    std::printf("[case] RegionIdAssignmentMode\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    filter->SetRegionIdAssignmentMode(ConnectivityFilter::CELL_COUNT_ASCENDING);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    auto cr = DynamicCast<IntArray>(FindRegionId(out, IG_CELL));
    Expect(cr != nullptr, "cell RegionId present");
    if (cr) {
        // 升序：两个 1-face 区域得 0/1，2-face 区域得 2。
        Expect(cr->GetValue(2) == 0, "1-face region reordered to id 0");
        Expect(cr->GetValue(3) == 1, "1-face region reordered to id 1");
        Expect(cr->GetValue(0) == 2, "2-face region reordered to id 2");
    }
    const auto& sizes = filter->GetRegionSizes();
    Expect(sizes.size() == 3 && sizes[0] == 1 && sizes[1] == 1 && sizes[2] == 2,
           "region sizes reordered to {1,1,2}");
}

void TestScalarConnectivity() {
    std::printf("[case] ScalarConnectivity\n");
    // 点 2 的值远在区间外，隔断 f0/f1；点 3/4 在区间内。
    auto src = MakeChainWithScalar({0.0, 0.0, 0.0, 0.0, 0.0});

    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
        Expect(filter->Execute(), "geometric-only Execute returns true");
        Expect(filter->GetNumberOfExtractedRegions() == 1, "no scalar split -> 1 region");
    }
    {
        auto src2 = MakeChainWithScalar({0.0, 0.0, 10.0, 10.0, 10.0});
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src2);
        filter->SetScalarConnectivity(true);
        filter->SetScalarArrayName("v");
        filter->SetScalarRange(0.0, 1.0);
        filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
        Expect(filter->Execute(), "scalar Execute returns true");
        Expect(filter->GetNumberOfExtractedRegions() == 2, "scalar range splits into 2 regions");
    }
}

void TestFailures() {
    std::printf("[case] 失败路径\n");
    {
        auto filter = ConnectivityFilter::New();
        Expect(!filter->Execute(), "null input fails");
    }
    {
        auto um = UnstructuredMesh::New();
        um->GetPoints()->AddPoint(Point(0, 0, 0));
        um->GetPoints()->AddPoint(Point(1, 0, 0));
        um->GetPoints()->AddPoint(Point(0, 1, 0));
        igIndex ids[3] = {0, 1, 2};
        um->AddCell(ids, 3, IG_TRIANGLE);
        auto filter = ConnectivityFilter::New();
        filter->SetInput(um);
        Expect(!filter->Execute(), "UnstructuredMesh input rejected");
    }
    {
        auto src = MakeThreeIslands();
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS);
        // 不给种子
        Expect(!filter->Execute(), "seeded mode without seeds fails");
    }
    {
        auto src = MakeThreeIslands();
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(999);
        Expect(!filter->Execute(), "invalid extraction mode fails");
    }
}

} // namespace

int main() {
    TestAllRegions();
    TestLargest();
    TestSpecified();
    TestSeeded();
    TestReassignment();
    TestScalarConnectivity();
    TestFailures();

    std::printf("total checks: %d, failures: %d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
