# CellMeshMetricsFilter 使用说明

## 1. Overview (功能概述)

`CellMeshMetricsFilter` 是 iGameVis 平台中面向三维体网格及复合多块装配体（MultiBlock Dataset / `.vtm`）的**单元质量评估过滤器**。其交互与语义**对齐 ParaView 的 `Cell Quality` 过滤器（`vtkCellQuality`）**。

### 核心特性

- **统一指标交互（对齐 ParaView）**：用户只选择一个**统一的质量指标**（如「纵横比」「边长比」），该指标自动作用于模型中的**所有单元**，无需用户区分四面体/六面体专用算法；
- **按真实单元类型分发**：逐单元读取 `GetCellType()`，按**真实单元类型**（而非顶点数量）分发到对应的数学实现；
- **"不支持值"语义**：该指标对某类单元不适用时，填入**不支持值**（默认 `-1.0`，界面可配置），**绝不用 `0` 混淆**；
- **独立结果节点**：评估结果写入**新生成的网格对象**，**原始模型零改动**；
- **装配树递归穿透**：深度优先遍历多层级装配结构，逐个子零件评估并按原结构回装，保留层级与命名；
- **明确的跳过统计**：执行后报告「成功计算单元数」「跳过不适用单元数」「无法处理的网格块数」三类计数。

---

## 2. Algorithm & Supported Metrics (算法与支持指标)

### 2.1 处理流程

```
1. 复合装配节点（HasSubDataObject() == true）
   → 新建 DrawObject 容器，保留原节点名称
   → 递归处理每个子数据对象，按原结构回装
   → 处理失败的子块【原样保留】，不静默丢弃

2. 叶子实体网格
   → 若不是 UnstructuredMesh 或单元数为 0
        → m_SkippedBlockCount++，原样保留并记录日志
   → 否则：新建独立输出网格（共享几何/拓扑 + 深拷贝属性集）
        → 逐单元读取 GetCellType(cellId)
        → 按真实类型 + 当前指标分发计算
        → 适用 → 写入真实值，m_SupportedCount++
        → 不适用 → 写入 m_UnsupportedValue，m_UnsupportedCount++
   → 结果数组命名为 CellQuality，挂载到【输出网格】的 Cell Data
```

### 2.2 支持的统一指标（11 项）

| # | 指标（界面显示名） | 枚举 | 四面体 | 六面体 |
| :---: | :--- | :--- | :---: | :---: |
| 1 | 边长比 (Edge Ratio) | `QUALITY_EDGE_RATIO` | ✅ | ✅ |
| 2 | 单元体积 (Volume) | `QUALITY_VOLUME` | ✅ | ✅ |
| 3 | 纵横比 (Aspect Ratio) | `QUALITY_ASPECT_RATIO` | ✅ | ✗ |
| 4 | 雅可比行列式 (Jacobian) | `QUALITY_JACOBIAN` | ✅ | ✅ |
| 5 | 歪斜度 (Skew) | `QUALITY_SKEW` | ✗ | ✅ |
| 6 | 最小内角 (Minimum Angle) | `QUALITY_MIN_ANGLE` | ✅ | ✗ |
| 7 | 锥度 (Taper) | `QUALITY_TAPER` | ✗ | ✅ |
| 8 | 伸展度 (Stretch) | `QUALITY_STRETCH` | ✗ | ✅ |
| 9 | 对角线比值 (Diagonal) | `QUALITY_DIAGONAL` | ✗ | ✅ |
| 10 | 最大长宽比 (Max Edge Ratio) | `QUALITY_MAX_EDGE_RATIO` | ✗ | ✅ |
| 11 | 塌陷率 (Collapse Ratio) | `QUALITY_COLLAPSE_RATIO` | ✅ | ✗ |

> **✅ = 该指标对该单元类型适用，输出真实计算值**
> **✗ = 不适用，输出不支持值（默认 `-1.0`）并计入跳过统计**
>
> 上表的 ✗ 分布与 ParaView/VTK 实测行为**完全一致**（详见第 5 章）。

### 2.3 指标语义说明

