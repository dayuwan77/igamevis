# IntegrateVariablesFilter 使用说明

> Filter 名称：**IntegrateVariablesFilter（变量积分 / Integrate Variables）**
>
> 代码位置：`iGameCore/Filters/IntegrateVariables/iGameIntegrateVariablesFilter.{h,cpp}`

---

## 一、功能

`IntegrateVariablesFilter` 按网格单元的长度、面积或体积，对输入模型中的 Point Data 和 Cell Data 进行积分。其线性积分语义与 ParaView 的 Integrate Variables 线性策略保持一致。

Filter 会先确定输入网格中可积分单元的最高维度，并且只积分该维度的单元：

| 最高维度 | 积分测度 | 输出测度属性 |
| --- | --- | --- |
| 1D 线单元 | 长度 | `Length` |
| 2D 面单元 | 面积 | `Area` |
| 3D 体单元 | 体积 | `Volume` |

例如，一个网格同时包含线、三角形和四面体时，最高维度为 3，因此只积分四面体，线和三角形不参与最终结果。

### 1. Point Data 积分

点属性按照单元内的线性插值进行积分。实现中将单元测度分配给组成该单元的顶点，再对各顶点属性进行加权累加。

以一个四面体为例：

```text
点属性积分 = 四面体有符号体积 × 四个顶点属性的平均值
```

多个单元的最终结果为所有最高维度单元积分结果之和。

### 2. Cell Data 积分

默认情况下，每个单元属性乘以该单元的长度、面积或体积后累加：

```text
Cell Data 积分 = Σ（单元属性值 × 单元测度）
```

开启 `Divide Cell Data By Volume`（界面显示为“单元数据除以总测度”）后，Cell Data 的积分结果会再除以总测度，得到测度加权平均值：

```text
Cell Data 加权平均值 = Σ（单元属性值 × 单元测度）/ Σ（单元测度）
```

该选项只影响 Cell Data，不影响 Point Data、输出位置以及 `Length`、`Area`、`Volume` 总测度。

### 3. 输出

Filter 不修改输入模型，而是生成一个新的 `UnstructuredMesh`：

- 输出节点名称：`原模型名_IntegrateVariables`；
- 输出包含一个点和一个 `Vertex` 单元；
- 输出点位于网格的测度加权积分中心；
- Point Data 积分结果仍保存为 Point Data；
- Cell Data 积分结果仍保存为 Cell Data；
- 额外生成一个 `Length`、`Area` 或 `Volume` Cell Data 数组；
- 输出属性统一使用双精度数组保存。

长度、面积和体积的中间计算使用 `double` 精度，避免在几何向量运算阶段提前丢失精度。

---

## 二、调用方式

### 方式 1：C++ 代码调用

```cpp
#include <IntegrateVariables/iGameIntegrateVariablesFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile("model.vtk");
if (!input) {
    return;
}

auto filter = iGame::IntegrateVariablesFilter::New();
filter->SetInput(input);

// false：输出 Cell Data 的积分值；
// true：将 Cell Data 积分值除以总长度、总面积或总体积。
filter->SetDivideAllCellDataByMeasure(false);

if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << std::endl;
    return;
}

auto output = filter->GetOutput();
std::cout << "Dimension: " << filter->GetIntegrationDimension() << '\n';
std::cout << "Cells: " << filter->GetIntegratedCellCount() << '\n';
std::cout << filter->GetMeasureName() << ": "
          << filter->GetIntegratedMeasure() << '\n';
```

常用接口：

| 接口 | 说明 |
| --- | --- |
| `SetDivideAllCellDataByMeasure(bool)` | 设置是否将 Cell Data 积分值除以总测度，默认 `false` |
| `GetDivideAllCellDataByMeasure()` | 获取上述选项 |
| `GetIntegrationDimension()` | 获取本次积分维度：1、2 或 3 |
| `GetIntegratedCellCount()` | 获取实际参与积分的单元数量 |
| `GetIntegratedMeasure()` | 获取总长度、总面积或总体积 |
| `GetMeasureName()` | 获取测度名称：`Length`、`Area` 或 `Volume` |
| `GetMessage()` | 获取执行结果或失败原因 |

### 方式 2：iGameVis 界面调用

1. 打开 iGameVis 并加载一个网格模型；
2. 在模型树中选中原始模型；
3. 选择菜单：**算法处理 → 变量积分 (Integrate Variables)**；
4. 积分策略选择 `Linear Strategy`；
5. 根据需要勾选“单元数据除以总测度”；
6. 点击“执行”；
7. 执行成功后，模型树中生成新的积分结果节点，并自动打开“查找信息”面板；
8. 在“点数据”和“面数据”之间切换，查看 Point Data、Cell Data 及总测度。

当前版本只实现并提供 `Linear Strategy`，未提供 Gaussian Strategy。

### 方式 3：运行示例程序

示例代码：

```text
Examples/Filter/FeatureExtraction/IntegrateVariables.cpp
```

编译与运行：

```powershell
# 在 iGameVis 仓库根目录中执行
cmake --build build --config Release --target testIntegrateVariables -- /m

Set-Location .\build\Examples
.\Release\testIntegrateVariables.exe
```

