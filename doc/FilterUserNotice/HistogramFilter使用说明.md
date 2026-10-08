# HistogramFilter（属性直方图）使用说明

统计属性数组的取值分布（直方图）。参数与算法逐项对齐 VTK 的 `vtkExtractHistogram`，也就是 ParaView 里的 **Histogram** 过滤器（对照版本：ParaView 6.2.0 / VTK 9.7.0）。

- 核心实现：`iGameCore/Filters/Histogram/iGameHistogramFilter.h/.cpp`
- 结果容器：`iGameCore/Core/Common/iGameHistogramData.h`（`HistogramData`）
- 图表控件：`Qt/include/IQWidgets/igQtHistogramChartWidget.h`、`Qt/src/IQWidgets/igQtHistogramChartWidget.cpp`（放在主窗口底部 Dock 里）
- Qt 入口：菜单「算法处理 → 属性直方图 (Histogram)」

## 1. 功能与输出

| 输出数组 | 类型 | 含义 | 长度 | 出现条件 |
| --- | --- | --- | --- | --- |
| `bin_extents` | DoubleArray（VTK 为 vtkDoubleArray） | 每个 bin 的中心 | 分箱数 | 总是 |
| `bin_values` | IntArray；`Normalize` 打开时是 DoubleArray | 每个 bin 的计数（或归一化频率） | 分箱数 | 总是 |
| `bin_accumulation` | 与 `bin_values` 同类型 | 计数的累计值（累计分布） | 分箱数 | `Accumulation` 打开 |
| `<数组名>_total` / `<数组名>_average` | DoubleArray（维数同源数组） | 属性集里**其余每个数组**在各 bin 上的累加值 / 均值 | 分箱数 | `CalculateAverages` 打开 |

数组名与 VTK 输出表 `vtkTable` 的列名一致，便于逐元素比对。另外 `HistogramData` 还附带统计范围（min/max）、来源数组名、来源挂载位置（点/单元）、是否已归一化、实际计入统计的元组数。

## 2. 参数（与 ParaView 面板一一对应）

ParaView 6.2 的 Histogram 面板属性实测为：`BinCount, CalculateAverages, CenterBinsAroundMinAndMax, Component, CustomBinRanges, Input, Normalize, SelectInputArray, UseCustomBinRanges`。

| 参数 | 接口 | ParaView 面板对应项 | 默认值 |
| --- | --- | --- | --- |
| 数组选择 | `SetAttributeIndex(int)` | `SelectInputArray` | -1：沿用输入对象当前属性（`DataObject::GetAttributeIndex()`） |
| 分箱数 | `SetNumberOfBins(int)` | `BinCount` | 10；小于 1 时按 1 处理 |
| 分量 | `SetComponent(int)` | `Component` | 0；等于数组维数时按模长 `sqrt(Σv²)` 统计 |
| 居中分箱 | `SetCenterBinsAroundMinAndMax(bool)` | `CenterBinsAroundMinAndMax` | false |
| 自定义范围 | `SetUseCustomBinRanges(bool)` + `SetCustomBinRanges(min, max)` | `UseCustomBinRanges` + `CustomBinRanges` | false；范围默认 [0, 1] |
| 归一化 | `SetNormalize(bool)` | `Normalize` | false |
| 计算均值 | `SetCalculateAverages(bool)` | `CalculateAverages` | false |
| 累计 | `SetAccumulation(bool)` | ——（**该 VTK 选项没有被 ParaView 面板暴露**，仅核心 API 提供） | false |

Qt 面板暴露除「累计」外的全部 7 项（与 ParaView 面板一致）；「累计」只在代码里可用。

## 3. 调用方式

