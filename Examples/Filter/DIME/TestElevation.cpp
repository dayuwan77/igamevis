/**
 * Test: ElevationFilter (DIME Filter #19)
 *
 * 代码内构建已知斜面网格（4 点 2 三角形，Z 坐标 0/1/2/3），验证：
 *   1. 默认映射 [0, 1]（Z 轴）  : elevation = z / 3（端点钉扎）
 *   2. 自定义映射 [10, 20]      : elevation = 10 + z / 3 * 10
 *   3. 平面网格降级             : 全部输出 Low，无 NaN
 *   4. 任意方向 (1,1,1)         : elevation = (x+y+z) / 5
 *   5. 方向缩放不变性 (3,3,3)   : 与 (1,1,1) 结果逐点相同
 *   6. 负方向翻转 (0,0,-1)      : 高低互换
 *   7. SetAxis(X) 向后兼容      : elevation = x
 *   8. 零向量拒绝               : SetDirection 返回 false 且方向不变
 * 测试自包含（无外部模型文件、无渲染窗口）；成功返回 0，失败返回 1。
 */
#include <cmath>
#include <iostream>
#include <DIME/iGameElevationFilter.h>
#include <iGamePointSet.h>
#include <iGameUnstructuredMesh.h>

static int g_failures = 0;

static void CheckValue(const char* tag, float actual, float expected) {
    const bool ok = std::fabs(actual - expected) < 1e-5f;
    std::cout << "[" << (ok ? "PASS" : "FAIL") << "] " << tag
              << ": expected " << expected << ", got " << actual << "\n";
    if (!ok) { ++g_failures; }
}

// 从网格上查找名为 name 的点挂载 FloatArray
static iGame::FloatArray::Pointer FindArray(iGame::PointSet::Pointer mesh, const std::string& name) {
    auto attrs = mesh->GetAttributeSet();
    for (int i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.pointer != nullptr && attr.attachmentType == IG_POINT &&
            attr.pointer->GetName() == name) {
            return iGame::DynamicCast<iGame::FloatArray>(attr.pointer);
        }
    }
    return nullptr;
}

// 4 点斜面：两个三角形组成的条带，Z 从 0 升到 3
static iGame::UnstructuredMesh::Pointer NewSlopedMesh() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));   // p0: z = 0
    mesh->AddPoint(iGame::Point(1.f, 0.f, 1.f));   // p1: z = 1
    mesh->AddPoint(iGame::Point(0.f, 1.f, 2.f));   // p2: z = 2
    mesh->AddPoint(iGame::Point(1.f, 1.f, 3.f));   // p3: z = 3
    igIndex tri0[3] = {0, 1, 2};
    igIndex tri1[3] = {1, 3, 2};
    mesh->AddCell(tri0, 3, iGame::IG_TRIANGLE);
    mesh->AddCell(tri1, 3, iGame::IG_TRIANGLE);
    return mesh;
}

