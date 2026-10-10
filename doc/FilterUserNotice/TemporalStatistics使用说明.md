# TemporalStatistics 滤波器使用说明

> - 头文件:`iGameCore/Filters/Animation/iGameTemporalStatistics.h`
> - 实现文件:`iGameCore/Filters/Animation/iGameTemporalStatistics.cpp`
> - 菜单入口:`算法处理 -> 开发中filter/第二批 -> 时域统计 (Temporal Statistics)`
> - 对标实现:ParaView 的 Temporal Statistics(演示视频见 PR 附件)

---

## 一、功能

对带时间序列的模型,对**每个点 / 每个单元**在时间维度上统计所选数组的**平均值、最小值、最大值**,结果作为一个**新模型**加入模型树;输入模型全程只读。

- **逐元素统计**:每个点(`IG_POINT`)或每个单元(`IG_CELL`)各得一份统计值,颜色映射后逐单元 / 逐点呈现不同颜色
- **全部数组一起统计**:没有"选哪个数组"的参数,一次执行把输入里的每个数组都统计一遍(对标 ParaView 的默认全选)
- **统计量固定为平均 / 最小值 / 最大值**,输出名固定为 `<数组名>_average` / `<数组名>_minimum` / `<数组名>_maximum`
- **向量数组逐分量统计**,输出与源同维的向量(3 分量位移得到 3 分量的 `U_average`);标量数组输出标量
- **输出是新数据集**:点坐标与单元拓扑沿用输入(共享,省内存),属性集是全新的;执行后自动选中第一个统计数组,便于直接着色查看
- **时序帧类型兼容**:`MultiSubFiles`(PVD 逐帧多文件 / 多子文件)与 `SingleFieldAttributes`(单场属性帧,如 ODB 场数据)

**适用场景**:统计某一物理量在整个时间域上的极值与均值分布(例如"每个单元在整个过程中的最大应力""每个点的时间平均速度"),用于和逐帧动画对照观察关键区域。

---

## 二、调用方式

### 2.1 主要接口

| 接口 | 说明 |
| --- | --- |
| `static Pointer New()` | 创建滤波器 |
| `SetInput(DataObject::Pointer)` | 设置输入数据(基类接口),输入需带 `StreamingData` 时间序列 |
| `bool Execute()` | 执行,失败返回 `false`(基类接口) |
| `std::string GetMessage()` | 执行失败原因(帧数不足 / 数组在各帧不一致 / 几何类型不支持等) |
| `DataObject::Pointer GetOutput(0)` | 获取输出(新数据集),交给模型树接口即可成为新模型 |

该滤波器**不提供参数**:不选数组、不选统计量、不改后缀、不选分量。

### 2.2 典型调用步骤

1. `iGameTemporalStatistics::New()` 创建滤波器
2. `SetInput(dataObject)` 设置带时间帧的模型
3. `Execute()` 执行
4. `GetOutput(0)` 取回新数据集,`SetName(...)` 命名(默认 `<输入名>_temporal_statistics`)
5. 交给 `igQtModelDialogWidget::addDataObjectToModelTree()` 挂成新模型节点

---

## 三、使用示例

### 3.1 界面操作(推荐)

1. 打开含多时间步的数据(PVD 或多个时间步文件组成的序列),在模型树中选中该模型
2. 点击菜单 `算法处理 -> 开发中filter/第二批 -> 时域统计 (Temporal Statistics)`
3. 模型树新增 `<输入模型名>_temporal_statistics`,展开可见各数组的 `_average` / `_minimum` / `_maximum`
4. 结果模型会自动选中第一个统计数组并着色;想换别的统计量,在模型树点选对应属性行或在标量场面板选择
5. 需要整段动画颜色稳定时,把标量场面板的**映射范围模式**设为「全局固定」或「只扩不缩」

### 3.2 C++ 调用