| 指标 | 四面体 | 六面体 |
| :--- | :--- | :--- |
| 边长比 | 最长边 / 最短边，正四面体 = 1 | 12 条边中最长 / 最短，单位立方体 = 1 |
| 单元体积 | 四面体体积 | 六面体体积，单位立方体 = 1 |
| 纵横比 | `最长边 / (2√6 · 内切球半径)`，正四面体 = 1 | — |
| 雅可比行列式 | `6 × 体积`，正四面体 = `√2/2` | 8 个顶点处雅可比行列式的最小值，单位立方体 = 1 |
| 歪斜度 | — | 三组主方向单位向量点积的最大值，单位立方体 = 0（最优） |
| 最小内角 | **最小二面角**（相邻两面夹角），正四面体 = `acos(1/3) ≈ 70.5288°` | — |
| 锥度 | — | 单位立方体 = 0（最优） |
| 伸展度 | — | `√3 · 最短边 / 最长对角线`，单位立方体 = 1（最优） |
| 对角线比值 | — | 最短体对角线 / 最长体对角线，单位立方体 = 1 |
| 最大长宽比 | — | 三组主方向长度比的最大值，单位立方体 = 1 |
| 塌陷率 | 各顶点高度 / 对面最长边 的最小值，正四面体 = `√6/3 ≈ 0.8165` | — |

---

## 3. 调用方式 (API & Invocations)

### 3.1 C++ 核心 API

头文件：`#include <MeshMetrics/iGameCellMeshMetricsFilter.h>`

```cpp
// 1. 工厂方法创建 Filter 实例
auto filter = iGame::CellMeshMetricsFilter::New();

// 2. 配置统一评估指标
filter->setMetric(iGame::CellMeshMetricsFilter::QUALITY_ASPECT_RATIO);

// 3. 配置"不支持值"（可选，默认 -1.0）
filter->setUnsupportedValue(-1.0);

// 4. 设置输入（支持单体网格或多块装配体）
filter->SetInput(0, inputDataObject);

// 5. 执行计算
if (!filter->Execute()) { /* 处理失败 */ }

// 6. 获取【独立输出节点】（原始模型未被修改）
auto outputObj = filter->GetOutput(0);

// 7. 查询统计结果
int supported   = filter->GetSupportedCount();     // 成功计算的单元数
int unsupported = filter->GetUnsupportedCount();   // 跳过的不适用单元数
int skippedBlk  = filter->GetSkippedBlockCount();  // 无法处理的网格块数
```

### 3.2 结果数据

- **数组名**：`CellQuality`（与 ParaView 输出数组同名）
- **挂载位置**：**输出节点**的 Cell Data（`IG_SCALAR` + `IG_CELL`）
- **不支持值**：默认 `-1.0`，可通过 `setUnsupportedValue()` 修改

### 3.3 桌面端交互触发 (iGameVis Qt UI)

1. 通过【文件】→【打开】载入 `.vtm` 装配体或 `.vtk` 体网格模型；
2. 在左侧模型树中单击选中目标模型；
3. 点击顶部菜单：**`算法处理` → `单元质量评估 (Cell Quality)`**；
4. 弹出参数对话框：

   | 参数 | 说明 |
   | :--- | :--- |
   | **评估指标** | 下拉选择 11 个统一指标之一 |
   | **不支持/无效值** | 默认 `-1.0`，可自定义 |

5. 点击【应用】；
6. **结果**：
   - 模型树新增**独立节点** `<原模型名>_CellQuality`（原模型保持不变）；
   - 弹窗汇总报告：
     ```text
     单元质量评估完成。

     • 评估指标：纵横比 (Aspect Ratio)
     • 成功计算单元：N 个
     • 跳过不适用单元：M 个（已标记为 -1）
     • 无法处理的网格块：K 个
     • 结果已生成为独立节点：xxx_CellQuality
     ```

---

## 4. 使用示例 (Code Example)

测试用例源码：`Examples/Filter/TestCellMeshMetrics.cpp`
测试数据模型：`Examples/Models/cell_metric_assembly.vtm`
- `cell_metric_tet.vtk`：2 个四面体（1 个正四面体 + 1 个拉伸四面体）
- `cell_metric_hex.vtk`：2 个标准六面体

