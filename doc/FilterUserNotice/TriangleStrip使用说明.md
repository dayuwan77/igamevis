# TriangleStrip 使用说明

## 功能

`iGame::TriangleStripFilter` 将共享边的相邻三角形组织为三角带，并提取输入表面的边界边。开启连续片段合并后，首尾点 ID 相同的边界段会被连接为 `IG_POLY_LINE` 折线。

算法采用逐面访问和贪心延伸方式：从一个未处理三角形的三个局部有向边分别试探，选择覆盖三角形数量最多的候选带，然后继续处理剩余三角形。

Filter 的两个主要结果含义不同：

- `GetStrips()` 返回原生三角带点 ID 序列。一个包含 `T` 个三角形的条带具有 `T + 2` 个点 ID。
- `GetOutput()` 返回普通 `SurfaceMesh`。为兼容现有数据模型和渲染管线，三角带会在该对象中重新展开为普通三角面。

因此，读取真正的三角带必须调用 `GetStrips()`。

## 输入与输出

核心 Filter 支持以下输入：

- `SurfaceMesh`。
- 只包含二维表面单元的 `UnstructuredMesh`。

体网格或混合维度 `UnstructuredMesh` 应先提取表面，再进行三角化。非三角形表面不会参与三角带生成，而是通过 `GetPassThroughPolys()` 原样保留。

可读取的结果包括：

| 接口 | 返回内容 |
| --- | --- |
| `GetOutput()` | 展开为三角面和透传多边形的 `SurfaceMesh` |
| `GetStrips()` | 原生三角带，每个 CellArray 单元是一条带 |
| `GetStripSourceFaceIds()` | 每条带中的三角形到输入 face ID 的映射 |
| `GetPassThroughPolys()` | 未参与三角带生成的非三角形面 |
| `GetPolyLines()` | 输入表面的边界段，或合并后的连续边界折线 |
| `GetNumberOfStrips()` | 生成的三角带数量 |
| `GetLongestStripLength()` | 最长三角带包含的三角形数量 |

输出表面会创建新的 `AttributeSet`。PointData 保留原数组和点对应关系；CellData 按输出三角面的源 face ID 重新排列。

## 参数说明

### 最长三角带长度

```cpp
filter->SetMaximumLength(1000);
```

`MaximumLength` 表示单条三角带最多包含的三角形数量，不是点 ID 数量。默认值为 `1000`，小于 `1` 的输入会被限制为 `1`。

例如，一条包含 8 个三角形的完整带：

- `MaximumLength = 1000` 时可以输出 1 条长度 8 的带。
- `MaximumLength = 4` 时会被拆分为 2 条长度 4 的带。

### 合并连续片段

```cpp
filter->SetJoinContiguousSegments(true);
```

- `false`：每条边界边作为一个含两个点 ID 的线段输出。
- `true`：比较各线段的首尾点 ID，必要时反转方向后拼接为连续折线。

闭合边界的合并折线会重复起点。例如，由 10 条连续边界段组成的闭合边界会生成一条包含 11 个点 ID 的折线，且第一个和最后一个点 ID 相同。

该选项只处理边界折线，不会把不同三角带连接在一起。

## 调用方式

已经得到三角化 `SurfaceMesh` 时，可以直接调用：

```cpp
#include <TriangleStrip/iGameTriangleStripFilter.h>

#include <iostream>

void BuildStrips(const iGame::SurfaceMesh::Pointer& triangles) {
    using namespace iGame;

    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(true);

    if (!filter->Execute()) {
        std::cerr << "三角带转换失败。\n";
        return;
    }

    auto* strips = filter->GetStrips();
    for (IGsize stripId = 0; stripId < strips->GetNumberOfCells(); ++stripId) {
        const igIndex* pointIds = nullptr;
        const int pointCount = strips->GetCellIds(stripId, pointIds);
        const int triangleCount = pointCount - 2;

        std::cout << "strip " << stripId
                  << ": points=" << pointCount
                  << ", triangles=" << triangleCount << '\n';
    }

    auto* boundaryLines = filter->GetPolyLines();
    std::cout << "boundary polylines="
              << boundaryLines->GetNumberOfCells() << '\n';
}
```

## 从文件读取并预处理

测试模型是二维 `UnstructuredMesh`。下面展示完整的读取、表面转换、三角化和三角带生成流程：

```cpp
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <DataProcessing/iGameMeshTriangulationFilter.h>
#include <TriangleStrip/iGameTriangleStripFilter.h>
#include <iGameFileIO.h>

int main() {
    using namespace iGame;

    auto input = FileIO::ReadFile("Models/TriangleStripTestModel.vtk");
    if (!input) {
        return 1;
    }

    auto surfaceFilter = ConvertToSurfaceMeshFilter::New();
    surfaceFilter->SetInput(input);
    surfaceFilter->SetConvertMethod(
            ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
    if (!surfaceFilter->Execute()) {
        return 1;
    }

    auto triangulation = MeshTriangulationFilter::New();
    triangulation->SetInput(surfaceFilter->GetOutput());
    if (!triangulation->Execute()) {
        return 1;
    }

    auto triangles = DynamicCast<SurfaceMesh>(triangulation->GetOutput());
    auto filter = TriangleStripFilter::New();
    filter->SetInput(triangles);
    filter->SetMaximumLength(1000);
    filter->SetJoinContiguousSegments(true);
    if (!filter->Execute()) {
        return 1;
    }

    return 0;
}
```

完整测试示例位于：

```text
Examples/Filter/TriangleStrip/TestTriangleStrip.cpp
Examples/Filter/TriangleStrip/TestTriangleStripWidget.cpp
```

两个程序都固定读取 `Models/TriangleStripTestModel.vtk`。该模型包含 10 个点和 8 个连续三角形，默认生成 1 条长度 8 的三角带；表面具有 10 条连续边界段，开启合并后得到 1 条闭合折线。

## 注意事项

1. 输入表面应先完成三角化。非三角形面只会进入 `GetPassThroughPolys()`，不会形成三角带。
2. `GetOutput()` 中保存的是展开后的普通三角面；原生三角带必须通过 `GetStrips()` 读取。
3. `GetStrips()`、`GetPolyLines()` 和 `GetPassThroughPolys()` 返回的指针由 Filter 管理。需要长期保存时，应保证 Filter 生命周期有效或复制对应数据。
4. 三角带沿唯一可用的相邻三角面继续延伸。遇到边界、已处理面或无法唯一选择的非流形邻面时，当前条带会终止。
5. 当前算法是依赖输入面顺序的贪心算法，不保证得到全局最少的三角带数量。
6. `SetJoinContiguousSegments(true)` 只按点 ID 连通关系连接边界段，不进行几何距离容差合并；坐标相同但点 ID 不同的端点不会连接。
7. 闭合表面没有边界边，因此 `GetPolyLines()` 的单元数量为 `0`。
8. PointData 会保持点对应关系；CellData 会根据三角带展开后的源面映射重排。调用者不应假设输出面顺序与输入面顺序完全一致。
9. UI 会先尝试提取表面并三角化；直接使用核心 Filter 时，调用者需要自行保证输入是表面网格。
