# AppendDatasetsFilter（数据集合并）使用说明

## 1. 功能说明

AppendDatasetsFilter 对标 ParaView 的 **Append Datasets** Filter（VTK 类：`vtkAppendFilter`）。

将多个输入数据集合并为**单个 UnstructuredMesh**，保留原始单元类型（三角形、四边形、四面体、六面体等原样追加，不做类型转换）。

### 核心特性

- **多输入合并**：通过 `AddInput` 动态追加任意数量的输入网格
- **可选顶点合并**：`MergePoints` 开启时合并重合顶点（哈希 + 距离校验），消除接缝冗余
- **属性交集规则**：仅保留**所有输入都具备**的属性（按 (名称, 类型, 挂载位置) 三元组判定）

### 与 ParaView 对齐的口径

| 项目 | ParaView Append Datasets | iGameVis |
|------|--------------------------|----------|
| 输出类型 | Unstructured Grid | UnstructuredMesh |
| Merge Points 默认值 | **关闭** | **关闭**（与 ParaView 一致） |
| Tolerance 默认值 | 0（精确匹配） | 0（与 ParaView 一致） |
| 属性保留规则 | 交集（所有输入都有才保留） | 交集（一致） |
| 输入要求 | 至少 2 个数据集 | GUI 仅合并选中的模型，单选生成数据相同的新模型 |

> 默认行为（不合并点）下输出保留重复顶点，不做缝合；只有用户显式勾选
> 「合并重复顶点」才执行合并，Tolerance=0 走精确匹配分支。

---

## 2. 调用方式

### 2.1 iGameVis GUI 调用

1. 在模型树中按住 **Ctrl 或 Shift** 点击选中要合并的网格模型；只选一个时生成数据相同的新模型（不合并原有重复点），未选中时提示先选择；
2. 点击菜单 **「算法处理」→「新增filter/第二批」→「Append Datasets」**；
3. 参数对话框中按需勾选：
   - **合并重复顶点 (Merge Points)**：默认不勾选（与 ParaView 默认一致）；
   - **合并容差 (Tolerance)**：默认 0，仅合并完全重合的点；
4. 点击「应用」，仅将打开参数面板时选中的网格合并为一个 `append_datasets_result`，其他模型不参与，
   并自动激活第一个标量属性做颜色映射。

> 输入以打开参数面板时的模型树选择为准；未选中的模型（包括旧合并结果）不会参与。

### 2.2 C++ 代码调用

```cpp
#include <AppendDatasets/iGameAppendDatasetsFilter.h>
using namespace iGame;

auto filter = AppendDatasetsFilter::New();

// 逐个追加输入（数量不限）
filter->AddInput(mesh1);
filter->AddInput(mesh2);
filter->AddInput(mesh3);

// 可选：合并重合顶点（默认 false，与 ParaView 一致）
// filter->SetMergePoints(true);
// filter->SetTolerance(1e-6f);   // 默认 0 = 精确匹配

if (filter->Execute()) {
    auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    // outMesh 保留所有输入的单元与交集属性
} else {
    std::cout << "Error: " << filter->GetMessage() << std::endl;
}
```

---

## 3. 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| MergePoints | bool | false | 是否合并重合顶点（ParaView 默认为关） |
| Tolerance | float | 0 | 合并距离容差；0 表示精确匹配 |

---

## 4. 使用示例

### 示例 1：简单合并（默认参数，保留重复点）

```cpp
auto filter = AppendDatasetsFilter::New();
filter->AddInput(meshA);   // 3 点三角形
filter->AddInput(meshB);   // 与 A 共享 2 个顶点
filter->Execute();
// 输出 6 点 2 单元：共享顶点被原样保留（不缝合），与 ParaView 默认行为一致
```

### 示例 2：合并重合顶点

```cpp
filter->SetMergePoints(true);
filter->SetTolerance(0.0f);    // 精确匹配
filter->Execute();
// 输出 4 点 2 单元：2 个共享顶点被合并
```

### 示例 3：属性交集规则

```cpp
// meshA 带 Temperature + Pressure，meshB 只带 Temperature
filter->Execute();
// 输出只保留 Temperature（Pressure 不是所有输入都具备，被丢弃）
```

---

## 5. 注意事项

### 5.1 输入类型

- 支持 SurfaceMesh / VolumeMesh / UnstructuredMesh，统一归一为 UnstructuredMesh；
- DrawObject / CompositeDataObject 会递归收集子数据；
- 点云（PointSet）等无单元数据不参与合并。

### 5.2 属性挂载

- 点属性按 pointMap 重映射，单元属性按 cellOffset 追加；
- 属性判定为 (名称, 数据类型, 挂载位置) 三元组完全一致才视为同一属性。

### 5.3 与 AppendReduce 的关系

本 Filter 由旧版 AppendReduceFilter 更名并重写而来，类名与 ParaView 滤镜
"Append Datasets" 一一对应；旧类已删除，请改用 `AppendDatasetsFilter`。

### 5.4 测试与对标

测试代码位于 `Examples/Filter/TestAppendDatasets.cpp`，覆盖：多输入合并、
MergePoints 开/关、属性交集规则、AddInput 动态扩展。
GUI 对标验证模型位于 `Examples/Models/`：

| 文件名 | 描述 |
|--------|------|
| AppendDatasets_A_curved_patch_left/right.vtk | 曲面片对（共享接缝顶点） |
| AppendDatasets_B_hex_block.vtk / B_tet_mesh.vtk | 六面体块 + 四面体网格（混合单元类型） |
| AppendDatasets_C_hill_left/right.vtk | 山丘曲面左右半（点/单元属性交集验证） |

运行方式：在 `build/Examples` 目录下执行 `testAppendDatasets.exe`，
全部断言 PASS 且末尾 FAIL 数为 0 即通过。
