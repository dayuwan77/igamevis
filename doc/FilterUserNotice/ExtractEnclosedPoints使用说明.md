# ExtractEnclosedPointsFilter 使用说明

## 1. 功能概述

`ExtractEnclosedPointsFilter` 用于判定一组点是否位于**封闭表面**内部，输出位于内部的点（可选输出外部点）。

核心算法为**射线奇偶法**：

1. 对每个待判定点 `P`，从 `P` 出发沿随机方向发出一条射线；
2. 统计射线与表面三角形面片的**前方交点**个数；
3. 交点数为**奇数** → `P` 在内部；为**偶数** → `P` 在外部；
4. 使用 VTK 的多方向投票规则：内部票 `+1`、外部票 `-1`，票差达到 2 时结束，最多 9 次射线测试。

执行时先将表面三角化并构建三角形 BVH。包围盒外的点直接判为外部；其余点的射线
仅对经过 BVH 包围盒筛选后的候选三角形求交，避免逐点遍历全部面片。
BVH 按三角形重心最长轴中位数划分，每个叶节点最多包含 8 个三角形。
随机方向按 VTK 默认 `vtkRandomPool` 的最小标准序列生成，每个点从自身编号开始读取，
不受前面点的射线次数影响。线段求交允许起点交点，交点按参数 `t` 排序并按相对容差合并。

兼容性目标为 ParaView 6.2.0 使用的 VTK 提交 `a970ec452229b1a0953684c00ecd9fe612778b8a`。
比较时必须使用相同坐标精度和相同表面；iGameVis 当前坐标存储为 float，不能仅凭
原始 double 模型的总点数判断是否对齐。当前输出为框架原生 PointSet，尚不支持 VTK
的单个 poly-vertex 单元，因此未提供 GenerateVertices，点数和点属性不受此限制影响。

## 2. 输入与输出

### 2.1 双输入

| 输入 | 类型 | 说明 |
|---|---|---|
| Input | `PointSet` | 待判定的点集 |
| Surface | `SurfaceMesh` | 封闭流形表面 |

- 两个输入**可以相同**，也可以不同。
- `Surface` 必须是**封闭流形**：每条边恰好被两个面共享。否则结果不可靠。

### 2.2 输出

- 类型：`PointSet`（纯点云，无面片）
- 内容：位于内部的点子集，以及其携带的点属性（标量 / 向量 / 法向等）

## 3. 参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| `CheckSurface` | `true` | 是否检查表面封闭性。开启且表面不封闭时，执行失败并清空旧输出 |
| `GenerateOutliers` | `false` | 同时在第二输出生成未选中的点；GUI 添加独立的 `_outliers` 模型 |
| `InsideOut` | `false` | 保留的 C++ 兼容接口，反选主输出；GUI 使用 GenerateOutliers |
| `Tolerance` | `0.0001` | 相对于表面包围盒对角线的求交容差，同时用于线段参数交点合并 |

## 4. 调用方式

### 4.1 GUI 方式

1. 打开/导入一个**封闭表面网格**（球面、立方体表面等）和一个**点集**；
2. 在模型树中选中**封闭表面模型**，使其成为当前模型；
3. 主菜单 **算法处理 → 封闭点提取 (Extract Enclosed Points)**，点击 **执行**；
4. **Input（待判定点集）**：下拉框中选择已加载的点集，或选择“**手工构造随机点云**”；
5. **Surface（封闭表面）**：下拉框中选择表面模型；
6. 视图区显示过滤结果，模型树中生成 `xxx_enclosed` 结果节点。

### 4.2 代码方式

```cpp
#include "ExtractEnclosedPoints/iGameExtractEnclosedPointsFilter.h"

// 假设已有 PointSet::Pointer cloud 和 SurfaceMesh::Pointer surface
auto filter = ExtractEnclosedPointsFilter::New();
filter->SetInput(0, cloud);      // Input
filter->SetInput(1, surface);    // Surface
filter->SetCheckSurface(true);
filter->SetInsideOut(false);
filter->SetTolerance(0.0001);
filter->SetGenerateOutliers(true);

if (!filter->Execute()) {
    // 处理失败
    return 1;
}

auto output = filter->GetOutput();           // DataObject::Pointer
auto outPS  = DynamicCast<PointSet>(output); // 转成 PointSet
auto outside = filter->GetOutput(1);        // 未选中的点
// outPS->GetNumberOfPoints() 即为保留的点数
```

### 4.3 命令行测试

```bash
cd cmake-build-examples
.\Release\testExtractEnclosedPoints.exe
```

旧示例读取相对路径模型（`Examples/Models/` 下两个文件），覆盖以下场景；历史点数仅供参考，
不作为新版边界判定的验收标准：