```cpp
auto filter = iGame::HistogramFilter::New();
filter->SetInput(object);                 // DataObject::Pointer
filter->SetAttributeIndex(idx);           // 属性集下标（点属性或单元属性均可）
filter->SetNumberOfBins(20);
filter->SetComponent(0);
filter->SetCenterBinsAroundMinAndMax(false);
// filter->SetUseCustomBinRanges(true); filter->SetCustomBinRanges(-5.0, 25.0);
// filter->SetNormalize(true); filter->SetCalculateAverages(true); filter->SetAccumulation(true);
if (!filter->Execute()) {
    // filter->GetMessage() 给出中文失败原因
}
auto data = filter->GetHistogramData();
data->GetBinExtents();       // DoubleArray::Pointer，bin 中心
data->GetBinValues();        // ArrayObject::Pointer，计数（IntArray）或归一化频率（DoubleArray）
data->GetBinAccumulation();  // ArrayObject::Pointer，累计列（可为空）
data->GetAverageColumns();   // std::vector<std::pair<total, average>>
```

Qt 侧：加载模型 → 菜单「算法处理 → 属性直方图 (Histogram)」→ 面板选择数组、填写分箱数/分量、按需勾选居中分箱、自定义范围（含范围下限/上限）、归一化、计算均值 → 点击应用 → **主窗口底部出现柱状图 Dock**（`dockWidget_Histogram`，可停靠/移动/浮动/关闭）：横轴为 bin 中心（`bin_extents`），纵轴为频次（`bin_values`；归一化时是频率，标签为 `%g`），柱高即计数，标题给出「数组名 / bin 数 / 参与统计的值数 / 取值范围 / 是否归一化」；箱数 ≤ 30 时显示每个 bin 的中心标签，超过则不显示以免压叠。

柱状图与原模型在**同一个窗口内**同时可见。再次应用时复用同一个 Dock 原地刷新（先清掉上一次的系列与坐标轴），并重设标题为当前模型名。

同时还会在 **3D 视图右上角叠加一张紫色的柱状图**（`igQtHistogramChartWidget(rendererWidget, true)`）。注意这张图画的是 **数组数值本身**，不是频次：

- ParaView 侧实测：在 Chart View 里显示**模型**时，图表用 `UseIndexForXAxis = 1`（横坐标 = 索引），纵轴就是所选数组，系列颜色按 ColorBrewer Set1 依次分配 —— `SeriesColor = [Points_Magnitude 黑, Points_X #E41A1C, Points_Y #377EB8, Points_Z #4DAF4A, v (0.60,0.31,0.64) 紫]`。所以文件里第一条点数组（如 `v`）在 ParaView 里就是**紫色**的，柱子高度 = `v` 的数值（例如数据 `v = 0…19` 时看到 (0,0)(1,1)…(19,19) 那一列柱子）；
- 本实现的叠加层即为该图：`DrawArrayValues(数组, 分量, 标题)`，纵轴标题 = 数组名（对应「纵坐标是 v」），横轴 = 索引，柱色 `#984EA3`（`kOverlayBarColor`）；分量等于维数时按模长；元素数 > 1000 时退化为同色折线以免 Qt Charts 卡顿；
- 叠加层是 `rendererWidget`（`igQtRenderWidget : QOpenGLWidget`）的子控件，画在模型之上；右上角对齐的 `QVBoxLayout`（边距 12 px）定位，窗口缩放自动跟随；尺寸固定 320×200；
- 半透明底色 `rgba(18,18,18,190)`、图区 `rgba(0,0,0,120)`，模型可透出；右上角 ✕ 可单独关闭，再次点「应用」重新显示；
- 底部 Dock 那张图仍是**频次直方图**（`bin_extents` × `bin_values`），柱色改为 ParaView 里 `bin_values` 系列的红色 `#E41A1C`（`kDockBarColor`），归一化时纵轴变频率。

## 使用示例

### 1. GUI（iGameVis）

