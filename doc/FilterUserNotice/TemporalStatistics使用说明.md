# TemporalStatistics 滤波器使用说明

> - 头文件:`iGameCore/Filters/Animation/iGameTemporalStatistics.h`
> - 实现文件:`iGameCore/Filters/Animation/iGameTemporalStatistics.cpp`
> - 菜单入口:`算法处理 -> 新增filter/第二批 -> 时域统计 (Temporal Statistics)`
> - 参数面板:左侧 Dock(形态与「重采样到图像」一致),勾选要输出的统计量
> - 对标实现:ParaView / VTK 的 `vtkTemporalStatistics`(参考演示视频见 PR 附件)

---

## 一、功能

对带时间序列的模型,对**每个点 / 每个单元**在时间维度上统计所选数组的**平均值、最小值、最大值、标准差**,结果作为一个**新模型**加入模型树;输入模型全程只读。

| 项目 | 说明 |
| --- | --- |
| 输入 | 带 `StreamingData` 时间序列(≥ 1 帧)的模型;支持 `MultiSubFiles`(PVD 逐帧多文件 / 多子文件)与 `SingleFieldAttributes`(单场属性帧,如 ODB 场数据) |
| 数组范围 | **全部数组**自动参与统计(无"选数组"参数) |
| 统计量 | 平均值 / 最小值 / 最大值 / **标准差**,可在面板上分别勾选,默认**全开** |
| 输出命名 | `<数组名>_average` / `_minimum` / `_maximum` / `_stddev` |
| 输出类型 | **沿用源数组的数据类型**(源是 double 就输出 double,源是整型就输出整型) |
| 统计粒度 | **逐元素**:每个点(`IG_POINT`)/ 每个单元(`IG_CELL`)各得一份值,颜色映射后逐元素变化 |
| 向量 | 逐分量统计,输出与源同维的向量(不额外产生模长分量) |
| 平均口径 | 忽略时间间隔、每帧等权;平均值 = 累加和 ÷ **总帧数** |
| 标准差 | **总体标准差**(平方偏差和 ÷ 总帧数,再开方) |
| 输出对象 | **新数据集**:点坐标与单元拓扑沿用输入(共享),属性集全新;多块输入输出"容器根 + 每块子对象" |
| 容错 | 某帧缺数组 → 跳过该帧的值(仍按总帧数求平均)并提示;数组规模随帧变化 → 丢弃该数组并提示;**都不会整体失败** |
| 时间步记录 | 输出对象的 `Metadata` 里记录本次处理的时间步(`time_steps`,DoubleArray) |

---

## 二、调用方式

### 2.1 面板参数

菜单点击后弹出**左侧**参数面板,面板上有四个勾选框(默认全开):

| 勾选框 | 输出数组 |
| --- | --- |
| 平均值 (Average) | `<数组名>_average` |
| 最小值 (Minimum) | `<数组名>_minimum` |
| 最大值 (Maximum) | `<数组名>_maximum` |
| 标准差 (Standard Deviation) | `<数组名>_stddev` |

面板下半部分是「输入信息与诊断」:显示当前模型的帧数 / 数组数,以及本次执行的非致命提示(哪些数组在某些帧缺失、规模随帧变化被丢弃、单帧输入等)。四个都不勾选时点「执行」会提示至少勾一个。

### 2.2 主要接口

| 接口 | 说明 |
| --- | --- |
| `static Pointer New()` | 创建滤波器 |
| `SetInput(DataObject::Pointer)` | 设置输入(基类接口) |
| `SetComputeAverage / SetComputeMinimum / SetComputeMaximum / SetComputeStandardDeviation(bool)` | 统计量开关,默认全开 |
| `bool Execute()` | 执行;失败返回 `false` |
| `std::string GetMessage()` | 失败原因 |
| `std::vector<std::string> GetWarnings()` / `std::string GetWarningsText()` | 非致命提示(执行成功时也可能非空) |
| `DataObject::Pointer GetOutput(0)` | 取新数据集,交给模型树接口即成为新模型 |

### 2.3 典型调用步骤