示例依次读取线、面、体三个模型，并分别显示不开启和开启“单元数据除以总测度”两种情况下的最终数据：

| 维度 | 测试模型 | 主要属性 |
| --- | --- | --- |
| 1D 线 | `vase2.vtk` | Point Data `height`、Cell Data `segment_id` |
| 2D 面 | `SurfaceNormalsFilter_pyramid_roof.vtk` | Point Data `PointId`、Cell Data `FaceId` |
| 3D 体 | `AIGen_Tet_TwistedRod.vtk` | `Temperature`、`Pressure`、`MaterialID`、`CellEnergy` 等 |

---

## 三、使用示例

### 1. 线积分示例

`vase2.vtk` 是由 7 条线段组成的花瓶轮廓线，同时包含点属性 `height` 和单元属性 `segment_id`。示例输出：

```text
Divide Cell Data By Volume: Off
Center: 0.618550956249 0 1.96484792233
Point Data: height = 16.9246727451
Cell Data:  segment_id = 38.7800007126
Cell Data:  Length = 10.1386895554

Divide Cell Data By Volume: On
Center: 0.618550956249 0 1.96484792233
Point Data: height = 16.9246727451
Cell Data:  segment_id = 3.82495198227
Cell Data:  Length = 10.1386895554
```

### 2. 面积分示例

`SurfaceNormalsFilter_pyramid_roof.vtk` 是同时包含三角形和四边形的表面网格。示例输出：

```text
Divide Cell Data By Volume: Off
Center: 0.5 0.5 0.569035589695
Point Data: PointId = 24.2900192284
Cell Data:  FaceId = 19.1923881554
Cell Data:  Area = 6.41421356237

Divide Cell Data By Volume: On
Center: 0.5 0.5 0.569035589695
Point Data: PointId = 24.2900192284
Cell Data:  FaceId = 2.99216544145
Cell Data:  Area = 6.41421356237
```

### 3. 体积分示例

`AIGen_Tet_TwistedRod.vtk` 是一个四面体体网格，同时包含：

- Point Data：`Temperature`、`Pressure`、`Displacement`；
- Cell Data：`MaterialID`、`CellEnergy`。

不开启“单元数据除以总测度”时：

```text
Divide Cell Data By Volume: Off
Center: -0.135519579053 -0.418100506067 2.99306607246
Point Data: Temperature = 54.5537092266
Point Data: Pressure = 68.2730114901
Cell Data:  MaterialID = 2.00847404893
Cell Data:  CellEnergy = 3.15911429152
Cell Data:  Volume = 0.741240861809
```

开启该选项后：

```text
Divide Cell Data By Volume: On
Center: -0.135519579053 -0.418100506067 2.99306607246
Point Data: Temperature = 54.5537092266
Point Data: Pressure = 68.2730114901
Cell Data:  MaterialID = 2.70961053607
Cell Data:  CellEnergy = 4.26192679639
Cell Data:  Volume = 0.741240861809
```

可以看到：

- Point Data 的积分结果不变；
- `Volume` 总体积不变；
- Cell Data 从体积分值变为除以总体积后的体积加权平均值。

示例程序只展示各模型的最终积分数据，不额外输出 `PASSED`。读取或执行失败时会在标准错误中显示失败原因，并返回非零退出码。

---

## 四、注意事项

1. **只积分最高维度单元**

   混合维度网格中的低维单元不会参与积分。例如同时存在三角形和四面体时，只积分四面体。

2. **支持的输入数据对象**

   支持 `SurfaceMesh`、`VolumeMesh`、`UnstructuredMesh` 和 `StructuredMesh`。点云等其他数据类型会执行失败。

3. **当前支持的线性单元**
   - 1D：线段、折线；
   - 2D：三角形、四边形和任意顶点数多边形，使用三角扇拆分；
   - 3D：四面体、金字塔、三棱柱和六面体；
   - 当前不支持一般多面体、二次单元和其他高阶单元。

4. **三维体积具有方向符号**

   三维单元按照四面体拆分计算有符号体积。正常网格的单元顶点顺序应保持一致；若输入单元顶点顺序反转，体积和相应积分贡献可能为负。正负混杂通常表示网格单元方向不一致，应先检查输入数据。

5. **除以总测度选项只作用于 Cell Data**

   Point Data 不会被再次除以总体积，这是与 ParaView 选项语义一致的行为。总测度为零时不会执行除法。

6. **结果应在数据表中查看**

   Filter 输出只有一个点，三维窗口中通常看不出明显几何变化。应在“查找信息”面板中切换 Point Data 和 Cell Data 查看积分数值。

7. **重复执行前重新选择原模型**

   每次执行都会创建新的结果节点。如果需要比较不同选项，应先在模型树中重新选中原始模型，再次打开 Filter；不要将上一次的单点积分结果作为下一次输入。

8. **输入属性要求**

   Point Data 元素数应不少于输入点数，Cell Data 元素数应不少于输入单元数；不满足要求的属性不会进入积分输出。