1. 打开模型（例如 `Examples/Models/Convert_Quad_Bicycle.vtk`）；
2. 菜单 **Filters** 下点 **属性直方图 (Histogram)**；
3. 面板 7 个参数：数组（**默认 = 当前显示的数组**）、分量、分箱数、居中分箱、自定义区间、归一化、累加；
4. 点「确定」后：
   - 模型树新增 `*_histogram` 输出，右下角 Dock 直接给出**柱状图**；
   - 3D 窗口**右上角同时叠加一张紫色 `#984EA3` 柱状图**（横轴 = 当前数组取值、纵轴 = 频数），对应 ParaView 运行
     Histogram 后在模型上出现的那张图；切换面板里的数组，叠加图的纵轴标题随之改变（默认跟随当前显示数组，
     与 ParaView 的 `SelectInputArray` 行为一致）。

### 2. 核心库（C++）

```cpp
auto filter = iGame::HistogramFilter::New();
filter->SetInput(model);
filter->SetAttributeIndex(model->GetAttributeIndex());  // -1 = 使用当前活动属性
filter->SetNumberOfBins(30);
filter->SetComponent(0);
filter->Execute();
auto histogram = iGame::DynamicCast<iGame::HistogramData>(filter->GetOutput(0));
// histogram->GetBinExtents() / GetBinValues() / GetBinAccumulation() / GetAverageColumns()
```

### 3. 测试程序

- 可视化（OpenGL 窗口）：`build-msvc\Release\testHistogram.exe` —— 自行车模型 + 右上角紫色直方图；实测点数组
  `test_1` 分量 0、30 bins：`range=[2.18399e-11, 1]`、`bin 0 = 13272`、`bin 29 = 42`、合计 82212，与 ParaView 6.2 逐元素一致。
- 无窗口自检：`build-msvc\Release\testHistogramChecks.exe` —— **44 / 44 通过**（exit 0），双击运行时末尾会暂停等待回车。

### 4. 演示视频

- `doc/FilterUserNotice/videos/属性直方图.mp4` —— iGameVis 与 ParaView 运行同一模型的效果对比；
- `doc/FilterUserNotice/videos/两个测试程序-追加弧长和直方图.mp4` —— 两个测试程序的运行录像。

## 注意事项

- 一次只统计**一个数组的一个分量**；分量号超出数组维数会直接报错「所选分量超出数组维数。」，不会静默退回 0。
- 默认按 **bin 中心**给区间：居中分箱时 `delta = (max-min)/(N-1)`、`bin_extents[i] = min + i*delta`；不居中时 `delta = (max-min)/N`、
  `bin_extents[i] = min + i*delta + delta/2`，与 `vtkExtractHistogram` 一致。
- **blanking**：带 `vtkGhostType` 的元组不参与统计（点 `HIDDENPOINT=2` / `DUPLICATEPOINT=4`，单元 `HIDDENCELL=32` / `DUPLICATECELL=64`），与 ParaView 一致。
- 归一化打开后 `bin_values` 变成 `double` 且总和为 1；不打开时是 `int` 计数。
- 「计算平均值」列对应 VTK 上游一个已知 bug（VTK 自身会输出脏数据）；我们输出数学上正确的 `_total` / `_average`，详见 §6。
- 读取器对 `char` / `unsigned char` 按数值解析、支持 `nan` / `inf` 文本，是本 PR 一并修复的缺陷（详见 §7）。
- 可视化测试程序需要能创建 GL 上下文的环境；无显示器时请改用无窗口自检程序。

## 4. 语义（逐项对齐 `vtkExtractHistogram.cxx`）

1. 统计范围 = 所选数组该分量的 min/max，**排除被 blank 的元组**：点属性看 `vtkGhostType` 的 `HIDDENPOINT|DUPLICATEPOINT` = 2|1，单元属性看 `HIDDENCELL|DUPLICATECELL` = 32|1（常量取自 `vtkDataSetAttributes.h`）；与 `FiniteMinAndMaxWithBlankingFunctor` 一样**跳过非有限值**（NaN/Inf 不参与范围）；
2. `UseCustomBinRanges` 打开时用 `CustomBinRanges`（两端反了先交换，对应 VTK 里的告警分支）；关闭时若范围内一个可用值都没有，按 VTK 的失败分支不产生结果，本实现给出中文提示；
3. `min == max` 时把范围扩成 `[min-0.5, max+0.5]`（`InitializeBinExtents`）；
4. bin 宽度 `bin_delta = (max-min) / (CenterBinsAroundMinAndMax ? BinCount-1 : BinCount)`，
   bin 中心 `bin_extents[i] = min + i*bin_delta + (居中 ? 0 : bin_delta/2)`（`FillBinExtents`）；