```cpp
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <iGameFileIO.h>

int main() {
    // 1. 读取测试装配体
    auto multiBlockObj = iGame::FileIO::ReadFile("./Models/cell_metric_assembly.vtm");
    if (!multiBlockObj) { return -1; }

    // 2. 配置统一指标与不支持值
    auto filter = iGame::CellMeshMetricsFilter::New();
    filter->setMetric(iGame::CellMeshMetricsFilter::QUALITY_ASPECT_RATIO);
    filter->setUnsupportedValue(-1.0);

    // 3. 执行
    filter->SetInput(0, multiBlockObj);
    if (!filter->Execute()) { return -1; }

    // 4. 取独立输出节点
    auto outputObj = filter->GetOutput(0);

    // 5. 统计报告
    //    成功计算单元 = filter->GetSupportedCount()
    //    跳过不适用单元 = filter->GetUnsupportedCount()
    //    无法处理的网格块 = filter->GetSkippedBlockCount()
    return 0;
}
```

### 自动化测试

测试用例会**循环执行全部 11 个指标**，并把结果与内嵌的 ParaView 标准答案逐值比对：

```powershell
cmake --build .\cmake-build-examples\ --target testCellMeshMetrics
cd cmake-build-examples
.\testCellMeshMetrics.exe
```

输出格式：

```text
  [PASS] 纵横比 (Aspect Ratio)   (ParaView: Aspect Ratio)
     cell_metric_tet  实际 = 1, 2.07313   期望 = 1, 2.07313   OK
     cell_metric_hex  实际 = -1, -1       期望 = -1, -1       OK
     统计: 成功计算 = 2 个, 跳过不适用 = 2 个

[步骤 3] 对比汇总
  ------------------------------------------------
  与 ParaView 一致: 11 个指标
  不一致/失败    : 0 个指标
  ------------------------------------------------
```

---

## 5. 对比验证 (Validation against ParaView)

### 5.1 标准答案来源

标准答案由 **ParaView 6.1.1 自带的 `vtkCellQuality`**（即界面 `Filters → Cell Quality` 的底层实现）在**同一份几何**上实跑得到：

1. 用 `vtkUnstructuredGridReader` 读取与 iGameVis 完全相同的 `.vtk` 文件；
2. 组装为 `vtkMultiBlockDataSet` 并写出 `cell_metric_assembly_pv.vtm`（供 ParaView 打开）；
3. 对每个指标调用 `vtkCellQuality::SetQualityMeasure()` 并读回 `CellQuality` 数组。

> ParaView 对比模型：`Examples/Models/cell_metric_assembly_pv.vtm`
> ⚠️ 该模型引用 `.vtu`，**仅供 ParaView 打开**；iGameVis 请使用 `cell_metric_assembly.vtm`。

### 5.2 逐值对比结果（11 指标全部一致）

| 指标 | `cell_metric_tet`（2 个四面体） | `cell_metric_hex`（2 个六面体） |
| :--- | :--- | :--- |
| 边长比 (Edge Ratio) | `1, 1.73205` | `1, 1` |
| 单元体积 (Volume) | `0.117851, 0.117851` | `1, 1` |
| 纵横比 (Aspect Ratio) | `1, 2.07313` | `-1, -1` |
| 雅可比行列式 (Jacobian) | `0.707107, 0.707107` | `1, 1` |
| 歪斜度 (Skew) | `-1, -1` | `0, 0` |
| 最小内角 (Minimum Angle) | `70.5288, 35.2644` | `-1, -1` |
| 锥度 (Taper) | `-1, -1` | `0, 0` |
| 伸展度 (Stretch) | `-1, -1` | `1, 1` |
| 对角线比值 (Diagonal) | `-1, -1` | `1, 1` |
| 最大长宽比 (Max Edge Ratio) | `-1, -1` | `1, 1` |
| 塌陷率 (Collapse Ratio) | `0.816496, 0.288675` | `-1, -1` |

### 5.3 理论值交叉验证

