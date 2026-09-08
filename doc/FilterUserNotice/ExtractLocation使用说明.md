# Extract Location 使用说明

## 功能

Extract Location（提取指定位置数据）用于查找第一个包含指定三维坐标的体单元，并输出该单元及其关联的点数据和单元数据，语义与 ParaView 的 `Extract Cell At Location` 一致。输出还包含 64 位的 `vtkOriginalPointIds` 和 `vtkOriginalCellIds`，用于追溯输出点、单元在原始网格中的编号。

当前支持线性四面体、六面体、三棱柱和金字塔单元。位置不在任何单元内部时会得到空输出；不支持的输入会给出提示，不会导致程序崩溃。

## 调用方式

### iGameVis 图形界面

1. 打开受支持的体网格，并在模型树中选中该模型。
2. 在【算法处理】一级菜单中选择【提取指定位置数据 (Extract Location)】。
3. 输入位置 X、Y、Z，或显示查询点后选择坐标轴并在视图中拖动。
4. 可点击“中心定位 (Center on Bounds)”把查询点放回模型包围盒中心。
5. 点击“执行”。命中时输出单元会加入模型树，并在 Data Arrays 区域显示保留的数组、类型、元素数和范围。

### C++ 接口

```cpp
auto filter = iGame::ExtractLocationFilter::New();
filter->SetInput(mesh);
filter->SetLocation(2.0, 1.5, 1.5);
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << '\n';
}
auto output = filter->GetOutput();
```

## 使用示例

示例程序为 `testExtractLocation`。程序会自动读取以下相对路径，无需手动输入文件名：

- `./Models/ExtractLocationConnectedSteppedSolid.vtk`：连通的阶梯状六面体网格，用于验证复杂模型中多个位置的提取和数据数组范围。
- `./Models/ExtractLocationTetraPair.vtk`：共享面的上下两个四面体，带有点标量、点向量和单元标量，用于验证指定位置命中不同单元。

自动测试会同时验证：

- 不同位置能够命中预期单元；
- 输出保留原始 Data Arrays 的名称、类型和值；
- `vtkOriginalPointIds` 和 `vtkOriginalCellIds` 可正确追溯原编号；
- 位于网格外部的位置得到空输出；不支持的单元类型安全失败。

在构建目录的 `Examples` 目录中运行 `testExtractLocation`，程序会自动完成模型读取、多个位置提取和结果校验。

## 注意事项

- 查询坐标采用模型自身的坐标系，不是屏幕像素坐标。
- 查询位置落在多个单元的公共边界上时，只返回按输入顺序找到的第一个单元。
- 点数据只保留被提取单元引用的点对应的数据；单元数据只保留命中单元对应的数据。
- 64 位整数数组按原类型复制，避免大于 `2^53` 的整数经过 `double` 搬运而丢失精度。
- 测试程序中的路径相对于运行时的 `Examples` 目录，不应改成个人电脑的绝对路径。