5. 计数 `bin = clamp((int)((value - min + (居中 ? bin_delta/2 : 0)) / bin_delta), 0, BinCount-1)`，即等于 max 的值落进最后一个 bin；**计数遍不筛非有限值**：VTK 里 `static_cast<int>(NaN/±Inf)` 得到 `INT_MIN` 后被 Clamp 夹到 0，本实现显式把非有限值计入 bin 0（实测与 VTK 输出一致）；
6. `Normalize` 把 `bin_values` 整列换成 double（`count / 总计数`，`NormalizeBins`）；
7. `Accumulation` 追加与 `bin_values` 同类型的累计列（`AccumulateBins`，在 Normalize 之后累计）；
8. `CalculateAverages` 为属性集里**其余每个数组**追加 `<名>_total` / `<名>_average` 两列（多分量按原维数展开；空 bin 写 0；只统计未被 blank 的元组）；
9. ghost 数组名统一转小写比较（`vtkGhostType` / `vtkghosttype` 都认），与 `iGameRemoveGhostInformationFilter.cpp` 的既有约定一致。

## 5. 数值验证（ParaView 6.2.0 / VTK 9.7.0，逐元素比对）

对比方式：`pvpython` 用 `Histogram(Input=..., SelectInputArray=[关联, 名], BinCount=..., Component=..., <选项>)` 导出输出表全部列为 CSV；核心库侧用临时探针（`--center / --range min max / --normalize / --accum / --averages`）导出同格式 CSV，按列名（忽略大小写）逐元素比对。

### 5.1 默认路径与 blanking（18 个用例）

| 用例 | 数据 | 参数 | 计数和 | bin 中心最大绝对误差 | 判定 |
| --- | --- | --- | --- | --- | --- |
| A | hist_small.vtk 点 `v`（0…19） | 10 bin | 20 = 20 | 0 | [2,2,2,2,2,2,2,2,2,2] |
| B | hist_small.vtk 点 `v` | 4 bin | 20 = 20 | 0 | [5,5,5,5] |
| C | hist_small.vtk 单元 `c`=[1,1,2,3] | 3 bin | 4 = 4 | 0 | [2,1,1] |
| D | hist_constant.vtk 点 `v` 恒为 5 | 5 bin | 5 = 5 | 0 | [0,0,5,0,0]，范围 [4.5,5.5]（min==max 分支） |
| E | hist_negative.vtk 点 `v`（含负值、重复值） | 6 bin | 12 = 12 | 0 | [1,3,2,3,0,3] |
| F | hist_multi.vtk 点 `vec` 分量 0 | 5 bin | 6 = 6 | 0 | [1,1,1,2,1] |
| G | hist_multi.vtk 点 `vec` 模长（Component=3） | 5 bin | 6 = 6 | 0 | [2,1,0,1,2] |
| K | hist_small.vtk 点 `v` | 1 bin | 20 = 20 | 0 | [20]，中心 9.5 |
| L | hist_small.vtk 点 `v` | 100 bin | 20 = 20 | 0 | 20 个 1，其余 0 |
| R1 | Examples/Models/Tet_Plane.vtk 点 `test_1` | 30 bin | 8604 = 8604 | 0 | 前 12 个 [2078,1142,724,569,407,353,318,407,320,391,284,262] |
| R2 | Examples/Models/Tet_Plane.vtk 单元 `test_1` | 30 bin | 30700 = 30700 | 0 | 同名数组在点/单元各一份，按属性下标正确区分 |
| R3 | Examples/Models/Convert_Quad_Bicycle.vtk 点 `test_1` | 50 bin | 82212 = 82212 | 0 | 8 万点规模 |
| K1 | hist_ghost.vtk 点 `v`（ghost [0,0,2,0,1,0] 从文件读入） | 10 bin | 18 = 18 | 0 | [2,1,1,2,2,2,2,2,2,2]：20 点中 2 个被 blank |
| K2 | hist_ghost.vtk 单元 `c`（ghost [0,32,1,0] 从文件读入） | 10 bin | 2 = 2 | 0 | [1,0,0,0,0,0,0,0,0,1] |
| K3 | hist_ghost_classic.vtk 点 `v`（classic SCALARS 写法） | 10 bin | 4 = 4 | 0 | [1,0,1,0,0,0,1,0,0,1] |
| K4 | hist_ghost_classic.vtk 单元 `c` | 10 bin | 1 = 1 | 0 | [0,0,0,0,0,1,0,0,0,0] |
| I | hist_ghost.vtk 点 `v` | 4 bin | 18 = 18 | 0 | [3,5,5,5] |
| J | hist_ghost.vtk 单元 `c` | 3 bin | 2 = 2 | 0 | [1,0,1] |
| I2 | hist_ghost_ptonly.vtk 点 `v` | 4 bin | 4 = 4 | 0 | [2,0,1,1] |
| J2 | hist_ghost_cellonly.vtk 单元 `c` | 3 bin | 1 = 1 | 0 | [1,0,1] |

