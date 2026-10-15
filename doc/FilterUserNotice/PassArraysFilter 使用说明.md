# PassArrays（传递过滤数据数组）使用说明

## 1. 功能

`PassArrays` 用于从输入网格中**只保留用户勾选的属性数组**（点数据 Point Data / 单元数据 Cell Data），并生成一个新的数据对象，对应 VTK 的 `vtkPassArrays` / ParaView 的「Pass Arrays」过滤器。

**主要特性：**

- **几何与拓扑共享**：输出直接引用输入的点坐标、单元连接关系（结构化网格共享维度与已生成的连接关系），不做深拷贝
- **保留的数组也按引用共享**：被勾选的属性数组及其数据范围直接挂到输出的 `AttributeSet` 上，不做深拷贝
- **点数据与单元数据分别记录选择**：点、单元上的同名属性（例如都叫 `test`）可以只保留其中一项，不会因为名称相同而互相牵连
- 未勾选的属性会被丢弃；输出为**新节点**，命名后缀 `_filtered`
- 输出与输入共享同一份几何与数组，因此**不会额外占用一份数据内存**

## 2. 输入 / 输出

| 项 | 说明 |
|---|---|
| 输入 | 1 个数据对象，支持 `PointSet` / `SurfaceMesh` / `VolumeMesh` / `UnstructuredMesh` / `StructuredMesh` |
| 输出 | 与输入共享几何、拓扑与选中数组的新数据对象，名称建议加后缀 `_filtered` |
| 失败 | 输入为空、数据类型不受支持时 `Execute()` 返回 `false` |

## 3. 界面调用方式

1. 加载并选中一个网格模型。
2. 菜单：**算法处理** → **开发中filter** → **传递过滤数据数组 (Pass Arrays)**。
3. 弹出「选择要传递的属性」对话框，属性按分区列出，**默认全部勾选**：
   - **点数据 (Point Data)**：输入模型的全部点属性
   - **单元数据 (Cell Data)**：输入模型的全部单元属性
4. 每个分区都提供 **全选 / 全不选** 按钮，也可以逐个勾选复选框。
5. 点击 **OK** 执行，过滤器结果以新模型节点加入模型树，名称为 `<输入名称>_filtered`。

> 至少需要勾选一个属性，否则会提示「请至少勾选一个属性。」并拒绝执行。

## 4. 代码接口

### 头文件

```cpp
#include <PassArrays/iGamePassArraysFilter.h>
```

### 基本流程

```cpp
auto filter = iGame::PassArrays::New();
filter->SetInput(input);

// 点数据与单元数据分别设置，互不影响
filter->SetPointArrayNames({"u1", "test"});   // 只保留点数据里的 u1、test
filter->SetCellArrayNames({"test"});          // 只保留单元数据里的 test

if (!filter->Execute()) {
    std::cerr << "Filter 执行失败\n";
    return 1;
}
auto output = filter->GetOutput();
output->SetName(input->GetName() + "_filtered");
```

### 主要 API

| 方法 | 说明 |
|------|------|
| `SetPointArrayNames(const std::vector<std::string>& names)` | 设置需要保留的点属性名称列表 |
| `AddPointArrayName(const std::string& name)` | 追加一个需要保留的点属性名称 |
| `GetPointArrayNames()` | 获取当前点属性选择 |
| `SetCellArrayNames(const std::vector<std::string>& names)` | 设置需要保留的单元属性名称列表 |
| `AddCellArrayName(const std::string& name)` | 追加一个需要保留的单元属性名称 |
| `GetCellArrayNames()` | 获取当前单元属性选择 |
| `SetArrayNames(pointNames, cellNames)` | 一次性设置两侧选择（仍然分别保存） |
| `ClearArrayNames()` | 清空两侧选择（输出将不保留任何属性数组） |
| `Execute()` | 执行过滤，成功返回 `true` |
| `GetOutput()` | 获取输出数据对象 |

## 5. 过滤规则与示例

### 5.1 只有「名称命中 + 归属相同」才保留

匹配规则：**名称精确匹配（大小写敏感），并且只在同一侧（点数据 / 单元数据）内匹配**。

以仓库自带测试模型 `Examples/Models/test_mesh.vtk` 为例（1 个六面体，8 个点）：

| 数据对象 | 点数据 | 单元数据 |
|---|---|---|
| `test_mesh.vtk` | `Temperature`、`Pressure`、`Velocity` | `Density`、`Material` |

| 勾选情况 | 输出保留的属性 |
|---|---|
| 点数据勾选 `Pressure`、`Velocity`，单元数据全不选 | 点数据：`Pressure`、`Velocity`；单元数据：无 |
| 点数据全不选，单元数据勾选 `Density` | 点数据：无；单元数据：`Density` |
| 两侧各选一项 | 各侧只保留自己被勾选的那一项 |

同名属性场景（旧实现会出错的地方）：假设某模型在点数据和单元数据上都存在名为 `test` 的属性，只勾选「点数据 → `test`」时，单元数据里同名的 `test` **不会**被一起保留；反之亦然。因为点、单元两侧的勾选结果分别记录、分别匹配。

### 5.2 与旧实现的区别

| 项 | 旧实现 | 当前实现 |
|---|---|---|
| 选择记录 | 点、单元共用一个名称列表，按名称匹配两侧 | 点数据、单元数据**分别记录**、分别匹配 |
| 几何 / 拓扑 | 深拷贝点坐标与单元连接关系 | 与输入**共享** |
| 属性数组 | 逐个深拷贝（`DeepCopyArray`） | 与输入**共享**（连数据范围一起共享） |
| 内存占用 | 约等于输入的一份副本 | 不额外复制几何与数组 |

## 6. 注意事项

1. **必须至少勾选一个属性**：两侧都不勾选时过滤器拒绝执行。
2. **名称在同一侧内精确匹配**：大小写敏感；点数据与单元数据的选择互不影响。
3. **共享语义**：输出与输入共享几何、拓扑与属性数组，修改其中一侧的数据会同时影响另一侧（这与旧版本的深拷贝隔离行为相反，换来的是零拷贝与更低内存占用）。
4. **支持的数据类型**：`PointSet`、`SurfaceMesh`、`VolumeMesh`、`UnstructuredMesh`、`StructuredMesh`；其他类型会报错并返回 `false`。
5. **结构化网格**：只共享维度、点坐标与已生成的连接关系；若输入尚未生成结构化连接关系，输出也不会主动生成。

## 7. 相关文件

| 文件路径 | 说明 |
|---|---|
| `iGameCore/Filters/PassArrays/iGamePassArraysFilter.h` | 过滤器头文件 |
| `iGameCore/Filters/PassArrays/iGamePassArraysFilter.cpp` | 过滤器实现 |
| `Qt/src/IQCore/igQtMainWindow.cpp` | 「传递过滤数据数组 (Pass Arrays)」菜单与多选框对话框 |