1. `iGameTemporalStatistics::New()` 创建滤波器
2. `SetInput(dataObject)` 设置带时间帧的模型
3. 按需 `SetComputeXxx(true/false)` 选择统计量
4. `Execute()` 执行,失败看 `GetMessage()`,成功后可看 `GetWarningsText()`
5. `GetOutput(0)` 取回新数据集并挂到模型树

---

## 三、使用示例

### 3.1 界面操作(推荐)

1. 打开含时间步的数据(PVD 或多个时间步文件组成的序列),在模型树中选中该模型
2. 点击菜单 `算法处理 -> 新增filter/第二批 -> 时域统计 (Temporal Statistics)`,左侧出现参数面板
3. 勾选需要的统计量(默认四个全开),点「执行」
4. 模型树新增 `<输入模型名>_temporal_statistics`,展开可见各数组的 `_average` / `_minimum` / `_maximum` / `_stddev`;结果模型会自动选中第一个统计数组并着色
5. 面板下方会显示诊断信息(例如"数组 X 在第 N 帧缺失,已跳过该帧")
6. 需要整段动画颜色稳定时,把标量场面板的**映射范围模式**设为「全局固定」或「只扩不缩」

### 3.2 C++ 调用

```cpp
#include "Animation/iGameTemporalStatistics.h"

auto filter = iGame::iGameTemporalStatistics::New();
filter->SetInput(input);                                  // 带时间序列的数据
filter->SetComputeAverage(true);
filter->SetComputeMinimum(true);
filter->SetComputeMaximum(true);
filter->SetComputeStandardDeviation(true);                // 四个统计量都能单独开关，这里显式打开标准差
if (!filter->Execute()) {
    // filter->GetMessage() 说明失败原因
}
auto output = filter->GetOutput(0);                       // 新数据集,输入未被修改
// filter->GetWarningsText() 可取非致命提示
```

### 3.3 数值自检(两帧)

用两帧、每个单元 / 点各一个值的标量数组验证(结果已实测):

| 数组 | 帧 0 | 帧 1 | `_average` | `_minimum` | `_maximum` | `_stddev` |
| --- | --- | --- | --- | --- | --- | --- |
| `stress`(2 个单元) | `1, 2` | `5, 7` | `3, 4.5` | `1, 2` | `5, 7` | `2, 2.5` |
| `temp`(4 个点) | `0, 10, 20, 30` | `2, 12, 22, 32` | `1, 11, 21, 31` | `0, 10, 20, 30` | `2, 12, 22, 32` | `1, 1, 1, 1` |

向量数组逐分量同理(3 分量 `velocity`:{1,3}/{2,0}/{3,0}/{4,0} 的 `comp0` 平均为 `2, 1, 1.5, 2`、标准差为 `1, 1, 1.5, 2`)。

### 3.4 容错行为(实测)

| 场景 | 行为 |
| --- | --- |
| 某帧缺某数组 | 不失败;该数组该帧的值不参与累加,但**均值 / 标准差仍除以总帧数**,面板给出提示。例:4 个点的 `temp` 只在第 1 帧存在时,`temp_average = 0, 5, 10, 15`(原值 ÷ 2 帧) |
| 数组元素数 / 维度随帧变化 | 不失败;该数组的统计结果**整体丢弃**,其余数组照常输出,面板给出提示 |
| 只有 1 帧 | 允许执行;平均值 = 原值,最小值 = 最大值 = 原值,标准差 = 0 |

### 3.5 运行内置回归测试

测试程序:`Examples/Filter/Animation/TestTemporalStatistics.cpp`(CMake 注册为 CTest 用例 `testTemporalStatistics`)。
测试数据用 **`test/Animation Test/result-case1`**(401 帧 legacy VTK 序列,每帧 3212 点 / 2880 单元,16 个数组)。

```powershell
# 构建(需 EXAMPLE_COMPILE=ON)后运行
ctest --test-dir build\Examples -C Release -R testTemporalStatistics --output-on-failure

# 或手动运行:默认取前 48 帧;--all 跑全量 401 帧(更慢、内存占用更高)
build\Examples\Release\testTemporalStatistics.exe "C:\igame\git\repos\iGameVis\test\Animation Test\result-case1"
build\Examples\Release\testTemporalStatistics.exe "C:\igame\git\repos\iGameVis\test\Animation Test\result-case1" --all
```