结论：18 个用例的**计数完全一致、bin 中心逐位相同**（最大绝对误差 0），ghost 数组都是从文件读入的真实数据。

### 5.2 各选项（W 系列，逐列比对）

| 用例 | 数据 / 参数 | 比对结果 |
| --- | --- | --- |
| W1 | hist_small.vtk 点 `v`，10 bin，`CenterBinsAroundMinAndMax` | bin_extents + bin_values 逐元素相同 |
| W2 | hist_small.vtk 点 `v`，10 bin，自定义范围 [-5, 25] | 同上 |
| W3 | hist_small.vtk 点 `v`，10 bin，`Normalize` | 同上（double 列，含 1/20 = 0.05 等值） |
| W5 | hist_ghost.vtk 点 `v`，`CalculateAverages` | 4 列（bin_extents / bin_values / `vtkGhostType_total` / `vtkGhostType_average`）逐元素相同 |
| W6 | hist_ghost.vtk 点 `v`，4 bin，居中分箱 | 逐元素相同 |
| W7 | hist_multi.vtk 点 `vec` 模长（Component=3），5 bin，`Normalize` | 逐元素相同 |
| W8 | hist_nan.vtk 点 `v` = [1,nan,2,inf,3,4,5,6]，4 bin | 逐元素相同，计数 [4,1,1,2]（依赖读取器 nan/inf 修复 + 非有限值入 bin 0） |
| W9 | hist_ghost_classic.vtk 单元 `c`，10 bin | 逐元素相同；`bin_accumulation` 由累计前缀和自校验通过 |
| W10 | Tet_Plane.vtk 单元 `test_1`（30700 单元），30 bin，自定义范围 [0, 0.5] | 逐元素相同；累计前缀和自校验通过 |
| W11 | SurfaceNormalsFilter_test.vtk 点 `Displacement`，8 bin，`CalculateAverages` | bin_extents/bin_values 相同；其余数组的 `_total`/`_average` 列**不同**，原因是 VTK 自身缺陷（见 6.1） |
| W12 | hist_negative.vtk 点 `v`，6 bin，`Normalize` + 自定义范围 [-12, 4] | 逐元素相同 |
| W13 | hist_multi.vtk 点 `vec` 分量 1，7 bin，居中分箱 | 逐元素相同 |

### 5.3 回归测试用例（`Examples/Filter/Histogram/TestHistogramChecks.cpp`）