int main() {
    // ---- Case 1: Z 轴默认范围 [0, 1] --------------------------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        if (!filter->Execute() || filter->GetOutput() != mesh) {
            std::cout << "[FAIL] case 1: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        if (arr == nullptr) {
            std::cout << "[FAIL] case 1: attribute 'Elevation' not found\n";
            return 1;
        }
        CheckValue("case1 p0 (z=0 -> 0)",        arr->GetValue(0), 0.0f);
        CheckValue("case1 p1 (z=1 -> 1/3)",      arr->GetValue(1), 1.0f / 3.0f);
        CheckValue("case1 p2 (z=2 -> 2/3)",      arr->GetValue(2), 2.0f / 3.0f);
        CheckValue("case1 p3 (z=3 -> 1)",        arr->GetValue(3), 1.0f);
    }

    // ---- Case 2: 自定义范围 [10, 20] ---------------------------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetAxis(iGame::ElevationFilter::Axis::Z);
        filter->SetRange(10.f, 20.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 2: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case2 p0 (z=0 -> 10)",       arr->GetValue(0), 10.0f);
        CheckValue("case2 p2 (z=2 -> 50/3)",     arr->GetValue(2), 10.0f + 2.0f / 3.0f * 10.0f);
        CheckValue("case2 p3 (z=3 -> 20)",       arr->GetValue(3), 20.0f);
    }

    // ---- Case 3: 平面网格降级为常量 Low（无 NaN）---------------------------
    {
        auto mesh = iGame::UnstructuredMesh::New();
        mesh->AddPoint(iGame::Point(0.f, 0.f, 5.f));
        mesh->AddPoint(iGame::Point(1.f, 0.f, 5.f));
        mesh->AddPoint(iGame::Point(0.f, 1.f, 5.f));
        igIndex tri[3] = {0, 1, 2};
        mesh->AddCell(tri, 3, iGame::IG_TRIANGLE);

        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetRange(2.f, 6.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 3: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        for (int i = 0; i < 3; ++i) {
            CheckValue("case3 flat mesh -> Low",  arr->GetValue(i), 2.0f);
        }
    }

    // ---- Case 4: 任意方向 (1,1,1)，h = x+y+z，范围 [0,5] -------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetDirection(1.f, 1.f, 1.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 4: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case4 p0 (h=0 -> 0)",        arr->GetValue(0), 0.0f);
        CheckValue("case4 p1 (h=2 -> 2/5)",      arr->GetValue(1), 2.0f / 5.0f);
        CheckValue("case4 p2 (h=3 -> 3/5)",      arr->GetValue(2), 3.0f / 5.0f);
        CheckValue("case4 p3 (h=5 -> 1)",        arr->GetValue(3), 1.0f);
    }

    // ---- Case 5: 缩放不变性——(3,3,3) 与 (1,1,1) 结果相同 ------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetDirection(3.f, 3.f, 3.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 5: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case5 p0 scaled == 0",       arr->GetValue(0), 0.0f);
        CheckValue("case5 p1 scaled == 2/5",     arr->GetValue(1), 2.0f / 5.0f);
        CheckValue("case5 p2 scaled == 3/5",     arr->GetValue(2), 3.0f / 5.0f);
        CheckValue("case5 p3 scaled == 1",       arr->GetValue(3), 1.0f);
    }

    // ---- Case 6: 负方向翻转——(0,0,-1) 高低互换 ----------------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetDirection(0.f, 0.f, -1.f);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 6: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case6 p0 (z=0 -> 1)",        arr->GetValue(0), 1.0f);
        CheckValue("case6 p3 (z=3 -> 0)",        arr->GetValue(3), 0.0f);
    }

    // ---- Case 7: SetAxis(X) 向后兼容——elevation = x -----------------------
    {
        auto mesh = NewSlopedMesh();
        auto filter = iGame::ElevationFilter::New();
        filter->SetInput(mesh);
        filter->SetAxis(iGame::ElevationFilter::Axis::X);
        if (!filter->Execute()) {
            std::cout << "[FAIL] case 7: Execute() returned failure\n";
            return 1;
        }
        auto arr = FindArray(mesh, "Elevation");
        CheckValue("case7 p0 (x=0 -> 0)",        arr->GetValue(0), 0.0f);
        CheckValue("case7 p1 (x=1 -> 1)",        arr->GetValue(1), 1.0f);
        CheckValue("case7 p2 (x=0 -> 0)",        arr->GetValue(2), 0.0f);
        CheckValue("case7 p3 (x=1 -> 1)",        arr->GetValue(3), 1.0f);
    }

    // ---- Case 8: 零向量被拒绝，方向保持不变 ---------------------------------
    {
        auto filter = iGame::ElevationFilter::New();
        if (filter->SetDirection(0.f, 0.f, 0.f)) {
            std::cout << "[FAIL] case 8: zero vector was accepted\n";
            ++g_failures;
        }
        // 默认方向 (0,0,1) 应保持不变
        CheckValue("case8 dir.x", filter->GetDirection()[0], 0.0f);
        CheckValue("case8 dir.y", filter->GetDirection()[1], 0.0f);
        CheckValue("case8 dir.z", filter->GetDirection()[2], 1.0f);
    }

    if (g_failures == 0) {
        std::cout << "TestElevation: all checks passed\n";
        return 0;
    }
    std::cout << "TestElevation: " << g_failures << " check(s) failed\n";
    return 1;
}
