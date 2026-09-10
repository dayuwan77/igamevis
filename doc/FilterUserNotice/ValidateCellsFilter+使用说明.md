# ValidateCellsFilter 使用说明

## 功能
`ValidateCellsFilter` 用于校验网格单元是否有效。

- 对每个单元或表面面片计算有效性状态码。
- 状态码为 0 表示有效，非 0 表示存在问题。
- 可以获取所有无效单元的 ID 列表。
- 如果设置了关联的 Model，还可以高亮显示无效单元。

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

// 如果需要在渲染窗口中高亮无效单元，请先加入场景并创建 Model。
iGame::Scene::Pointer scene = iGame::Scene::New();
scene->AddModel(obj);
auto model = scene->GetCurrentModel();

auto filter = iGame::ValidateCellsFilter::New();
filter->SetInput(obj);
filter->SetModel(model);  // 可选，设置后才会有高亮效果

if (!filter->Execute()) {
    return;
}

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
- filter 会修改输入数据对象，为其添加单元属性 `ValidityState`。
- 每个单元的值对应上述状态码。
- 无效单元 ID 可以通过 `GetInvalidCellIds()` 获取。
- 如果设置了 Model，无效单元会被自动选中并高亮。

## 注意事项
- 输入可以是 `UnstructuredMesh` 或 `SurfaceMesh`。
- 对 `UnstructuredMesh`，当前支持线、三角形、四边形、多边形、四面体等类型；六面体等类型会被标记为不支持。
- 对 `SurfaceMesh`，每个面都按多边形校验。
- 如果需要高亮，必须在 `Execute()` 之前调用 `SetModel(model)`。
- 每次执行会清除并重新写入 `ValidityState` 属性。
- 重复点、零面积三角形、自相交多边形等都会使对应单元被标记为无效。