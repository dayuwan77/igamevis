# Outline 使用说明

## 功能

生成模型包围盒线框。
复用 `DataObject::GetBoundingBox()`，生成与 X、Y、Z 轴平行的包围盒（AABB），
输出独立的 `UnstructuredMesh`，包含 8 个角点和 12 个 `IG_LINE` 单元。
不修改输入几何，不复制输入的点、单元属性。

## 在 iGameVis 中使用

1. 打开模型文件，例如 `Quad_Bicycle.vtk`，在模型树中选中要处理的模型。
2. 点击“算法处理 → 特征提取 → 生成包围盒 (Outline)”。此功能无需设置参数，点击后直接执行。
3. 模型树中新增 `Outline_1` 等结果节点，视图中显示白色包围盒线框，原模型保留。

可通过模型树分别控制原模型与包围盒的显示。再次处理原模型时，请重新选中原模型节点。
未选择模型或模型没有有效包围盒时，会显示提示，不生成结果节点。

## 代码调用方式

```cpp
#include <Outline/iGameOutlineFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile("Models/Quad_Bicycle.vtk");
auto filter = iGame::OutlineFilter::New();
filter->SetInput(input);
if (filter->Execute()) {
    auto outline = filter->GetOutput();
}
```

`Execute()` 成功返回 `true`；失败返回 `false`，通过 `GetMessage()` 查看原因。
每次执行先清空输出，失败后不会返回上次成功的结果。

## 使用示例

示例源码：`Examples/Filter/Outline/TestOutline.cpp`，构建目标：`testOutline`。
模型使用仓库已有的 `Examples/Models/Quad_Bicycle.vtk`。

在构建目录的 `Examples` 下运行 `testOutline.exe`：先执行结果检查，
通过后打开窗口，同时显示自行车表面和白色包围盒线框。
运行 `testOutline.exe --check` 只执行结果检查，不打开窗口。
示例使用相对路径 `Models/Quad_Bicycle.vtk`，需保持上述工作目录。

检查内容：输出点数和单元数、角点位置、线单元类型、边的唯一性与方向、
各角点连接数、输入与输出几何独立性，以及空输入、平面和单点输入。
失败返回非零退出码。交互验收时检查包围盒与模型范围贴合，并旋转、缩放和关闭窗口。

## 注意事项

- 输入必须能通过现有 `GetBoundingBox()` 提供有效包围盒；空包围盒或非有限边界值会失败。
- 平面、直线、单点数据仍保留 8 个点编号和 12 条边，部分点或边会重合、部分边长度为零。
- 此功能生成轴对齐包围盒，不生成随模型旋转的最小有向包围盒。