校验内容:四个统计数组是否齐全、隶属 / 维度 / 元素数是否与源一致、**数据类型是否与源一致**;抽样元素(0、1/4、1/2、3/4、末尾)逐分量与"测试自己按帧重算"的参考值比对(平均 / 最小 / 最大 / 总体标准差);`最小值 ≤ 平均值 ≤ 最大值`、`标准差 ≥ 0`;以及输出 `Metadata` 里的 `time_steps` 记录数是否等于帧数。
实测结果:48 帧、16 个源数组、130 个抽样比对值、**913 条断言、0 失败**。数据目录缺失(code-only checkout 未拉 LFS 素材)时程序打印跳过信息并返回 77,CTest 记为 Skipped。

---

## 四、注意事项

- **至少要有时间帧**:完全没有时间序列(0 帧)会失败并提示
- **统计量至少要勾一个**:四个都关掉时执行会失败并提示
- **平均值按总帧数算**:缺数组的帧也计入分母,因此缺帧较多时均值会偏小
- **标准差是总体标准差**(除以总帧数),不是样本标准差(除以 N−1)
- **统计粒度是元素级**:结果是"每个单元 / 每个点一个值";若整片模型显示同一颜色,先确认是否已经选中了某个统计数组
- **向量是逐分量统计**:输出向量的模长是"平均向量的模长",与"模长的平均"不是同一个量
- **几何共享**:输出与输入共享点坐标与单元拓扑,不要在任一模型上做改拓扑的操作;需要独立副本时把 `MakeGeometryCopy` 换成深拷贝(参考 `iGamePointAndCellIdsFilter.cpp` 的 `DeepCopyDataObject`)
- **多块时序**:每帧含多个块时,输出是"容器根 + 每块一个子对象",统计数组挂在各子块上,根行没有数组
- **几何类型支持范围**:`UnstructuredMesh`、`SurfaceMesh`、`VolumeMesh`(基础)、`PointSet` 已支持;多面体体网格的面-体索引、`StructuredMesh`、`LagrangeUnstructuredMesh` 尚未支持,`Execute()` 会返回失败并说明
- **输出命名固定**为 `<输入名>_temporal_statistics`,重复执行会得到同名多份,需要区分请自行改名
- **结果类型沿用输入**:统计数组与源数组**同类型**(整型源的平均值按整型截断);累加过程用 `double`,最后写回源类型
- **时间步记录**:输出对象的 `Metadata` 里有 `time_steps`(DoubleArray),记录本滤波器实际处理的每个时间步,可用于核对统计范围
- **帧缓存**:执行时会按需 `EnableCache(frameNum)` 缓存全部帧,大网格长时序注意内存占用;统计过程不改动当前播放帧
- **与「逐帧属性差值」的区别**:`iGameAttrDiff` 把结果写回输入对象(同一模型加属性),本滤波器产出**新模型**

---

## 五、相关文件

| 路径 | 说明 |
| --- | --- |
| `iGameCore/Filters/Animation/iGameTemporalStatistics.h` / `.cpp` | 滤波器实现:逐帧取块 → 逐元素累加(Welford)→ 写出统计数组 |
| `iGameCore/Filters/iGameFilterIncludes.h` | 滤波器总头,Qt 侧通过它引用 |
| `Qt/include/IQWidgets/igQtTemporalStatisticsWidget.h` / `Qt/src/IQWidgets/igQtTemporalStatisticsWidget.cpp` | 左侧参数面板(统计量勾选 + 输入信息 / 诊断显示) |
| `Qt/Resources/UI/igQtTemporalStatistics.ui` | 面板界面文件 |
| `Qt/src/IQCore/igQtMainWindow.cpp` | 菜单入口「算法处理 -> 新增filter/第二批 -> 时域统计」与左侧 Dock 接线 |
| `Examples/Filter/Animation/TestTemporalStatistics.cpp` | 回归测试(用 `test/Animation Test/result-case1` 的 401 帧序列逐元素校验) |
| `out/tmp_temporal_check/main.cpp` | 本地数值自检探针程序(构造假时序、打印输出数组,不进仓库) |
