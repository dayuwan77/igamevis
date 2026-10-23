# ComputeQuartilesFilter（四分位数统计）使用说明

## 1. 功能说明

ComputeQuartilesFilter 对标 ParaView 的 **Compute Quartiles** Filter（VTK 类：`vtkComputeQuartiles`）。

对输入网格的**所有点标量**属性数组，计算 5 项统计量：

| 统计量 | 含义 |
|--------|------|
| Min    | 最小值 |
| Q1     | 第一四分位数（Tukey hinge） |
| Median | 中位数（Q2） |
| Q3     | 第三四分位数（Tukey hinge） |
| Max    | 最大值 |

### 与 ParaView 对齐的口径（依据本机 ParaView 6.1.1 实测）

1. **只统计点数据（point data）标量数组**，单元数据（cell data）数组不参与输出；
2. 分位数采用 **Tukey hinges** 定义，而非线性插值（R7 / Excel PERCENTILE.INC）。

> 实测方法：`D:\Program Files\ParaView 6.1.1\bin\pvpython.exe` 执行
> `OpenDataFile(file)` → `ComputeQuartiles(Input=r)` → `Fetch`，
> 输出为 `vtkTable`，5 行 × N 列，行序固定为 Min / Q1 / Median / Q3 / Max。

### 与 ParaView 的对应关系

| 项目 | ParaView Compute Quartiles | iGameVis |
|------|----------------------------|----------|
| 输出结构 | vtkTable（5 行 × N 列） | 5 顶点 PointSet，每属性一个 `A_Quartiles` 数组 |
| 参与统计的数据 | 仅点数据数组 | 仅点数据数组（全属性模式） |
| 分位数定义 | Tukey hinges | Tukey hinges |
| 属性面板可调项 | 仅 Input（无可调参数） | 「全部点标量属性」为默认（对标行为）；另提供指定单个属性的扩展能力 |
| 行序 | Min / Q1 / Median / Q3 / Max | 数组元素 0..4 对应同名 5 行 |

---

## 2. 调用方式

### 2.1 iGameVis GUI 调用

1. 加载模型文件（支持 VTK、OBJ、STL 等格式）；
2. 点击左上角 **「算法处理」→「新增filter/第二批」→「Compute Quartiles」**；
3. 参数对话框中选择「全部点标量属性 (ParaView 默认行为)」或指定某个点标量属性；
4. 点击「应用」执行，统计结果显示在弹窗中，并在场景中生成一个承载结果的 5 顶点 PointSet。

### 2.2 C++ 代码调用

```cpp
#include <ComputeQuartiles/iGameComputeQuartilesFilter.h>
using namespace iGame;

auto filter = ComputeQuartilesFilter::New();
filter->SetInput(mesh);

// 模式一：全部点标量属性（默认，对标 ParaView）
// 不调用 SetAttributeByName / SetAttributeByIndex 即可

// 模式二：指定单个属性（iGame 扩展能力，ParaView 无对应入口）
// filter->SetAttributeByName("Temperature");
// 或 filter->SetAttributeByIndex(0);

if (filter->Execute()) {
    for (const auto& r : filter->GetResults()) {
        std::cout << r.arrayName << ": Min=" << r.min << " Q1=" << r.q1
                  << " Median=" << r.median << " Q3=" << r.q3
                  << " Max=" << r.max << std::endl;
    }
} else {
    std::cout << "Error: " << filter->GetMessage() << std::endl;
}
```

单属性模式下也可用 `GetMin() / GetQ1() / GetMedian() / GetQ3() / GetMax()`
直接取第一个结果。

---

## 3. 分位数定义（Tukey hinges）

1. 先对样本全排序（`std::sort`）；
2. Median = 排序后正中间的值；样本数为偶数时取中间两个的平均值；
3. Q1 = **下半区**样本的中位数，Q3 = **上半区**样本的中位数；
4. **样本数为奇数时，中位数本身不参与任何一半**（下半区取前 n/2 个，上半区取后 n/2 个）。

### 与线性插值（R7）的差异示例

以 `Quartiles_plane.vtk` 的 Temperature（n=16）为例：

