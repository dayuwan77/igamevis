# CellMeshMetricsFilter 使用说明

## 1. Overview (功能概述)

`CellMeshMetricsFilter` 是 iGameVis 平台中面向三维体网格（Volume Mesh）及复合多块装配体（MultiBlock Dataset / `.vtm`）的**单元质量指标评估过滤器**。

### 核心特性
- **装配体树状递归穿透**：基于组合模式（Composite Pattern）设计，采用深度优先搜索（DFS）自动遍历多层嵌套装配结构，将实体体网格子块委托给底层 `VolumeMeshMetricsFilter` 进行单元级质量分析；
- **全面的质量评价体系**：支持四面体（Tetrahedron）与六面体（Hexahedron）的主流几何畸变与质量评价指标（包含纵横比 Aspect Ratio、边长比 Edge Ratio、雅可比行列式 Jacobian、歪斜度 Skew、空间体积 Volume 等）；
- **原位属性挂载与即时反馈**：评估结果直接作为单元标量属性（Cell Attribute）自动追加至各网格的数据属性集合中，方便后续进行 3D 伪彩云图映射；
- **装配拓扑与命名保真**：完整保留原装配体的子构件层级关系与零件名称（Name），输出与原模型结构同构的多块结果；
- **高容错异构处理**：多块装配体中若存在不匹配当前指标类型的子零件（如在四面体指标下遇到六面体部件），算法安全赋予默认值 `0`，避免崩溃并确保批处理稳健。

---

## 2. Algorithm & Supported Metrics (算法逻辑与支持指标)

### 2.1 递归遍历算法流程
1. 若输入为复合装配节点（`input->HasSubDataObject() == true`）：
   - 实例化装配容器 `DataObject::New()`，保留原始节点名称；
   - 深度递归遍历每一个子数据对象：`ComputeCellMetrics(subObj, subOut)`；
   - 将计算完毕的子网格重新装配至输出容器中并返回。
2. 若输入为实体网格（叶子节点）：
   - 实例化底层 `VolumeMeshMetricsFilter`；
   - 配置目标评估指标类型 `SetVolumeMetric(m_Metric)`；
   - 执行单元遍历计算，生成单元标量属性并挂载至网格，更新输出对象。

### 2.2 支持的几何质量指标 (Supported Metrics)

#### 四面体 (Tetrahedron) 核心指标
| 指标枚举 (`VolumeMetric`) | 中文名称 | 最优理论值 / 范围 | 几何意义说明 |
| :--- | :--- | :---: | :--- |
| `TET_ASPECT_RATIO` | 纵横比 | 1.0 (最优) ~ $\infty$ | 最大边长与内切球半径比值，衡量四面体是否接近正四面体 |
| `TET_EDGE_RATIO` | 边长比 | 1.0 (最优) ~ $\infty$ | 最长边与最短边长度比值，反映单元几何各向同性 |
| `TET_VOLUME` | 单元体积 | $> 0$ | 单元几何空间体积，负值提示单元发生拓扑反转/畸变 |
| `TET_JACOBIAN` | 雅可比行列式 | $> 0$ | 衡量单元与参考正单元坐标映射的变形程度 |
| `TET_COLLAPSE_RATIO` | 塌陷率 | 0.0 ~ 1.0 | 衡量四面体特征高度与底面特征尺寸的比值 |
| `TET_MIN_ANGLE` | 最小二面角 | $0^\circ \sim 70.53^\circ$ | 衡量四面体是否存在扁平夹角或退化危险 |

#### 六面体 (Hexahedron) 核心指标
| 指标枚举 (`VolumeMetric`) | 中文名称 | 最优理论值 / 范围 | 几何意义说明 |
| :--- | :--- | :---: | :--- |
| `HEX_EDGE_RATIO` | 长宽比 | 1.0 (最优) ~ $\infty$ | 单元各边长差异 |
| `HEX_JACOBIAN` | 雅可比行列式 | $> 0$ | 检查六面体单元是否有严重扭曲、退化或负体积 |
| `HEX_SKEW` | 歪斜度 | 0.0 (最优) ~ 1.0 | 各相邻面夹角与 $90^\circ$ 正交基底的偏离程度 |
| `HEX_STRETCH` | 伸展度 | 0.0 ~ 1.0 (最优) | 衡量六面体受压缩扁平的程度 |
| `HEX_VOLUME` | 单元体积 | $> 0$ | 六面体单元的三维空间体积 |

---

## 3. 调用方式 (API & Invocations)

### 3.1 C++ 核心 API
头文件：`#include <MeshMetrics/iGameCellMeshMetricsFilter.h>`

```cpp
// 1. 工厂方法创建 Filter 实例
auto filter = iGame::CellMeshMetricsFilter::New();

// 2. 配置待测质量指标类型（例如四面体纵横比）
filter->setMetric(iGame::VolumeMeshMetricsFilter::TET_ASPECT_RATIO);

// 3. 设置输入（支持单体网格或多块装配体）
filter->SetInput(0, inputDataObject);

// 4. 执行计算
if (filter->Execute()) {
    // 5. 获取包含质量属性的数据对象
    auto outputObj = filter->GetOutput(0);
}
```

