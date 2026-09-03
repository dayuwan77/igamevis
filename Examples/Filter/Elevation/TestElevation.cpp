#include <Elevation/iGameElevationFilter.h>

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFileIO.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace iGame;

// 主测试网格：4 点 2 三角形，z = x + 2y（斜面）
//   p0(0,0,0)  p1(1,0,1)  p2(0,1,2)  p3(1,1,3)
// 沿 Z 投影 h = 0,1,2,3；沿 (1,1,0) 投影 h = x+y = 0,1,1,2
SurfaceMesh::Pointer MakeSlopeMesh() {
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 1.f);
    points->AddPoint(0.f, 1.f, 2.f);
    points->AddPoint(1.f, 1.f, 3.f);

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(1, 3, 2);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

// 平面网格：所有点 z = 5（沿 Z 投影退化）
SurfaceMesh::Pointer MakeFlatMesh() {
    auto points = Points::New();
    points->AddPoint(0.f, 0.f, 5.f);
    points->AddPoint(1.f, 0.f, 5.f);
    points->AddPoint(0.f, 1.f, 5.f);
    points->AddPoint(1.f, 1.f, 5.f);

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(1, 3, 2);

    auto mesh = SurfaceMesh::New();
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    return mesh;
}

void Check(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

FloatArray::Pointer FindElevationArray(DataObject::Pointer object) {
    if (!object || !object->GetAttributeSet()) { return nullptr; }
    auto* attributes = object->GetAttributeSet();
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attribute = attributes->GetAttribute(i);
        if (attribute.isDeleted || !attribute.pointer) { continue; }
        if (attribute.attachmentType != IG_POINT) { continue; }
        if (attribute.pointer->GetName() == "Elevation") {
            return DynamicCast<FloatArray>(attribute.pointer);
        }
    }
    return nullptr;
}

void CheckValues(const FloatArray::Pointer& array, const std::vector<double>& expected,
                 const std::string& label) {
    Check(array != nullptr, label + ": Elevation array is missing.");
    Check(array->GetDimension() == 1, label + ": dimension must be 1.");
    Check(array->GetNumberOfElements() == expected.size(),
          label + ": unexpected element count.");
    const float* values = array->RawPointer();
    for (IGsize i = 0; i < expected.size(); ++i) {
        if (std::fabs(values[i] - expected[i]) > 1e-6) {
            throw std::runtime_error(label + ": value " + std::to_string(i) +
                                     " is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected[i]));
        }
    }
}

// 用例 1：默认方向 +Z、默认范围 [0,1]（端点钉扎 + 中间值）
void TestAxisMapping() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckValues(FindElevationArray(mesh), {0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0},
                "axis mapping");
}

// 用例 2：自定义输出范围 [10, 20]
void TestCustomRange() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    filter->SetOutputRange(10.0, 20.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckValues(FindElevationArray(mesh), {10.0, 10.0 + 10.0 / 3.0, 10.0 + 20.0 / 3.0, 20.0},
                "custom range");
}

// 用例 3：任意方向 (1,1,0)——h = x+y = 0,1,1,2 → 0,0.5,0.5,1
void TestArbitraryDirection() {
    auto mesh = MakeSlopeMesh();
    auto filter = ElevationFilter::New();
    Check(filter->SetDirection(1.f, 1.f, 0.f), "SetDirection should accept (1,1,0).");
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed.");
    CheckValues(FindElevationArray(mesh), {0.0, 0.5, 0.5, 1.0}, "arbitrary direction");
}

// 用例 4：缩放不变性——(2,2,0) 与 (1,1,0) 输出完全一致
void TestScaleInvariance() {
    auto meshA = MakeSlopeMesh();
    auto filterA = ElevationFilter::New();
    filterA->SetDirection(1.f, 1.f, 0.f);
    filterA->SetInput(meshA);
    Check(filterA->Execute(), "Execute should succeed.");

    auto meshB = MakeSlopeMesh();
    auto filterB = ElevationFilter::New();
    filterB->SetDirection(2.f, 2.f, 0.f);
    filterB->SetInput(meshB);
    Check(filterB->Execute(), "Execute should succeed.");

    auto arrayA = FindElevationArray(meshA);
    auto arrayB = FindElevationArray(meshB);
    Check(arrayA && arrayB, "both arrays must exist.");
    const float* a = arrayA->RawPointer();
    const float* b = arrayB->RawPointer();
    const IGsize count = arrayA->GetNumberOfElements();
    for (IGsize i = 0; i < count; ++i) {
        Check(std::fabs(a[i] - b[i]) < 1e-6,
              "scale invariance: outputs of (1,1,0) and (2,2,0) must match.");
    }
}

// 用例 5：平面网格沿 Z 投影退化——全部输出 Low，无 NaN
void TestFlatMeshDegenerate() {
    auto mesh = MakeFlatMesh();
    auto filter = ElevationFilter::New();
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed on flat mesh.");
    CheckValues(FindElevationArray(mesh), {0.0, 0.0, 0.0, 0.0}, "flat mesh");

    // 自定义范围时降级值跟随 Low
    auto mesh2 = MakeFlatMesh();
    auto filter2 = ElevationFilter::New();
    filter2->SetOutputRange(10.0, 20.0);
    filter2->SetInput(mesh2);
    Check(filter2->Execute(), "Execute should succeed on flat mesh.");
    CheckValues(FindElevationArray(mesh2), {10.0, 10.0, 10.0, 10.0}, "flat mesh custom Low");
}

// 用例 6：非法输入防御——零向量、非法范围被拒绝且不影响执行结果
void TestInvalidInputs() {
    auto filter = ElevationFilter::New();
    Check(!filter->SetDirection(0.f, 0.f, 0.f), "zero direction must be rejected.");
    Check(filter->GetDirection()[2] == 1.f, "rejected input must keep old direction.");

    filter->SetOutputRange(1.0, 0.0);
    Check(filter->GetLowValue() == 0.0 && filter->GetHighValue() == 1.0,
          "invalid range must be rejected.");
    filter->SetOutputRange(5.0, 5.0);
    Check(filter->GetLowValue() == 0.0 && filter->GetHighValue() == 1.0,
          "empty range must be rejected.");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
            {"axis mapping (Z, [0,1])", TestAxisMapping},
            {"custom output range [10,20]", TestCustomRange},
            {"arbitrary direction (1,1,0)", TestArbitraryDirection},
            {"direction scale invariance", TestScaleInvariance},
            {"flat mesh degenerate", TestFlatMeshDegenerate},
            {"invalid input rejection", TestInvalidInputs},
    };

    int failures = 0;
    for (const auto& [name, test]: tests) {
        try {
            std::cout << "[RUN] " << name << '\n';
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
    std::cout << "All elevation filter tests passed.\n";
    return 0;
}