- 测试模型（每个 filter 只用一个）：`Examples/Models/Histogram_test.vtk`
  —— 20 个点 / 4 个三角形；点数组 `v`（1 分量）与 `vec`（3 分量），单元数组 `c`；
  点与单元的 `vtkGhostType` 都写在 `FIELD FieldData` 里。这一个模型同时覆盖 legacy VTK
  读取器的 `unsigned_char` 数值解析、FIELD 数组名小写化，以及 blanking 元组不参与统计。
- 期望值全部取自 ParaView 6.2 / VTK `vtkExtractHistogram` 实测并硬编码在测试里防漂移：
  `v` 10 bin 计数 `[2,1,1,2,2,2,2,2,2,2]`（总计 18、范围 [0,19]）、居中分箱首/末边界 `0 / 19`、
  `vec` 分量 1 与模长（分量 3 = 维数）的边界 `1.9 / 36.1` 与 `2.1242645786248 / 40.361026993871`、
  单元 `c` 的 3 bin / 10 bin 计数 `[1,0,1]` / `[1,0,…,0,1]`。
- 断言覆盖：默认路径与 blanking、居中分箱、分量选择与模长、自定义范围、单元属性与单元 blanking、
  归一化（总和 = 1、每箱 = 计数 / 18）、累积（`bin_accumulation` 前缀和）、`CalculateAverages`
  （每箱 `average × 计数 = total`、`vec` 分量 0 的 total 合计 = 184）、错误路径
  （分量越界、属性下标越界、`-1` 沿用当前属性）。
- 运行方式（**直接可运行的 exe**，不依赖 `Examples` / HDF5）：

  ```powershell
  cmake -S . -B build -DIGAME_BUILD_FILTER_CHECKS=ON     # 开关默认 OFF，不影响常规构建
  cmake --build build --config Release --target testHistogramChecks
  .\build\Release\testHistogramChecks.exe                # 失败返回非 0，可直接双击
  ```

  构建时会把测试模型拷到 `<exe 目录>/Models/`，exe 自己会依次在「当前工作目录 / exe 同目录 /
  `Examples/Models` / 仓库相对路径」里找模型，所以从任何目录运行都能跑（已实测）。
  具备 HDF5 的环境也可以走 `Examples` 里的同名 target（`-DEXAMPLE_COMPILE=ON` + 工作目录 `Examples`）。
- 实测结果：在 `build-msvc\Release` 直接运行 → **通过 44 / 44，失败 0**（exit 0）；
  从仓库根目录运行同样 44 / 44。

**可视化测试（OpenGL 窗口）**：`Examples/Filter/Histogram/TestHistogram.cpp` →
`build-msvc\Release\testHistogram.exe`（写法与 `Examples/Filter/Convert/TestResampleToImage.cpp` 一致）。

- 读取 `Models/Convert_Quad_Bicycle.vtk`（自行车模型，82212 点 / 96668 单元），对**点数组 `test_1`
  的分量 0** 做 30 bin 直方图；3D 视图显示模型并用该数组着色；
- 再用 `Scene::GetPainter2D()`（像素坐标、关深度测试）在窗口**右上角叠加紫色 `#984EA3` 柱状图**，
  对应 ParaView 运行 Histogram 后在模型上出现的那张图；
- 控制台打印 `bin_extents` / `bin_values`：实测 `range=[2.18399e-11, 1]`、`bin 0 = 13272`、
  `bin 29 = 42`、合计 82212，与 ParaView 6.2 逐元素一致（`extent[0]=0.0166667`、`extent[29]=0.983333`）；
- 构建：`cmake --build build --config Release --target testHistogram`（同样由 `IGAME_BUILD_FILTER_CHECKS` 控制）；
  实测窗口启动后持续运行 15 秒以上无异常退出。
- 本机限制：当前 checkout 缺少 HDF5，`Examples` 目录无法配置（`find_package(HDF5 REQUIRED)` 失败），
  所以上面的独立 exe 路径是本机验证实际使用的路径。

## 6. 与 VTK / ParaView 的差异

