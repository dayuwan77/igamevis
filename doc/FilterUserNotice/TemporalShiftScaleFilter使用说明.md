# TemporalShiftScaleFilter 时间平移/缩放 使用说明

对应任务：`temporal_shift_scale` —— 平移与缩放时间轴（依赖时间序列管线）。

## 1. 功能

`TemporalShiftScaleFilter` 对输入对象的**时间轴**做一次线性变换，公式为：

```text
t' = (t + PreShift) * Scale + PostShift
```

也就是先把时间值整体平移，再按系数缩放，最后再平移一次。执行后产出一个**独立输出对象**：
只替换时间轴，**帧数、几何、属性、每帧的元数据（文件路径列表）、帧类型与缓存状态都按原样保留**。
每个时间步对应的模型数据不会被修改，只是"这一帧叫什么时间"变了。

| 参数 | 接口 | 默认值 | 说明 |
| --- | --- | --- | --- |
| 前置平移 | `SetPreShift(float)` | 0 | 缩放之前的时间平移量 |
| 缩放系数 | `SetScale(float)` | 1 | 时间缩放系数；`0` 非法（会把所有帧压到同一时间值） |
| 后置平移 | `SetPostShift(float)` | 0 | 缩放之后的时间平移量 |

执行后可查询结果（界面回显、用例断言都用这几个接口）：

| 接口 | 说明 |
| --- | --- |
| `GetNumberOfTimeSteps()` | 本次处理的时间步数量（与输入帧数相同） |
| `GetInTimeValues()` | 变换前的时间值（按帧索引升序） |
| `GetOutTimeValues()` | 变换后的时间值（与输入一一对应） |

处理规则：

- 时间值是**逐帧独立**计算的，不依赖相邻帧，因此不要求输入时间值单调或等间隔；
- 输入的**时间顺序不变**：第 i 帧变换后仍然是第 i 帧，只换时间值，不重排、不丢弃；
- `Scale` 为 0 时执行失败（输出没有意义），负值合法（会导致时间轴反向）。

## 2. 输入要求

- 输入必须**带时间序列**，即 `DataObject::GetTimeFrames()` 非空；否则 `Execute()` 返回 `false`。
- 常见来源：`.pvd`（PVDReader 读出的序列）、`.igcm` 时间序列等。
- 两类形态都支持：容器（`.pvd`/`.igcm` 读出的根对象，几何在子对象里）与"网格 + 时间轴"；
  输出与输入保持**同类型**。
- 普通单帧网格没有时间步，本过滤器对它无事可做（会直接返回失败）。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开一个带时间序列的模型（例如 `Examples/Models/sequence.pvd`）；
2. 主菜单 **算法处理 → 时间平移/缩放**，弹出参数窗口；
3. 填写 `Pre Shift` / `Scale` / `Post Shift`（默认 0 / 1 / 0，即时间轴不变）；
4. 点窗口内的 **应用**，结果作为**独立节点**加入模型树。

### 3.2 代码方式

```cpp
#include <TimeSeries/iGameTemporalShiftScaleFilter.h>

// t' = (t - 1) * 2 + 3 = 2t + 1：放大两倍、起点从 1 开始
auto filter = iGame::TemporalShiftScaleFilter::New();
filter->SetPreShift(-1.0f);
filter->SetScale(2.0f);
filter->SetPostShift(3.0f);
filter->SetInput(object);                 // 带时间序列的对象
if (filter->Execute()) {
    auto output = filter->GetOutput();    // 独立输出对象（只换了时间轴）
    auto outTimes = filter->GetOutTimeValues();
} else {
    std::cout << "TemporalShiftScale ERROR!" << std::endl;  // 输入没有时间序列，或 Scale 为 0
}
```

### 3.3 命令行测试

```bash
cd Examples
./testTemporalShiftScale          # 或 ctest -R testTemporalShiftScale
```

用例读取 `Examples/Models/sequence.pvd`（21 帧），按 `t' = 2t + 1` 变换并打印结果，
全部符合预期时输出 `PASS` 且退出码为 0。

## 4. 使用示例

输入模型 `Examples/Models/sequence.pvd`：21 帧，帧索引 0 ~ 20，时间值 0 ~ 10（步长 0.5，单位由数据作者定义）。

| 帧索引 | 0 | 1 | 2 | 3 | … | 20 |
| --- | --- | --- | --- | --- | --- | --- |
| 原时间值 | 0 | 0.5 | 1 | 1.5 | … | 10 |

用 `PreShift = -1`、`Scale = 2`、`PostShift = 3`（即 `t' = 2t + 1`）后的输出：

```text
Time steps  : 21
Input time  : 0 0.5 1 1.5 2 2.5 3 3.5 4 4.5 5 5.5 6 6.5 7 7.5 8 8.5 9 9.5 10
Output time : 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21
First / last: 1 / 21
Result: PASS
```

可以看到：**帧数仍是 21**，几何与属性都没变，只是时间值整体变成 1 ~ 21。

常见用法：

| 目的 | PreShift | Scale | PostShift | 效果 |
| --- | --- | --- | --- | --- |
| 时间轴平移到从 0 开始 | `-t0`（t0 = 原起始时间） | 1 | 0 | 起点变 0，间隔不变 |
| 时间单位换算（秒 → 毫秒） | 0 | 1000 | 0 | 时间值放大 1000 倍 |
| 整体后移 2 秒 | 0 | 1 | 2 | 所有时间值 +2 |
| 反向播放（时间倒流） | 0 | -1 | `t_first + t_last` | 首尾时间值互换 |

## 5. 注意事项

1. **只改时间轴**：输出对象与输入对象只读共享点、单元与属性，输入的时间值不会被改动；
   需要原始序列时，原模型一直都在。
2. **输出是独立节点**：模型树里新增的是独立对象，不会替换或覆盖输入模型。
3. **帧数与帧数据不变**：本过滤器不增删帧、不重排帧、也不读帧文件内容，因此执行很快、不占额外内存；
   它**改变不了动画的播放速度**，播放速度由动画控件按时间值插值/推进，本过滤器改的是每帧对应的时间值。
4. **`Scale` 不能为 0**：会把所有帧压到同一个时间值，过滤器直接返回失败并提示。
5. **未实现周期性**：本框架没有"连续时间请求"的路径（帧按索引寻址），因此没有实现周期性时间
   （Periodic / Period）参数，时间轴一律按有限序列处理。
6. **输入必须带时间序列**：对普通单帧网格执行会失败；GUI 会提示"当前模型没有时间序列"，
   命令行用例会打印 `TemporalShiftScale ERROR!`。

## 6. 相关文件

| 内容 | 路径 |
| --- | --- |
| 过滤器实现 | `iGameCore/Filters/TimeSeries/iGameTemporalShiftScaleFilter.h` / `.cpp` |
| 菜单入口 / 参数弹窗 | `Qt/src/IQCore/igQtMainWindow.cpp`（算法处理 → 时间平移/缩放） |
| 测试用例 | `Examples/Filter/TimeSeries/TestTemporalShiftScale.cpp` |
| 测试模型 | `Examples/Models/sequence.pvd`（21 帧，帧数据在 `Examples/Models/sequence_frames/`） |