| 行 | Tukey hinges（当前实现 / ParaView） | 线性插值 R7（旧实现） |
|----|------------------------------------|----------------------|
| Min | 10.0 | 10.0 |
| Q1 | **22.5** | 23.75 |
| Median | 32.5 | 32.5 |
| Q3 | **42.5** | 41.25 |
| Max | 55.0 | 55.0 |

以 CellValue（n=9，奇数）为例，可体现「奇数样本排除中位数」规则：

| 行 | Tukey hinges | 线性插值 R7 |
|----|--------------|-------------|
| Q1 | **115.0** | 120.0 |
| Q3 | **165.0** | 160.0 |

样本数较大时（如 n=625，`0.25×(n-1)` 恰为整数），两种定义可能重合；
因此验证分位数定义应使用 n 为偶数或规模较小的模型。

---

## 4. 使用示例

### 示例 1：统计单个点标量属性

1. 打开 `Examples/Models/Quartiles_plane.vtk`；
2. 菜单【算法处理】→【新增filter/第二批】→【Compute Quartiles】；
3. 属性下拉框选择 `Temperature`，点击执行；
4. 弹出统计表显示 5 行统计值：`Min=10 / Q1=22.5 / Median=32.5 / Q3=42.5 / Max=55`
   （Tukey hinges 定义，与 ParaView 6.1.1 实测一致），输出模型命名为 `compute_quartiles_result`。

### 示例 2：一次统计全部点标量属性

属性下拉框选择第一项「全部点标量属性」，执行后统计表逐属性分行
（如 `Temperature`、`Pressure` 各一行），单元属性（`CellValue`）自动跳过。

### 示例 3：重看结果与导出

- 在模型树中选中已有的 `compute_quartiles_result` 节点，统计表直接重新弹出，无需重复计算；
- 点击统计表中的【保存 CSV】可将结果导出为全精度 CSV 文件（17 位有效数字）。

### 示例 4：C++ 代码调用

```cpp
auto quartiles = ComputeQuartilesFilter::New();
quartiles->SetInput(surfaceMesh);   // 任意含点标量属性的网格
quartiles->Execute();
auto result = quartiles->GetOutput(); // 5 顶点 PointSet
// 输出数组名为 Temperature_Quartiles，元素 0..4 依次为 Min/Q1/Median/Q3/Max
```

---

## 5. 注意事项

### 5.1 输入要求

- 输入网格类型不限（SurfaceMesh / VolumeMesh / UnstructuredMesh 均可）；
- 属性维度必须为 1（标量），向量 / 张量属性会被跳过；
- 支持 FloatArray、DoubleArray、IntArray 等所有数值型数组；
- 全属性模式下若输入不含点标量属性，执行失败并返回错误信息。

### 5.2 输出属性

对每个被统计的属性 `A`，在输出 5 顶点 PointSet 上添加名为 `A_Quartiles` 的标量数组
（维度=1，长度=5），元素 0..4 依次为 `[Min, Q1, Median, Q3, Max]`。

### 5.3 遗留接口

`GetMean() / GetSum() / GetStdDev() / GetCount()` 属于 Descriptive Statistics 范畴，
不属于 Compute Quartiles，**已废弃并固定返回 0**。

### 5.4 测试模型

`Examples/Models/` 目录下提供了测试模型：

| 文件名 | 描述 | 属性 |
|--------|------|------|
| Quartiles_plane.vtk | 4×4 平面网格（16 点，9 面） | Temperature（点标量, float）、Pressure（点标量, int）、CellValue（单元标量） |
| ComputeQuartiles_A_sphere.vtk | 球面（182 点，360 三角形） | Temperature（点标量）、Power（单元标量） |
| ComputeQuartiles_B_hill.vtk | 山丘曲面（625 点，576 三角形） | Height、Skew（点标量）、CellLoad（单元标量） |
| ComputeQuartiles_C_hex_volume.vtk | 六面体体网格（180 点，100 单元） | Pressure（点标量）、Energy（单元标量） |

后三个模型的期望值直接取自 ParaView 6.1.1 实测结果。

测试代码位于 `Examples/Filter/TestComputeQuartiles.cpp`，共 7 组用例
（含与 ParaView 实测真值的回归锁），运行后自动输出 PASS/FAIL 统计。