| 指标 | 输入 | 实测 | 理论值 |
| :--- | :--- | :---: | :---: |
| Volume / Jacobian | 单位立方体 | `1` | 1 |
| Skew / Taper | 单位立方体 | `0` | 0（最优） |
| Stretch / Diagonal / Max Edge Ratio | 单位立方体 | `1` | 1（最优） |
| Aspect Ratio / Edge Ratio | 正四面体 | `1` | 1（最优） |
| Minimum Angle | 正四面体 | `70.5288°` | `acos(1/3)` |
| Collapse Ratio | 正四面体 | `0.816496` | `√6/3` |

---

## 6. 注意事项与已知限制 (Precautions & Known Issues)

### 6.1 核心语义约定

1. **"不支持值"语义（对齐 VTK）**：
   - 默认 `-1.0`，对应 VTK `vtkCellQuality` 的 `UnsupportedGeometry` / `UndefinedQuality` 默认值；
   - **禁止用 `0` 表示不支持** —— 因为 `0` 在「歪斜度」「锥度」等指标中是**最优值**，会造成语义混淆；
   - 数据统计/着色时应**跳过**不支持值，避免污染色标范围。
2. **判断依据是「真实单元类型 + 指标适用性」**：
   - 使用 `GetCellType(cellId)` 返回的 `IGCellType` 判断，**不看顶点数量**；
   - 指标对某类型不适用时直接填入哨兵值，**不做数值范围猜测**。
3. **输出为独立结果节点**：
   - Filter 内部新建网格对象，**原始模型完全不被修改**；
   - 几何（`Points`）、拓扑（`Cells`）与原有属性数组采用**只读共享**方式，
     仅新增 `CellQuality` 数组 —— 这一行为与 ParaView `vtkCellQuality` 的
     `ShallowCopy` 语义完全一致（实测 ParaView 同样共享数组指针，不做深拷贝）；
   - Qt 界面把结果作为新节点挂载到模型树（`<原名>_CellQuality`）。

### 6.2 输入格式支持范围

> **证据等级说明**
> - **实测** —— 在本版本中**实际读取并成功执行过**单元质量评估；
> - **代码推断** —— 依据 IO 读取器的实现代码得出，**尚未实测**；
> - **未确定** —— 读取后的类型无法从代码路径明确判断。
>
> ⚠️ 下表**仅前 3 行为实测结论**，其余格式均为代码推断或未确定，实际行为请以运行时结果为准。

| 文件格式 | 读取后类型 | 证据等级 | 结论 |
| :--- | :--- | :---: | :--- |
| `.vtk`（Legacy 非结构网格） | `UnstructuredMesh` | **实测** | ✅ 支持 |
| `.vtu`（XML 非结构网格） | `UnstructuredMesh` | **实测** | ✅ 支持 |
| `.vtm`（多块装配体） | 容器（递归处理各子块） | **实测** | ✅ 支持 |
| `.ccm`（CFD） | `UnstructuredMesh`（经 `.vtu` 中转） | 代码推断 | 预计支持 |
| `.bdf`（Nastran） | `UnstructuredMesh`（经 `.vtu` 中转） | 代码推断 | 预计支持 |
| `.inp` | `UnstructuredMesh` | 代码推断 | 预计支持 |
| `.odb`（Abaqus） | `UnstructuredMesh` | 代码推断 | 预计支持 |
| `.cgns` | `VolumeMesh` 或 `UnstructuredMesh` | 代码推断 | ⚠️ 视网格类型而定 |
| `.igc` / `.igcm` | 取决于解码结果 | **未确定** | ⚠️ 未验证 |
| `.vts`（结构化网格） | `StructuredMesh` | 代码推断 | ❌ 不支持 |
| `.pvd`（时间序列集合） | `VolumeMesh` / `SurfaceMesh` | 代码推断 | ❌ 不支持 |
| `.vtp`（PolyData） | `SurfaceMesh` | 代码推断 | ❌ 非体网格，不适用 |
| `.cas` / `.rst` / `.rth` | **未确定** | **未确定** | ⚠️ 未验证 |

