# Subdivide（三角面细分）过滤器使用说明

## 一、功能简介

`SubdivideFilter` 对三角面做**线性 1-to-4 细分**：在三角形的三条边上各取一个中点，把一个三角形切分成四个小三角形。

- **只加密网格、不改变几何形状**：所有新点都落在原三角形所在平面上，细分前后模型外观完全一致。
- **旧顶点保持不动**：原顶点的坐标和点属性原样保留，不做任何移动。
- **中点为线性插值**：
  - 中点坐标 = 边两端点坐标的平均；
  - 中点的点属性 = 边两端点属性的平均。
- **共享边复用同一个中点**：相邻三角形在公共边上只生成一个中点，不会产生裂缝或重复点。
- **输出全部为三角形**：遇到非三角面（多边形）时先用三角形扇拆成三角形再细分。
- 行为与 ParaView 的 **Subdivide（vtkLinearSubdivisionFilter）一致**，包括点/面数量以及点的排列顺序。

**参数：**

| 参数名 | 含义 |
|---|---|
| `NumberOfSubdivisions` | 细分次数（正整数）。每细分一次，三角形数量 ×4。 |

数量关系（n 为细分次数）：

- 面数量 = 原面数量 × 4ⁿ；
- 点数量 = 原顶点 + 各代新增的边中点（共享边只计一次）。

---

## 二、调用方式

### 1. C++ 接口

```cpp
#include <Subdivide/iGameSubdivide.h>

auto filter = iGame::SubdivideFilter::New();
filter->SetNumberOfSubdivisions(1);   // 细分次数
filter->SetInput(surfaceMesh);        // 输入：SurfaceMesh
if (!filter->Execute()) {
    // 执行失败（通常是输入类型不支持）
}
auto outMesh = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(0));
```

主要接口：

| 接口 | 说明 |
|---|---|
| `SetNumberOfSubdivisions(int)` / `GetNumberOfSubdivisions()` | 设置 / 获取细分次数，默认 1。 |
| `SetInput(dataObject)` | 设置输入数据对象，须为 `SurfaceMesh`。 |
| `Execute()` | 执行细分，成功返回 `true`，类型不支持返回 `false`。 |
| `GetOutput(0)` | 获取输出数据对象，需 `DynamicCast<SurfaceMesh>`。 |

### 2. Qt 图形界面

菜单：**过滤器（Filters） → 三角面细分 (Subdivide)**

1. 在模型树中选中一个表面网格模型；
2. 在面板中填写“细分次数（1~8）”；
3. 点击“应用（Apply）”，生成细分结果。

---

## 三、使用示例

- 测试程序：`Examples/Filter/Subdivide/TestSubdivideFilter.cpp`
- 测试模型：`Examples/Models/subdivide_test.vtk`（23 个点、25 个三角形，带点标量 `height`）

命令行运行（工作目录为 Examples 输出目录）：

```bash
# 使用默认相对路径模型
TestSubdivideFilter.exe

# 或显式指定模型
TestSubdivideFilter.exe Models/subdivide_test.vtk
```

程序会打印输入/输出的点面数量，并逐行输出 1 级细分后**所有点的坐标位置**，例如：

```
读取模型: ././Models/subdivide_test.vtk
输入: 点数 23, 面数 25

========== Subdivide（1 级细分） ==========
输出: 点数 ..., 面数 100

---------- 所有点位置 ----------
Point 0: (..., ..., ...)
Point 1: (..., ..., ...)
...
```

> 1 级细分后面数量为 25 × 4 = 100；点数量为原 23 个顶点加上去重后的边中点，以程序实际输出为准。

在 ParaView 中对照：打开同一模型 → Filters → Subdivide，设置 `NumberOfSubdivisions`，用 Information 面板或导出 VTK 可逐点比对。

---

## 四、注意事项

1. **仅支持 `SurfaceMesh`（多边形表面网格 / Poly Data）**；体网格、非结构化网格等不支持，此时 `Execute()` 返回 `false`（界面提示“数据类型不匹配”）。
2. **外观无变化是正常现象**：线性细分不改变形状，颜色也等价于 GPU 的线性插值。要查看加密效果，请将显示模式改为 **Surface With Edges / Wireframe（带边面 / 线框）**，或在 Information 面板查看点/面数量。
3. **点/面数量随细分次数指数增长**（面数 ×4ⁿ），次数过大会占用大量内存、降低性能，建议控制在 1~8 次。
4. **点属性会保留并随中点线性插值**；**面属性（CellData）不会保留**，因为原面被拆分、重排。
5. 非三角面会先做三角形扇三角化，子三角形保持与原面一致的环绕方向和法向，不会出现法向翻转。
6. 点的排列顺序与 ParaView 严格对齐（边处理顺序为 C-A、A-B、B-C，四个子三角形按固定排列生成），可直接逐点比对结果。
