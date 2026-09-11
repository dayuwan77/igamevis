# ValidateCellsFilter 使用说明

## 功能
`ValidateCellsFilter` 用于校验网格单元是否有效。

- 对每个单元或表面面片计算有效性状态码。
- 状态码为 0 表示有效，非 0 表示存在问题。
- 生成独立的 `*_ValidateCells` 输出节点，不修改原输入。
- 可以获取已检查、有效、无效、未支持单元数量，以及各错误类别对应的单元 ID。
- Qt 端会显示结果汇总面板，并支持按错误类别或单个单元高亮。

## 支持范围
- `UnstructuredMesh`：`IG_VERTEX`、`IG_LINE`、`IG_POLY_LINE`、`IG_FACE`、`IG_TRIANGLE`、`IG_QUAD`、`IG_POLYGON`、`IG_TETRA`。
- `SurfaceMesh`：按面校验三角形、四边形和多边形面。
- 不支持的单元类型不会被当作“几何无效”，而是单独计入 `GetUnsupportedCellCount()`，状态码为 `0x40`。

## 状态码说明
- `0x00`：有效。
- `0x01`：点数不对。
- `0x02`：边自相交。
- `0x04`：面相交。
- `0x08`：边不连续。
- `0x10`：非凸，包括重复点、零面积。
- `0x20`：四面体方向错误。
- `0x40`：不支持的单元类型。

## 调用方式

```cpp
#include <MyFilter/iGameValidateCellsFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>

auto obj = iGame::FileIO::ReadFile("model.vtk");
if (obj == nullptr) {
    return;
}

// 如果需要在渲染窗口中查看结果，请使用 filter 的独立输出节点。
iGame::Scene::Pointer scene = iGame::Scene::New();

auto filter = iGame::ValidateCellsFilter::New();
filter->SetInput(obj);

if (!filter->Execute()) {
    return;
}

auto output = filter->GetOutput();
scene->AddModel(output);
auto model = scene->GetCurrentModel();

int invalidCount = filter->GetInvalidCellCount();
const auto& invalidIds = filter->GetInvalidCellIds();

for (auto id : invalidIds) {
    // id 为无效单元编号
}
```

## 使用示例

参考示例程序：

```
Examples/Filter/MyFilter/TestValidateCellsFilter.cpp
```

示例程序读取模型，执行校验，并在控制台输出无效单元数量和无效单元 ID 列表。

常用测试模型：

- 原测试模型：`Examples/Models/iGameValidateCellsFilter_test.vtk`
- 混合单元测试模型：`Examples/Models/iGameValidateCellsFilter_mixed.vtk`

## 输出说明
- filter 不会修改输入数据对象，而是生成一个独立输出节点，默认名称为 `<输入名称>_ValidateCells`。
- 输出节点会添加单元属性 `ValidityState`，每个单元的值对应上述状态码。
- `GetCheckedCellCount()`、`GetValidCellCount()`、`GetInvalidCellCount()`、`GetUnsupportedCellCount()` 可获取汇总数量。
- `GetCellIdsWithFlag(flag)` 可获取命中某一位标志的单元，用于按错误类别高亮。
- `GetValidityStateText(state)` 会把位掩码转换为“边相交、非凸”这类可读文本。

## 注意事项
- 输入可以是 `UnstructuredMesh` 或 `SurfaceMesh`。
- 对 `UnstructuredMesh`，当前支持点、线、折线、三角形、四边形、多边形、四面体；六面体等类型会被标记为不支持。
- 对 `SurfaceMesh`，每个面都按多边形校验。
- Qt 端负责把输出节点加入模型树，并由结果面板执行高亮；手动调用时请自行把 `filter->GetOutput()` 加入场景。
- 每次执行会在输出节点上重新写入 `ValidityState` 属性。
- 重复点、零面积三角形、自相交多边形等都会使对应单元被标记为无效。