> **遇到不支持类型时的行为**（对所有格式一致）：
> 该网格块被**原样保留**（不丢弃），计入「无法处理的网格块」计数，
> 并在 `logs/iGame-core-log.txt` 中记录块名。**不会崩溃，也不会静默丢失数据。**
>
> **如何自行判定某格式是否被支持**：
> 1. 在界面中打开该模型并执行「单元质量评估」；
> 2. 查看弹窗报告 —— 若「无法处理的网格块」> 0，说明该格式当前不被支持；
> 3. 或查看日志中的 `[CellMeshMetrics] Block skipped: ...` 行。

### 6.3 已知限制

1. **金字塔（Pyramid）/ 三棱柱（Wedge）/ 多面体（Polyhedron）暂未实现**：
   - 这几类单元目前一律计入「跳过不适用单元」并填入不支持值（默认 `-1`）；
   - 各类单元的覆盖情况：

     | 单元类型 | 本 Filter 支持的指标数 | ParaView 是否支持该类型 | 结论 |
     | :--- | :---: | :--- | :--- |
     | 四面体 Tetra | **6**（边长比/体积/纵横比/雅可比/最小内角/塌陷率） | ✅ 支持 | 已实现的 6 个**逐值一致** |
     | 六面体 Hexa | **8**（边长比/体积/雅可比/歪斜度/锥度/伸展度/对角线/最大长宽比） | ✅ 支持 | 已实现的 8 个**逐值一致** |
     | **金字塔 Pyramid** | **0** | ✅ 支持（VTK 提供 5 个 measure） | ❌ **本 Filter 完全缺失** |
     | **三棱柱 Wedge** | **0** | ✅ 支持 | ❌ **本 Filter 完全缺失** |
     | 多面体 Polyhedron | **0** | ❌ VTK 同样无实现 | ✅ **行为一致**（均返回 `-1`） |

   - **各项结论的证据等级**：

     | 结论 | 证据等级 | 依据 |
     | :--- | :---: | :--- |
     | 本 Filter 支持 6 个（Tet）/ 8 个（Hex） | **实测** | 黄金对比测试输出确认 |
     | 已实现的 14 个组合与 ParaView 逐值一致 | **实测** | 第 5 章对比表，11 个指标全部 `[PASS]` |
     | 多面体在 ParaView 中同样返回 `-1` | **实测** | `TetPlane_polyhedron.vtu`（30700 个多面体）双方均为全 `-1` |
     | Pyramid 的 5 个 measure 清单 | 头文件 | `vtkMeshQuality.h` 的 `SetPyramidQualityMeasureTo*` |
     | Tetra 21 / Hexa 24 个 measure | 头文件 | `vtkMeshQuality.h` 的 per-type setter 清单 |
     | Wedge 的具体 measure 数 | **未完整获取** | 官方文档页在该处截断，故不列具体数字 |

   - **已实现的 14 个"单元类型 × 指标"组合，与 ParaView 逐值一致**（见第 5 章对比表）；
   - 金字塔与三棱柱的数学实现待后续补充。

2. **不支持值不使用 `0`**：
   - 所有 ✗ 的格子（如六面体的「纵横比」、四面体的「锥度」）均填**不支持值**（默认 `-1.0`），
     与 ParaView 的 `UnsupportedGeometry` / `UndefinedQuality` 默认值一致。

3. **子模型暂不支持独立 Filter 操作**：
   - 受模型树选定机制限制，多块装配体无法单独选择某个子零件执行局部评估，Filter 会对整棵装配树递归处理。

4. **`VolumeMesh` 输入限制**：
   - `VolumeMesh` 类不保存单元类型数组，无法按真实类型判断，故当前不处理；
   - 若模型被读取为 `VolumeMesh`（如部分 `.cgns` / `.pvd`），会记为「无法处理的网格块」。

### 6.4 运行时日志

关键事件写入项目标准日志：

```text
logs/iGame-core-log.txt
```

无法处理的网格块会记录：

```text
[iGameVis_Core][warning]: [CellMeshMetrics] Block skipped: not an UnstructuredMesh
                          or NumberOfCells == 0, name='xxx'
```