1. **VTK 的 `CalculateAverages` 在 bin 0 为空时会输出未初始化内存（本实现不复现）**
   `vtkExtractHistogram.cxx:798` 用 `numComps = iter.second.TotalValues[0].size()` 推断列的分量数；当第一个 bin 没有任何元组时 `TotalValues[0]` 为空，`numComps` 变成 0，随后的 `SetValue(i*numComps+j, ...)` 只写第 0 个位置，其余分量留成未初始化内存。实测 W11（第一个 bin 为空）ParaView 输出 `1.18e-311` 之类的非规格化数，而本实现按**源数组的实际维数**展开，结果是确定且正确的。bin_extents / bin_values 不受影响，仍然逐元素相同。
2. **输出容器**：VTK 输出 `vtkTable`（行 = bin）；iGameVis 没有表类型，用 `HistogramData`（`DataObject`）承载同名的各列，再由 `igQtHistogramChartWidget` 画成柱状图显示。该对象只作为结果容器，不加入场景/模型树。
3. **呈现方式**：本实现把结果画成柱状图，既能停靠在主窗口底部（`igQtHistogramChartWidget` + `QDockWidget`），也会在 3D 视图右上角叠加一张紫图（`overlay = true`），对应 ParaView 用 Chart View / 叠加面板显示 `bin_extents`（X）与 `bin_values`（系列）。ParaView 默认的 Spreadsheet 表格视图未实现（ParaView 的 SpreadsheetView 只能由 GUI 创建，`pvpython` 里也不可用）。
   实测补充：ParaView 6.2 的 Render View 本身**没有图表叠加层属性**（`ListProperties()` 里无任何 overlay/chart 项），且 `Show(histogram, renderView)` 在无界面模式下报 “Could not create a representation object for proxy Histogram”，即那张紫图来自 ParaView 自动创建的 Chart View 面板。同一个 Chart View 里会同时出现两条来源的系列：**模型的数组**（横坐标取索引，`v` 为紫色）与**直方图表格**（横坐标取 `bin_extents`，`bin_values` 为红色 `#E41A1C`）；本实现分别对应 3D 视图上的叠加层与底部 Dock。
4. **范围内无可用值**：VTK 走失败分支、不产生输出表（只在调试输出里提示），本实现返回 `false` 并在 Qt 面板给出中文提示「所选数组没有可统计的值（可能所有元组都被 blanked，或取值全为非有限值）。」，这也是用户最初报错的定位依据。
5. **`Accumulation`**：VTK 有该选项、ParaView 面板没有暴露；本实现在核心 API 提供并在文档中标注，Qt 面板不显示。

## 7. 本次同时修复的读取器缺陷（`iGameCore/IO/iGameFileReader.*`）

### 7.1 char / unsigned char 按数值解析

根因：`Read(char*)` / `Read(unsigned char*)` 在 ASCII 模式下直接取一个**字符**（`*this->IS`）并只把游标前进 1 字节，而其它整型重载走 `mAtoi` 按数值解析。于是：

- `unsigned char` / `char` 数组的取值变成字符的 ASCII 码（`'0'` → 48）；
- 一次只前进 1 字节，导致数组之后的解析整体错位——同一文件里 `CELL_DATA` 段之后的 `POINT_DATA` 段会整段丢失。

修复：两个重载在 ASCII 模式下改用 `mAtoi` 数值解析（二进制分支保持逐字节读取不变）。由此一并解决：

| 现象 | 修复前实测 | 修复后实测（对照 VTK） |
| --- | --- | --- |
| legacy `FIELD` 写法里 `vtkGhostType` 被读成 ASCII 码 | hist_ghost_ptonly.vtk 点 ghost = [48,48,48,48,48,48] | [0,0,2,0,1,0]；hist_ghost.vtk 直方图计数与 VTK 完全一致（K1/K2/I/J/I2/J2） |
| `CELL_DATA` 之后的 `POINT_DATA` 整段丢失 | hist_ghost.vtk 读到 2 个属性 | 读到 4 个属性（`c`、`vtkghosttype`、`v`、`vtkghosttype`） |
| classic `SCALARS` 文件里点段之后的单元段丢失 | hist_ghost_classic.vtk 只读到点段 | 点段 + 单元段都读到（K3/K4） |
| 用户侧报错「所选数组没有可统计的值」 | hist_ghost.vtk 两个数组都报错（ghost 被读成 48，`48 & 32 ≠ 0` → 所有单元都被判为 blanked） | 两个数组都出结果且与 ParaView 一致 |

