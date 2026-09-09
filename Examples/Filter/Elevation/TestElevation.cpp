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
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace iGame;

// ===== 模型文件路径（相对构建目录；Examples/CMakeLists.txt 会把 Models/ 复制过去）=====
std::string SlopeModelPath = "Models/ElevationSlopeTerrain.vtk";
std::string TerracesModelPath = "Models/ElevationTerraces.vtk";

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

// 读取模型文件并转型为 PointSet（ElevationFilter 的输入类型）
PointSet::Pointer LoadPointSetModel(const std::string& path) {
    auto object = FileIO::ReadFile(path);
    Check(object != nullptr, "failed to read model file: " + path);
    auto pointSet = DynamicCast<PointSet>(object);
    Check(pointSet != nullptr, path + " is not a PointSet-compatible mesh.");
    return pointSet;
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

// ===== 模型文件用例（Examples/Models 下的配套测试模型）=====

// 用例 7：斜坡地形模型 + 任意方向 (1,1,0) + 默认范围 [0,1]
// 模型 ElevationSlopeTerrain.vtk：11x11 顶点，x,y ∈ {0..10}，z = 0.1*(x+y)
// 沿 (1,1,0) 投影 h = x+y ∈ [0,20]，期望 elevation = (x+y)/20（端点钉扎：(0,0)→0，(10,10)→1）
void TestSlopeModelArbitraryDirection() {
    auto mesh = LoadPointSetModel(SlopeModelPath);
    Check(mesh->GetNumberOfPoints() == 121, "slope model must have 121 points.");

    auto filter = ElevationFilter::New();
    Check(filter->SetDirection(1.f, 1.f, 0.f), "SetDirection should accept (1,1,0).");
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed on the slope model.");

    auto array = FindElevationArray(mesh);
    Check(array != nullptr, "slope model: Elevation array is missing.");
    Check(array->GetNumberOfElements() == 121, "slope model: unexpected element count.");

    const float* values = array->RawPointer();
    for (IGsize i = 0; i < 121; ++i) {
        const Point& p = mesh->GetPoint(i);
        const double expected = (p[0] + p[1]) / 20.0;
        if (std::fabs(values[i] - expected) > 1e-5) {
            throw std::runtime_error("slope model: point " + std::to_string(i) +
                                     " elevation is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected));
        }
    }
    // 端点钉扎：首点 (0,0) → 0；末点 (10,10) → 1
    Check(std::fabs(values[0]) < 1e-6, "slope model: first point must map to 0.");
    Check(std::fabs(values[120] - 1.0) < 1e-6, "slope model: last point must map to 1.");
}

// 用例 8：梯田地形模型 + 默认方向 +Z + 范围 [10,20]
// 模型 ElevationTerraces.vtk：11x11 顶点，z = floor((x+y)/4) ∈ {0..5} 六层台阶
// h = z 为精确整数，期望 elevation = 10 + 2z ∈ {10,12,...,20}，且恰好出现六个离散值
void TestTerracesModelAxisMapping() {
    auto mesh = LoadPointSetModel(TerracesModelPath);
    Check(mesh->GetNumberOfPoints() == 121, "terraces model must have 121 points.");

    auto filter = ElevationFilter::New();
    filter->SetOutputRange(10.0, 20.0);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute should succeed on the terraces model.");

    auto array = FindElevationArray(mesh);
    Check(array != nullptr, "terraces model: Elevation array is missing.");
    Check(array->GetNumberOfElements() == 121, "terraces model: unexpected element count.");

    std::set<int> distinctLevels;
    const float* values = array->RawPointer();
    for (IGsize i = 0; i < 121; ++i) {
        const Point& p = mesh->GetPoint(i);
        const double expected = 10.0 + 2.0 * p[2];
        if (std::fabs(values[i] - expected) > 1e-6) {
            throw std::runtime_error("terraces model: point " + std::to_string(i) +
                                     " elevation is " + std::to_string(values[i]) +
                                     ", expected " + std::to_string(expected));
        }
        distinctLevels.insert(static_cast<int>(std::lround(values[i])));
    }
    // 六层台阶映射后应恰好产生 {10,12,14,16,18,20}
    const std::set<int> expectedLevels{10, 12, 14, 16, 18, 20};
    Check(distinctLevels == expectedLevels,
          "terraces model: mapped values must form six discrete levels {10..20}.");
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
            {"slope model with arbitrary direction", TestSlopeModelArbitraryDirection},
            {"terraces model with Z axis and range [10,20]", TestTerracesModelAxisMapping},
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
