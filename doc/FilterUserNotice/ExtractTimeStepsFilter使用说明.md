# ExtractTimeStepsFilter 保留指定时间步 使用说明

对应任务：`extract_time_steps` —— 保留指定时间步（依赖时间序列管线）。

## 1. 功能

`ExtractTimeStepsFilter` 从带时间序列的对象中**保留指定的时间步**，其余帧被裁掉。
执行后产出一个**独立输出对象**：只替换时间轴，几何、属性、每帧的元数据（文件路径列表）、
帧类型与缓存状态都按原样保留，**时间值不重编号**。

| 参数 | 接口 | 说明 |
| --- | --- | --- |
| 选择模式 | `SetSelectionMode(SelectionMode)` | `SELECT_TIME_STEPS`（按索引，默认）/ `SELECT_TIME_RANGE`（按区间 + 步长） |
| 索引列表 | `SetTimeStepIndices(const std::vector<int>&)` | 帧索引（0 起）；空列表 = 全部保留 |
| 索引区间 | `SetTimeStepRange(int begin, int end)` | 闭区间 `[begin, end]`；`end < 0` 表示取到最后一帧 |
| 步长 | `SetTimeStepInterval(int)` | 区间模式下每 n 帧取一个；`<= 0` 视为 1 |

执行后可查询结果（界面回显、用例断言都用这几个接口）：

| 接口 | 说明 |
| --- | --- |
| `GetNumberOfKeptTimeSteps()` | 本次保留的帧数 |
| `GetKeptTimeStepIndices()` | 本次保留的帧索引（升序） |
| `GetKeptTimeValues()` | 本次保留帧的时间值 |

处理规则：

- 越界索引（含负索引）直接丢弃；
- 索引**去重并按升序**排列，输入顺序不影响输出；
- 一个有效时间步都没剩下时（空列表、或索引全部越界），**保留全部帧**。

## 2. 输入要求

- 输入必须**带时间序列**，即 `DataObject::GetTimeFrames()` 非空；否则 `Execute()` 返回 `false`。
- 常见来源：`.pvd`（PVDReader 读出的序列）、`.igcm` 时间序列等。
- 两类形态都支持：容器（`.pvd`/`.igcm` 读出的根对象，几何在子对象里）与"网格 + 时间轴"；
  输出与输入保持**同类型**。
- 普通单帧网格没有时间步，本过滤器对它无事可做（会直接返回失败）。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开一个带时间序列的模型（例如 `Examples/Models/sequence.pvd`）；
2. 主菜单 **算法处理 → 保留指定时间步**，左侧出现面板；
3. 选择要保留的时间步：
   - **按索引选择**：点「+」添加一行，填写帧索引，右侧「时间值」列会自动显示该帧对应的时间值；
     `−` 删除选中行，`清空` 清掉全部行（清空 = 全部保留）；
   - **按区间+步长**：填写 `起始帧` / `结束帧` / `步长`；
4. 点 **执行 (Apply)**，结果作为**独立节点**加入模型树，面板状态栏显示实际保留的帧索引。

### 3.2 代码方式

```cpp
#include <TimeSeries/iGameExtractTimeStepsFilter.h>

// 按索引：保留第 0、5、10、15、20 帧
auto filter = iGame::ExtractTimeStepsFilter::New();
filter->SetTimeStepIndices({0, 5, 10, 15, 20});
filter->SetInput(object);                 // 带时间序列的对象
if (filter->Execute()) {
    auto output = filter->GetOutput();    // 独立输出对象（只换了时间轴）
    int kept = filter->GetNumberOfKeptTimeSteps();   // 5
} else {
    std::cout << "ExtractTimeSteps ERROR!" << std::endl;   // 输入没有时间序列
}

// 按区间 + 步长：保留第 1 ~ 20 帧里每 5 帧取一个
auto rangeFilter = iGame::ExtractTimeStepsFilter::New();
rangeFilter->SetSelectionMode(iGame::ExtractTimeStepsFilter::SELECT_TIME_RANGE);
rangeFilter->SetTimeStepRange(1, 20);
rangeFilter->SetTimeStepInterval(5);
rangeFilter->SetInput(object);
rangeFilter->Execute();
```

### 3.3 命令行测试

```bash
cd Examples
./testExtractTimeSteps          # 或 ctest -R testExtractTimeSteps
```

用例读取 `Examples/Models/sequence.pvd`（21 帧），保留第 0/5/10/15/20 帧并打印结果，
全部符合预期时输出 `PASS` 且退出码为 0。

## 4. 使用示例

输入模型 `Examples/Models/sequence.pvd`：21 帧，帧索引 0 ~ 20，时间值 0 ~ 10（步长 0.5，单位由数据作者定义）。

| 帧索引 | 0 | 1 | 2 | 3 | … | 20 |
| --- | --- | --- | --- | --- | --- | --- |
| 时间值 | 0 | 0.5 | 1 | 1.5 | … | 10 |

保留索引 `{0, 5, 10, 15, 20}` 后的输出：

```text
Kept time steps: 5
Kept indices : 0 5 10 15 20
Time values  : 0 2.5 5 7.5 10
Result: PASS
```

可以看到：帧数由 21 变为 5，而**时间值仍是原始的 0 / 2.5 / 5 / 7.5 / 10**，没有被重新编号成 0~4。

## 5. 注意事项

1. **只改时间轴**：输出对象与输入对象只读共享点、单元与属性，输入的时间步数量与内容都不变；
   需要原始序列时，原模型一直都在。
2. **输出是独立节点**：模型树里新增的是独立对象，不会替换或覆盖输入模型。
3. **空选择 = 全部保留**：不填索引、或填的索引全部越界时，结果仍包含全部帧（状态栏会显示实际保留数量，便于确认）。
4. **时间值不重编号**：本过滤器不改变时间值。若需要把时间轴平移到从 0 开始，需要用时间轴平移/缩放类处理，而不是本过滤器。
5. **执行时不读取帧数据**：过滤器只操作时间轴上的帧记录，裁剪后切换时间步时才按需读取帧文件，因此执行很快、也不占用额外内存。
6. **输入必须带时间序列**：对普通单帧网格执行会失败；GUI 面板会提示"当前模型没有时间序列"，命令行用例会打印 `ExtractTimeSteps ERROR!`。

## 6. 相关文件

| 内容 | 路径 |
| --- | --- |
| 过滤器实现 | `iGameCore/Filters/TimeSeries/iGameExtractTimeStepsFilter.h` / `.cpp` |
| 界面面板 | `Qt/include/IQWidgets/igQtExtractTimeStepsWidget.h` / `Qt/src/IQWidgets/igQtExtractTimeStepsWidget.cpp` |
| 菜单入口 | `Qt/src/IQCore/igQtMainWindow.cpp`（算法处理 → 保留指定时间步） |
| 测试用例 | `Examples/Filter/TimeSeries/TestExtractTimeSteps.cpp` |
| 测试模型 | `Examples/Models/sequence.pvd`（21 帧，帧数据在 `Examples/Models/sequence_frames/`） |
