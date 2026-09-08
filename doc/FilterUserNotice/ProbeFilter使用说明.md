# ProbeFilter 使用说明

> 过滤器名称：`ProbeFilter`（探测 / probe）
> 头文件：`iGameCore/Filters/Probe/iGameProbeFilter.h`
> 命名空间：`iGame`

## 1. 功能

在指定位置探测数据（点定位 + 插值）：以 `center` 为球心、`radius` 为半径的球体内均匀随机采样生成 `n` 个查询点；对每个查询点在模型网格中做单元定位（find cell），命中后在单元内按插值权重（重心坐标 / 双线性 / 三线性）对模型**点属性**做线性插值；结果写回查询点，输出为查询点集（`PointSet`）。

数据流：

- 输入 0：模型网格（面网格 / 体网格），提供单元与点属性
- 输入 1：查询点点集（`PointSet`），可用随机生成方法或由用户自行设置
- 输出 0：与输入 1 为**同一个对象**（原地更新，不新建输出点集）

输出点集内容：

- 点坐标 = 查询点坐标
- 点属性 = 模型各点属性的插值结果（模型无点属性时只输出坐标与 `ValidPointMask`）
- 新增点属性 `ValidPointMask`：找到单元 = 1，未找到 = 0；未找到时插值属性填 0
- 查询点集自带属性保留并叠加，同名属性原地覆盖为插值结果
- 重复执行不会新建输出点集，只在该点集上更新属性

支持的查询单元类型（凸网格）：三角形、四边形（面单元）；四面体、六面体（体单元）；其余类型直接返回未命中。

## 2. 调用方式

### 2.1 代码方式

```cpp
#include <Probe/iGameProbeFilter.h>

using namespace iGame;

// 1. 准备查询点集（必须非空，否则 Execute 失败）
auto queryPts = PointSet::New();
queryPts->SetName("ProbeQueryPoints");

//    方式 A：球体均匀随机采样（静态工具函数）
Point center(0.0f, 0.0f, 0.0f);   // 球心，按需设置
float radius = 1.0f;              // 球半径
int n = 100;                      // 查询点数
ProbeFilter::GenerateSpherePoints(queryPts, center, radius, n);  // seed 默认 0（随机）

//    方式 B：查询点由自己定义，逐点添加任意坐标（不要求球内采样）
//    queryPts->GetPoints()->AddPoint(x, y, z);

// 2. 创建过滤器并设置输入
auto filter = ProbeFilter::New();
filter->SetInput(model);         // 输入 0：模型网格
filter->SetInput(1, queryPts);   // 输入 1：查询点点集（Execute 后原地带上插值属性）
filter->SetTolerance(1e-6);      // 可选；默认自动使用 kProbeDefaultTolerance（1e-6）

// 3. 执行（返回 false 表示失败，如查询点集为空）
if (!filter->Execute()) {
    // 失败处理
    return;
}

// 4. 取输出：与 queryPts 为同一对象，插值属性 + ValidPointMask 已原地写回
auto result = filter->GetOutput();
```

球体随机采样工具函数：

```cpp
static void ProbeFilter::GenerateSpherePoints(PointSet::Pointer points,
                                              const Point& center,
                                              float radius, int count,
                                              unsigned seed = 0);
```

- `radius == 0`：n 个点全部生成在球心位置（退化球）
- `radius < 0`：不生成任何点
- `seed == 0`：随机种子；`seed > 0`：固定种子（便于复现）

### 2.2 界面方式

Qt 主窗口「算法处理」菜单 →「探测 (probe)」：在右侧 Dock 面板填写 Center X/Y/Z、Radius、点数 N（默认 1）、容差（留空自动），点击「探测」后原地生成球体内随机查询点并执行 `ProbeFilter`，状态栏显示「探测完成：有效 X / 共 Y 个点」，「查询结果」列表逐点展示坐标与各插值属性。打开面板或切换模型时按模型包围盒自动重置参数（Center = 包围盒中心，Radius = 对角线 × 0.1）。

## 3. 使用示例

测试程序 `testProbe`：

- 注册：`Examples/CMakeLists.txt` 中 `igame_add_example(testProbe Filter/Probe/TestProbe.cpp)`
- 源文件：`Examples/Filter/Probe/TestProbe.cpp`
- 运行：**直接运行，无需命令行输入**，模型与探测参数已全部写死在代码中

```text
testProbe.exe
```

示例写死的参数：

| 参数 | 值 |
| ---- | -- |
| 模型 | `./Models/AIGen_Hex_PipeSegment.vtk`（六面体体网格：108 点 / 48 六面体，带 `VonMisesStress`、`Displacement`、`Temperature` 点属性） |
| 球心 | `(1.385819, 0.574025, 0.75)`（管道壁内一个六面体单元中心附近） |
| 半径 | `0.1` |
| 查询点数 | `10` |
| 随机种子 | `42`（固定，输出可复现） |

示例流程与输出：

1. 读取模型并打印模型规模（点数 / 单元数）、包围盒中心与对角线、点属性清单
2. 在球心 `(1.385819, 0.574025, 0.75)`、半径 `0.1` 的球体内均匀随机生成 10 个查询点
3. 执行 `ProbeFilter`，逐点打印坐标、`ValidPointMask` 与各插值属性
4. 打印命中统计与最终结果

```text
Probe center=(1.385819, 0.574025, 0.75) radius=0.1 n=10 seed=42
Model: ./Models/AIGen_Hex_PipeSegment.vtk
Model points=108 cells=48
...
Valid points: 10 / 10
Result: PASS
```

球心选在管道壁内六面体单元正中（r = 1.5、角向 22.5°、z = 0.75，避开所有单元边界），半径 0.1 的球完全落在该单元内部，**预期 10/10 全部命中**。

## 4. 注意事项

1. **查询点集必须非空**：空点集会直接导致 `Execute()` 失败。
2. **输出0与输入 1 是同一对象**：`GetOutput()` 返回的就是传入的查询点集，插值属性与 `ValidPointMask` 原地写回；重复执行不会新建输出点集。
3. **只对模型点属性插值**：模型单元属性不参与插值；模型无点属性时查询点只带坐标与 `ValidPointMask`。
4. **未命中处理**：未找到单元的点 `ValidPointMask = 0`，插值属性填 0。
5. **容差为相对值**：默认 `kProbeDefaultTolerance = 1e-6`；面单元投影距离上限 = 容差 × 模型包围盒对角线。可用 `SetTolerance` 设置、`SetAutoTolerance()` 恢复自动。
6. **仅支持部分凸网格**：目前支持三角形、四边形、四面体、六面体；其余单元类型直接返回未命中。凹网格则会导致查询结果不准确。
7. **球心要落在模型有效范围内**：若球心在模型外部 / 中空区域或贴着单元边界，命中数会减少甚至为 0。本示例球心 `(1.385819, 0.574025, 0.75)` 位于管道壁内六面体单元正中，保证全部命中。
8. **相对路径**：`./Models/AIGen_Hex_PipeSegment.vtk` 是相对路径，需在 `Examples` 目录下运行程序。
9. **失败提示**：模型读取失败、查询点构造失败或 `Execute()` 失败都会打印 `Result: FAIL` 并以非 0 退出码结束。
