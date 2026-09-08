# AppendReduceFilter（网格合并去重）使用说明

## 1. 功能说明

AppendReduceFilter 用于将多个表面网格（SurfaceMesh）合并为一个网格，并可选择合并重复顶点，同时自动合并各输入网格共有的属性数据。

### 核心功能

- **网格附加（Append）**：将多个输入网格的顶点和面片简单拼接，不做去重处理
- **顶点合并（Reduce / Merge）**：开启后，对空间位置相同（在容差范围内）的顶点进行去重，消除接缝处的冗余顶点
- **属性合并**：自动合并所有输入网格共有的点属性（点标量、点向量）和单元属性（单元标量），遵循**交集规则**——只有所有输入网格都拥有的属性才会被保留到输出中

### 与 ParaView 的对应关系

本 Filter 对标 ParaView 的 **Append Datasets** + **Merge Points** 功能的组合：

| 功能 | ParaView | iGameVis |
|------|----------|----------|
| 拼接网格 | Append Datasets | AppendReduce（合并关闭） |
| 顶点去重 | Merge Points | AppendReduce（合并开启） |
| 属性交集 | Append Datasets 自动处理 | MergeAttributes 自动处理 |

---

## 2. 调用方式

### 2.1 C++ 代码调用

```cpp
#include <AppendReduce/iGameAppendReduceFilter.h>
using namespace iGame;

// 1. 创建 Filter
auto filter = AppendReduceFilter::New();

// 2. 添加输入网格（可添加多个）
filter->AddInput(mesh1);
filter->AddInput(mesh2);
filter->AddInput(mesh3);

// 3. 设置参数
filter->SetMergePoints(true);   // 是否合并重复顶点，默认 true
filter->SetTolerance(1e-6f);    // 顶点合并容差，默认 1e-6

// 4. 执行
if (filter->Execute()) {
    auto result = filter->GetOutput();
    auto outMesh = DynamicCast<SurfaceMesh>(result);
    // 使用合并后的网格...
}
```

### 2.2 iGameVis GUI 调用

在 iGameVis 界面中：

1. 加载多个模型文件（支持 VTK、OBJ、STL 等格式）
2. 点击顶部菜单 **「算法处理」→「网格合并去重 (Append/Reduce)」**
3. 在弹出的参数对话框中设置：
   - **合并重复顶点**：勾选后对共顶点进行去重（默认勾选）
   - **合并容差**：顶点位置判断相等的距离阈值（默认 1e-6）
4. 点击「应用」执行合并，结果会自动添加到模型树

---

## 3. 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| MergePoints | bool | true | 是否合并重复顶点。关闭时仅做简单拼接，顶点数 = 各输入顶点数之和 |
| Tolerance | float | 1e-6 | 顶点合并的空间容差。两个顶点距离小于该值视为同一顶点。建议根据模型尺度调整 |

---

## 4. 使用示例

### 示例 1：两个相邻平面网格的合并

```cpp
// 读取两个相邻的平面网格
auto meshA = FileIO::ReadFile("AppendReduce_planeA.vtk");  // x=0..3
auto meshB = FileIO::ReadFile("AppendReduce_planeB.vtk");  // x=3..6（在 x=3 处共边）

auto filter = AppendReduceFilter::New();
filter->AddInput(meshA);
filter->AddInput(meshB);
filter->SetMergePoints(true);   // 开启顶点合并，消除 x=3 处的接缝顶点
filter->SetTolerance(1e-6f);
filter->Execute();

auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
// 合并前：16 + 16 = 32 个顶点
// 合并后：28 个顶点（4 个接缝顶点被去重）
```

### 示例 2：仅拼接不去重

```cpp
auto filter = AppendReduceFilter::New();
filter->AddInput(mesh1);
filter->AddInput(mesh2);
filter->SetMergePoints(false);  // 关闭合并，简单拼接
filter->Execute();
// 顶点数 = mesh1顶点数 + mesh2顶点数
```

### 示例 3：属性交集规则验证

```cpp
// meshA 有属性：Temperature（点标量）、Pressure（点标量）、Stress（单元标量）
// meshB 有属性：Temperature（点标量）、Velocity（点向量）、Stress（单元标量）

auto filter = AppendReduceFilter::New();
filter->AddInput(meshA);
filter->AddInput(meshB);
filter->SetMergePoints(true);
filter->Execute();

auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
auto attrSet = outMesh->GetAttributeSet();

// 输出网格中存在的属性（两个输入共有的）：
// - Temperature（点标量）✓
// - Stress（单元标量）✓
// 输出网格中不存在的属性（仅单个输入拥有的）：
// - Pressure（仅 meshA 有）✗
// - Velocity（仅 meshB 有）✗
```

---

## 5. 注意事项

### 5.1 属性合并规则（重要）

- 遵循 **交集规则**：只有**所有**输入网格都拥有的同名、同类型（标量/向量）、同附着类型（点/单元）的属性，才会被合并到输出中
- 单输入独有的属性会被丢弃
- 若属性同名但类型不同（一个是标量、一个是向量），不会被合并
- 若属性同名但附着类型不同（一个是点属性、一个是单元属性），不会被合并

### 5.2 顶点合并说明

- 合并使用空间哈希（Spatial Hash）加速查找，时间复杂度接近 O(n)
- 容差需根据模型尺度合理设置：过小可能无法正确去重，过大可能误合并不同顶点
- 合并开启时，接缝处的顶点属性以**第一个**包含该顶点的输入网格的属性值为准
- 仅支持**表面网格（SurfaceMesh）** 的顶点合并，不支持体网格

### 5.3 输入要求

- 输入必须为 `SurfaceMesh` 类型（或可递归提取出 SurfaceMesh 的数据对象）
- 支持任意数量的输入（≥1）
- 单个输入时 Filter 仍正常工作（相当于复制一份网格）

### 5.4 测试模型

`Examples/Models/` 目录下提供了以下测试模型：

| 文件名 | 描述 | 属性 |
|--------|------|------|
| AppendReduce_planeA.vtk | 左侧 4×4 平面网格（x=0..3） | Temperature（点标量）、Pressure（点标量）、Stress（单元标量） |
| AppendReduce_planeB.vtk | 右侧 4×4 平面网格（x=3..6） | Temperature（点标量）、Velocity（点向量）、Stress（单元标量） |

两个平面在 `x=3` 处共享边界，可用于验证顶点合并和属性交集功能。

示例代码位于 `Examples/Filter/TestAppendReduce.cpp`，运行后自动完成所有测试项。