### 7.2 nan / inf / infinity 文本值

根因：`mAtof` 只认数字/小数点/指数，遇到 `nan`、`inf` 既解析不出值，也**不会推进游标**，于是该数组从该位置起的所有值都被读成 0。修复：`Read(float*)` / `Read(double*)` 在调用 `mAtof` 之前先经 `TryReadNonFinite()` 识别 `nan` / `inf` / `infinity`（大小写、正负号都支持），识别成功则把游标越过整个词。

| 数据 | 修复前 | 修复后（对照 VTK） |
| --- | --- | --- |
| hist_nan.vtk 点 `v` 文本值 [1,nan,2,inf,3,4,5,6] | [1,0,0,0,0,0,0,0] | [1,nan,2,inf,3,4,5,6]，直方图计数与 VTK 完全一致（W8） |

回归：仓库内 12 个模型、34 个属性的批量扫描全部正常（无报错）；`testResampleOctreeChecks` 63/63 通过。

## 8. 仍未修复的既有缺陷（与本次改动无关）

| # | 现象 | 复现与实测 |
| --- | --- | --- |
| 1 | `.vtu`（XML）里的 `vtkGhostType` 完全不进属性集 | hist_ghost.vtu 点/单元各一个 ghost 数组，读取后属性集只有 `v`、`c`；原因是 `iGameVTUReader` 的 `DataArray` 类型分支只认 Float32/Float64/Int32，blanking 对 `.vtu` 不生效 |
| 2 | legacy `FIELD FieldData` 里的数组名被写成小写 | `SurfaceNormalsFilter_test.vtk` 的 `Velocity/Acceleration/Force/Contact/vtkOriginalPointIds` 读入后变成 `velocity/acceleration/...`，ParaView 保持原大小写。直方图本身不受影响，但 `CalculateAverages` 的列名会带上小写名（比对时忽略大小写）。该行为是 `ReadFieldData` 的既有实现，未在本次改动中调整 |

## 9. 构建与验证记录

- `cmake --build build-msvc --config Release --target iGameVis --parallel` 通过（0 错误，产物 `build-msvc/Release/iGameVis.exe`，2026-09-30 11:15:56，含 7 项参数 + Dock 柱状图 + 3D 视图紫色叠加层）；
- 冒烟测试：启动新构建的 `iGameVis.exe`，20 秒后进程存活、Windows 应用程序日志无错误事件（叠加层的视觉效果与交互需人工确认，不能在无界面环境里自动验证）；
- `testResampleOctreeChecks`：通过 63 / 63，失败 0；
- 仓库模型批量扫描：12 个模型 / 34 个属性全部正常；
- 数值比对：默认路径 + blanking 18 个用例、选项 W 系列 12 个用例（W1–W13，W4 因 ParaView 未暴露 `Accumulation` 改为前缀和自校验）与 ParaView/VTK 逐元素一致。

验证命令（临时探针，验证后已删除；探针源码 `_tmp_histfull.cpp` 与 CMake 目标 `_tmpHistFull` 不在提交范围内）：

```powershell
build-msvc\Release\_tmpHistFull.exe --file <模型> --array <名|POINTS:名|CELLS:名> --bins <n> --comp <n> --csv <out> \
    [--center] [--range min max] [--normalize] [--accum] [--averages]
pvpython _pv_hist_opts.py        # 生成 VTK 参考 CSV（每个选项一个用例）
pvpython _cmp_hist_wide.py <pv.csv> <igv.csv> <用例名>   # 按列名（忽略大小写）逐元素比对
```