1. 点云 vs 球面（与 ParaView 对齐），2000 点 → 68 点；
2. InsideOut 反转，68 + 1932 = 2000；
3. 确定性，多次运行结果一致；
4. 容差敏感性。

### 4.4 手工构造随机点云

当场景里只有一个模型（只有表面）时，Input 可选择“**手工构造随机点云**”。

| 参数 | 默认值 | 说明 |
|---|---|---|
| 采样点数 | 2000 | 在包围盒内随机采样的点数 |
| 包围盒扩展倍数 | 1.5 | 采样范围 = Surface 包围盒 × 该倍数 |
| 加入场景以对比 | true | 是否把随机点云也加入模型树，便于视觉对比 |
| 随机种子 | 42 | 固定种子保证结果可复现 |

手工构造的随机点云会作为独立节点加入模型树，与过滤结果并排显示，方便对比“处理前 vs 处理后”。

## 5. 使用示例

### 示例 1：点云位于球面内部（与 ParaView 对照）

- **Input**：`Examples/Models/ExtractEnclosedPoints_Points_Cloud1.vtk`（2000 个点）
- **Surface**：`Examples/Models/ExtractEnclosedPoints_Surface_Sphere.vtk`（50 点 / 96 面的球面）

执行后：

| 指标 | 值 |
|---|---|
| 输入点数 | 2000 |
| 输出点数 | 68 |
| ParaView 输出 | 68 |

此表为历史球面示例记录。新版应使用相同精度的点和表面，通过原始点 ID 验证。

### 示例 2：同时输出外部点

同上输入，勾选 `Generate Outliers`，同时生成内部点和外部点两个模型。
旧 C++ 接口仍可通过 `SetInsideOut(true)` 反选主输出。下表为历史示例点数：

| 参数 | 输出点数 |
|---|---|
| `InsideOut = false` | 68 |
| `InsideOut = true` | 1932 |
| 和 | 2000 = 输入点数 ✓ |

### 示例 3：手工构造随机点云

- **Surface**：球面
- **Input**：手工构造随机点云，采样 2000 点，扩展 1.5 倍

输出点数取决于球面与采样范围的体积比，采样范围扩 1.5 倍时，球内比例约为：

```
(4/3 · π · 1³) / (2·1.5)³ ≈ 0.155
```

即约 300 点保留。**此例用于快速验证**，不要求与 ParaView 严格对照。

## 6. 注意事项

1. **Surface 必须封闭流形**：每条边必须恰好被两个面共享。若开启 `CheckSurface` 且表面不封闭，执行失败并清空旧输出。若关闭 `CheckSurface`，结果**不可靠**。

2. **点云文件的渲染问题**：加载 `.vtk` 点云文件时，iGameVis 目前会将其解析为 `SurfaceMesh`（面数为 0）。由于 `SurfaceMesh` 的渲染管线**默认只画三角形**，面数为 0 的 `SurfaceMesh` 在视口中不可见。**临时方案**：使用“手工构造随机点云”选项。

3. **表面上的点属于边界情况**：使用与 VTK 相同的射线奇偶规则，不额外规定表面点全部为内部。同一输入可复现；坐标精度、面片划分或版本变化可能影响边界分类。

4. **不自动转换输入类型**：如果输入是体网格或其他非表面网格，应先用 `ConvertToSurfaceMeshFilter`（`算法处理 → 数据处理 → 表面提取`）得到表面网格。

## 7. 相关文件

- **源码**：`iGameCore/Filters/ExtractEnclosedPoints/`
- **测试程序**：`Examples/Filter/TestExtractEnclosedPoints.cpp`
- **参考实现**：ParaView `Extract Enclosed Points`

## 8. 最新版对照回归

临时无窗口程序验证了立方体内外分类、边界点、双输出、失败后清空旧输出，并导出
精确 float 几何及原始点 ID，与官方 VTK 逐点比较。按用户要求，验证完成后已清理
新增的测试源码、Python 脚本和临时构建入口。

Python 验证依赖仅用于测试，不加入 iGameCore。用户模型 `analysis_vtu_subcase1.vtu`
包含 58764 点，提取表面为 57471 点、115070 个三角面。相同 float 几何下，VTK 9.7.1
与 iGameVis 均输出内部 29418 点、外部 29346 点，两个输出的原始点 ID 完全一致。
原始 double 模型的 VTK 9.7.1 参考为内部 29415 点；这与 float 数据是不同的数值输入。
核对过的 ParaView 6.2.0 所绑定 vtkTriangle 和 vtkStaticCellLocator 源码与 VTK 9.7.1 一致。