```cpp
#include "Animation/iGameTemporalStatistics.h"
#include "iGameFileIO.h"

auto input = iGame::FileIO::ReadFile("./Models/xxx.pvd");   // 带时间序列的数据

auto filter = iGame::iGameTemporalStatistics::New();
filter->SetInput(input);
if (!filter->Execute()) {
    // filter->GetMessage() 说明失败原因
}
auto output = filter->GetOutput(0);     // 新数据集,输入未被修改
```

输出属性集里会出现输入中**每个数组**的三份统计,例如输入有 `stress`(单元)与 `U`(点、3 分量):

| 输入数组 | 输出数组 |
| --- | --- |
| `stress`(IG_CELL,标量) | `stress_average` / `stress_minimum` / `stress_maximum`(标量) |
| `U`(IG_POINT,3 分量) | `U_average` / `U_minimum` / `U_maximum`(3 分量向量) |

### 3.3 数值自检(两帧)

用两帧、每个单元 / 点各一个值的标量数组验证:

| 数组 | 帧 0 | 帧 1 | `_average` | `_minimum` | `_maximum` |
| --- | --- | --- | --- | --- | --- |
| `stress`(2 个单元) | `1, 2` | `5, 7` | `3, 4.5` | `1, 2` | `5, 7` |
| `temp`(4 个点) | `0, 10, 20, 30` | `2, 12, 22, 32` | `1, 11, 21, 31` | `0, 10, 20, 30` | `2, 12, 22, 32` |

平均值是**逐元素对帧取平均**(除以帧数),不是除以元素个数;每个元素的值应互不相同。

---

## 四、注意事项

- **至少 2 帧**:单帧模型没有可统计的时间维度,菜单会提示「当前模型没有多帧时间序列(至少需要 2 帧)」
- **各帧数组必须一致**:同名数组在各帧的元素数与维度必须相同,否则执行失败并在消息里指出是第几帧、哪一块、哪个数组
- **统计粒度是元素级**:结果是"每个单元 / 每个点一个值";若整片模型显示同一颜色,先确认是否已经选中了某个统计数组
- **向量取的是逐分量统计**:输出向量的模长是"平均向量的模长",与"模长的平均"不是同一个量
- **几何共享**:输出与输入共享点坐标与单元拓扑,不要在任一模型上做会改拓扑的操作(合并 / 删除单元 / 简化);需要独立副本时把 `MakeGeometryCopy` 换成深拷贝(参考 `iGamePointAndCellIdsFilter.cpp` 的 `DeepCopyDataObject`)
- **多块时序**:每帧含多个块时,输出是"容器根 + 每块一个子对象",统计数组挂在各子块上,根行没有数组
- **几何类型支持范围**:`UnstructuredMesh`、`SurfaceMesh`、`VolumeMesh`(基础)、`PointSet` 已支持;多面体体网格的面-体索引、`StructuredMesh`、`LagrangeUnstructuredMesh` 尚未支持,`Execute()` 会返回失败并说明
- **输出命名固定**为 `<输入名>_temporal_statistics`,重复执行会得到同名多份,需要区分请自行改名
- **帧缓存**:执行时会按需 `EnableCache(frameNum)` 缓存全部帧,大网格长时序注意内存占用;统计过程不改动当前播放帧
- **未实现**:ParaView 的**标准差、方差**默认关闭项本实现未提供;如需扩展,可在 `Accumulator` 中增加平方和累加与对应后缀

---

## 五、相关文件

| 路径 | 说明 |
| --- | --- |
| `iGameCore/Filters/Animation/iGameTemporalStatistics.h` | 滤波器声明(无参数):`Accumulator`(每个块 × 每个数组一个)、`SourceInfo` |
| `iGameCore/Filters/Animation/iGameTemporalStatistics.cpp` | 执行流程:解析全部数组 -> 逐帧取块 -> 逐元素累加 -> 建输出骨架并写入统计数组 |
| `iGameCore/Filters/iGameFilterIncludes.h` | 滤波器总头,Qt 侧通过它引用 |
| `Qt/src/IQCore/igQtMainWindow.cpp` | 菜单入口「开发中filter/第二批 -> 时域统计 (Temporal Statistics)」 |