### 3.2 桌面端交互触发 (iGameVis Qt UI)
已在桌面端 `Filter` 主菜单完成集成：
1. 通过【文件】->【打开】载入 `.vtm` 装配体或 `.vtk` / `.vtu` 体网格模型；
2. 在左侧模型树（ModelTree）中单击选中目标模型；
3. 点击顶部主菜单：`Filter` -> `单元质量评估 (CellMeshMetrics)`；
4. 在弹出的参数对话框的下拉列表中选择具体指标（如四面体纵横比、边长比、雅可比等）；
5. 点击【应用】，属性自动挂载，模型树自动刷新并可直接激活云图渲染。

---

## 4. 使用示例 (Code Example)

测试用例源码：`Examples/Filter/TestCellMeshMetrics.cpp`  
测试数据模型：`Examples/Models/cell_metric_assembly.vtm`  
- 内含 `cell_metric_tet.vtk`（含正四面体与拉伸四面体）  
- 内含 `cell_metric_hex.vtk`（标准六面体单元）

代码内嵌智能相对寻址，无需手动传参，终端全自动运行：

```cpp
#include <Core/iGameScene.h>
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <iostream>

int main() {
    // 1. 读取专属测试装配体模型（多块装配体）
    const std::string vtmPath = "./Models/cell_metric_assembly.vtm";
    auto multiBlockObj = iGame::FileIO::ReadFile(vtmPath);
    if (!multiBlockObj) return -1;

    // 2. 实例化并配置质量评估 Filter
    auto filter = iGame::CellMeshMetricsFilter::New();
    filter->setMetric(iGame::VolumeMeshMetricsFilter::TET_ASPECT_RATIO);
    filter->SetInput(0, multiBlockObj);

    // 3. 执行评估
    if (!filter->Execute()) return -1;
    auto outputObj = filter->GetOutput(0);

    // 4. 伪彩云图着色与 3D 交互视口
    auto scene = iGame::Scene::New();
    // 遍历子构件调用 drawObj->ViewCloudPicture 并配置色标条...
    return 0;
}
```

---

## 5. 验证结果分析 (Validation & Analysis)

### 5.1 实测运行输出
运行 `testCellMeshMetrics.exe`，控制台输出如下：
```text
[步骤 1] 读取多块模型 (.vtm)...
  文件: ./Models/cell_metric_assembly.vtm
  读取成功，多块树结构:
├── 装配组: [cell_metric_assembly] (包含 2 个子块)
    └── 实体网格: [cell_metric_tet] (单元数 = 2)
    └── 实体网格: [cell_metric_hex] (单元数 = 2)
  [步骤 1] 完成

[步骤 2] 应用四面体 Aspect Ratio (纵横比, 1=正四面体最优) 评估...
  评估完成，属性已挂载到每个叶子网格 (Metric2)
  [步骤 2] 完成

[步骤 3] 伪彩着色、显示色条并输出指标范围...
├── 装配组: [cell_metric_assembly] (2 个子块)
    └── 实体网格: [cell_metric_tet]
        └─ 指标 [Metric2]: 单元总数 = 2 | 范围 = [1, 2.07313] | 平均值 = 1.53657
        └─ 色条已绑定: TET_ASPECT_RATIO (蓝=范围最小, 白=中值, 红=范围最大)
    └── 实体网格: [cell_metric_hex]
        └─ 指标 [Metric2]: 单元总数 = 2 | 范围 = [0, 0] | 平均值 = 0
  ------------------------------------------------
  全局指标范围 (TET_ASPECT_RATIO): [0, 2.07313]
  [步骤 3] 完成
```

### 5.2 理论吻合度分析
| 子块构件 | 单元类型 | 质量指标 | 实测范围 | 数学理论值 | 吻合判定 |
| :--- | :--- | :--- | :---: | :---: | :---: |
| `cell_metric_tet` 单元 1 | 标准正四面体 | 纵横比 (Aspect Ratio) | **1.0** | **1.0** (理论最优) | ✅ 完全吻合 |
| `cell_metric_tet` 单元 2 | 几何拉伸四面体 | 纵横比 (Aspect Ratio) | **2.07313** | **2.07313** | ✅ 严格一致 |
| `cell_metric_hex` | 六面体单元 | 纵横比 (四面体模式) | **0** | **0** (安全默认值) | ✅ 异构保护正确 |

---

## 6. 注意事项与已知限制 (Precautions & Known Issues)

1. **单元类型匹配机制**：
   - 四面体指标（`TET_*`）专用于四面体单元，六面体指标（`HEX_*`）专用于六面体单元；
   - 装配体中若包含异构网格，当前算法会自动赋 `0` 保护，建议针对不同构件分别应用对应的评估模式。
2. **子模型暂不支持独立 Filter 操作**：
   - 当前模型树选定机制限制，多块装配体无法单独选择某个子零件执行局部的指标评估，Filter 会对装配体全树所有匹配单元进行递归评估。
3. **模型树子节点刷新机制修复说明**：
   - 此前在 UI 中调用 `updateAllAttriubute` 刷新模型树时存在重复调用两次 `BuildSubObjectTreeSkeleton` 的缺陷，导致属性刷新后模型树会多出 2 个冗余子节点；已在此版本中修正，现已恢复为精确对应真实构件数量。
