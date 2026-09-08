# Overlapping Cells Detector 使用说明

## 功能

Overlapping Cells Detector 用于检测输入体网格中具有真实公共体积的重叠单元。执行后会在输入网格的单元数据中添加标量数组 `NumberOfOverlapsPerCell`，其值表示每个单元与其他多少个单元发生重叠。仅接触于点、边或面的单元不计为体积重叠。

当前支持 `UnstructuredMesh` 和 `VolumeMesh` 中的线性四面体、六面体、三棱柱和金字塔单元；具有有效三维体单元连接关系的 `StructuredMesh` 也可处理。不支持的输入或单元类型会给出提示，不会导致程序崩溃。

## 调用方式

### iGameVis 图形界面

1. 打开一个受支持的体网格，并在模型树中选中该模型。
2. 在【算法处理】一级菜单中选择【检测重叠单元 (Overlapping Cells Detector)】。
3. 设置非负的“公差”；通常从 `0` 开始。
4. 点击“执行”。程序会生成 `NumberOfOverlapsPerCell`，并高亮存在重叠的单元。

### C++ 接口

```cpp
auto filter = iGame::OverlappingCellsDetectorFilter::New();
filter->SetInput(mesh);
filter->SetTolerance(0.0);
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << '\n';
}
const auto& counts = filter->GetNumberOfOverlapsPerCell();
```

## 使用示例

示例程序为 `testOverlappingCellsDetector`。程序会自动读取以下相对路径，无需手动输入文件名：

- `./Models/OverlappingCellsDetectorValidation.vtk`：4 个四面体，其中前两个相互重叠，期望结果为 `[1, 1, 0, 0]`。
- `./Models/OverlappingCellsDetectorFaceTouching.vtk`：2 个仅共享面的四面体，期望结果为 `[0, 0]`。

在构建目录的 `Examples` 目录中运行 `testOverlappingCellsDetector`，程序会自动执行模型读取、Filter 计算和结果校验，并以退出码表示测试是否通过。

## 注意事项

- 公差必须为非负数。公差增大后，只有重叠更明确的单元才会被认定为重叠。
- 当前只支持上述线性三维体单元，不支持二维面单元、高阶单元、多面体和复合数据。
- `NumberOfOverlapsPerCell` 是单元数据；查看结果时应选择该数组进行着色。
- 测试程序中的路径相对于运行时的 `Examples` 目录，不应改成个人电脑的绝对路径。
