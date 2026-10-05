# Connected Surface Properties 使用说明

## 1. 功能说明

`ConnectedSurfacePropertiesFilter` 按共享边识别多边形表面中的连通对象，并计算逐面和逐对象的几何属性，对应 ParaView 的 **Connected Surface Properties**（`vtkMultiObjectMassProperties`）。

Filter 会生成独立输出，不修改输入模型。输出保留原有几何和属性，并增加以下数据：

| 名称 | 保存位置 | 类型 | 含义 |
| --- | --- | --- | --- |
| `ObjectIds` | 单元数据 | 64 位整数标量 | 每个面的连通对象编号 |
| `Areas` | 单元数据 | `double` 标量 | 每个面的面积 |
| `Volumes` | 单元数据 | `double` 标量 | 每个面对对象体积的有符号贡献 |
| `ObjectValidity` | 输出 Metadata | 整数标量 | 对象是否为闭合流形表面 |
| `ObjectAreas` | 输出 Metadata | `double` 标量 | 每个对象的总面积 |
| `ObjectVolumes` | 输出 Metadata | `double` 标量 | 每个对象的体积 |
| `ObjectCentroids` | 输出 Metadata | `double[3]` | 每个对象的质心 |

输出 Metadata 还保存 `NumberOfObjects`、`AllValid`、`TotalArea` 和 `TotalVolume`。iGame 当前没有与 ParaView Field Data 完全对应的属性位置，因此逐对象数组保存在 Metadata 中，并显示在 Qt 面板的对象明细表内。

## 2. iGameVis 调用方式

1. 打开一个多边形表面模型，并在模型树中选中该模型。
2. 点击一级菜单【算法处理】→【计算连通表面属性 (Connected Surface Properties)】。
3. 通常保持“跳过识别/有效性检查”未勾选。
4. 点击【执行并生成输出】。
5. 在“7 项数组”页查看输出数组，在“对象明细”页查看各对象的有效性、面积、体积和质心。
6. 在模型树中选择新生成的输出节点；原输入节点保持不变。

如果输入已包含与面数相同的 64 位整数单元数组，可以勾选“跳过识别/有效性检查”并填写数组名。此时 Filter 使用现有对象编号，不再检查连通性、闭合性和面方向。数组不存在、类型不符或长度不一致时，Filter 改用自动识别。

## 3. C++ 调用示例

```cpp
#include <DataProcessing/ConnectedSurfaceProperties/iGameConnectedSurfacePropertiesFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile(
        "./Models/ConnectedSurfacePropertiesPolyhedra.vtk");

auto filter = iGame::ConnectedSurfacePropertiesFilter::New();
filter->SetInput(input);
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << std::endl;
    return;
}

auto output = filter->GetOutput();
std::cout << "objects: " << filter->GetNumberOfObjects() << '\n';
std::cout << "all valid: " << filter->GetAllValid() << '\n';
std::cout << "total area: " << filter->GetTotalArea() << '\n';
std::cout << "total volume: " << filter->GetTotalVolume() << '\n';
```

使用已有对象编号：

```cpp
filter->SetSkipObjectIdentification(true);
filter->SetObjectIdsArrayName("RegionIds");
filter->Execute();
```

## 4. 示例模型与自动测试

| 模型 | 用途 | 预期结果 |
| --- | --- | --- |
| `ConnectedSurfacePropertiesOpenPatch.vtk` | 开放平面对象 | 1 个无效对象，面积 6，体积 0，质心为 `NaN` |
| `ConnectedSurfacePropertiesTwoBoxes.vtk` | 两个闭合盒体 | 2 个有效对象，总面积 16，总体积 3 |
| `ConnectedSurfacePropertiesPolyhedra.vtk` | 三棱柱、方锥和八面体 | 3 个有效对象，总面积 `9+√5+4√10+4√3`，总体积 `19/3` |

录屏使用 `ConnectedSurfacePropertiesPolyhedra.vtk`。其总面积约为 `30.813382`，总体积约为 `6.333333`；对比时应同时检查 `ObjectIds`、面积、体积、有效性和质心。

自动测试文件：

```text
Examples/Filter/DataProcessing/ConnectedSurfaceProperties/TestConnectedSurfaceProperties.cpp
Examples/Filter/DataProcessing/ConnectedSurfaceProperties/TestConnectedSurfacePropertiesWidget.cpp
```

测试程序从 `Examples/Models` 读取固定相对路径，无需输入文件名。在构建目录下运行：

```text
Examples/testConnectedSurfaceProperties.exe
testConnectedSurfacePropertiesWidget.exe
```

测试内容包括连通对象识别、开放表面判定、逐面与逐对象结果、现有对象编号、独立输出、渲染数据转换，以及面板关闭和应用退出时的内存检查。测试程序不使用 `Tet_Plane.vtk`。

## 5. 支持范围

输入必须是 `SurfaceMesh`，对应 ParaView 的多边形 `PolyData`。体网格、结构网格和点集需要先提取或转换为表面。

连通关系按共享完整边确定。只共享一个点的两个表面会被识别为不同对象。有效对象必须闭合、流形，并且能够建立一致的面方向。

## 6. 注意事项

1. 开放边、非流形边或无法统一面方向的对象会被标记为无效。
2. `TotalVolume` 只累计有效对象；无效对象仍输出面积和逐面的有符号体积贡献。
3. 零体积或开放平面对象的质心按 ParaView 行为输出为 `NaN`。
4. 输入面应是按边界顺序排列的简单平面多边形；自相交面、严重非平面面和退化面不保证得到有效结果。
5. 自动识别会在计算过程中统一相邻面的方向，但不会修改输出几何中的点顺序。
6. 跳过识别模式默认信任给定对象编号，并假定各对象闭合、流形且面方向一致。
7. Legacy VTK 测试模型使用统一三角面或统一四边形面，以兼容当前 iGame 表面读取和显示流程。
