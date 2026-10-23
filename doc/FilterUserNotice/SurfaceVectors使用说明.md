# SurfaceVectorsFilter（表面向量约束）使用说明

## 1. 功能说明

SurfaceVectorsFilter 对标 ParaView 的 **Surface Vectors** Filter（VTK 类：`vtkSurfaceVectors`）。

将表面网格上的向量属性按选定**约束模式**投影，在**独立的新模型**中替换同名数组；原模型及其属性保持不变：

| 约束模式 | 公式 | 说明 |
|----------|------|------|
| Parallel（默认） | `v' = v - (v·n)·n` | 去除法向分量，保留切向分量 |
| Perpendicular | `v' = (v·n)·n` | 仅保留法向分量 |
| PerpendicularScale | 输出标量 `v·n` | 法向投影长度（向量数组变为标量数组） |

其中 `v` 为原始向量，`n` 为该点/单元的单位法向量。

### 与 ParaView 完全对齐的口径（依据 ParaView 6.1.1 逐点数值比对）

1. **输出写回同名数组**：输出模型中 `Velocity` 的值就是投影结果（替换语义），
   与 ParaView 行为一致，不新增 `TangentVectors` 之类的独立数组；
2. **法向计算全链路 double 精度**，仅在写回数组时转 float；
3. **面法向**：任意多边形单元固定取**前 3 个顶点**计算三角形叉积
   （不使用 Newell 法、不使用扇形拆分）；
4. **环绕一致性修正**：BFS 遍历面邻接关系统一环绕方向（对齐 VTK 的 Consistency 行为），
   相邻面在共享边上顶点方向相同则判定环绕不一致并翻转其法向；
5. **点法向**：相邻面法向（未归一化，长度∝面积）**面积加权累加**后归一化。

### 与 ParaView 的对应关系

| 项目 | ParaView Surface Vectors | iGameVis |
|------|--------------------------|----------|
| 约束模式 | Parallel / Perpendicular / PerpendicularScale | 三模式完全一致 |
| 输出命名 | 写回同名数组 | 写回同名数组 |
| 点向量 / 单元向量 | 均支持 | 均支持 |
| 法向算法 | 前3顶点叉积 + 一致性修正 + 面积加权 | 逐位对齐（浮点噪声 < 1e-6） |

---

## 2. 调用方式

### 2.1 iGameVis GUI 调用

1. 加载表面模型文件（支持 VTK、OBJ、STL 等格式）；
2. 确保模型上存在向量属性（点向量或单元向量，维度 ≥ 3）；
3. 点击顶部菜单 **「算法处理」→「新增filter/第二批」→「Surface Vectors」**；
4. 参数面板中选择：
   - **向量属性**：列出所有维度 ≥ 3 的属性（自动标注 Point / Cell）；
   - **约束模式**：Parallel / Perpendicular / PerpendicularScale；
5. 点击「应用」执行，结果写回同名数组，新模型节点挂到模型树。

### 2.2 C++ 代码调用

```cpp
#include <SurfaceVectors/iGameSurfaceVectorsFilter.h>
using namespace iGame;

auto filter = SurfaceVectorsFilter::New();
filter->SetInput(surfaceMesh);

// 1. 指定向量属性（按名称或索引二选一）
filter->SetVectorAttributeByName("Velocity");
// 或 filter->SetVectorAttributeByIndex(0);

// 2. 设置约束模式（默认 Parallel）
filter->SetConstraintMode(SurfaceVectorsFilter::Parallel);
// filter->SetConstraintModeToPerpendicular();
// filter->SetConstraintModeToPerpendicularScale();

if (filter->Execute()) {
    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
    // 结果写回同名数组："Velocity" 现在的值即为投影结果
    auto& attr = outMesh->GetAttributeSet()->GetAttribute("Velocity");
} else {
    std::cout << "Error: " << filter->GetMessage() << std::endl;
}
```

---

## 3. 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| VectorAttribute | string / int | - | 要投影的向量属性（按名称或索引），必须设置 |
| ConstraintMode | enum | Parallel | Parallel=0（切向）、Perpendicular=1（法向）、PerpendicularScale=2（标量投影） |

---

## 4. 使用示例

### 示例 1：切向投影（Parallel，默认）

```cpp
auto mesh = FileIO::ReadFile("SurfaceVectors_C_cylinder.vtk");

auto filter = SurfaceVectorsFilter::New();
filter->SetInput(mesh);
filter->SetVectorAttributeByName("Velocity");
filter->SetConstraintModeToParallel();   // v' = v - (v·n)·n
filter->Execute();

// 圆柱面上 Velocity 的径向（法向）分量被去除；
// 写回后的 Velocity 与法向点积 ≈ 0（浮点噪声 < 1e-6 属正常）
```

### 示例 2：仅保留法向分量（Perpendicular）

```cpp
filter->SetConstraintModeToPerpendicular();  // v' = (v·n)·n
filter->Execute();
// 写回后的 Velocity 只剩法向分量，切向分量为 0
```

### 示例 3：法向投影标量（PerpendicularScale）

```cpp
filter->SetConstraintModeToPerpendicularScale();  // 输出标量 v·n
filter->Execute();
// 注意：此时同名数组由 3 分量向量变为 1 分量标量
```

---

## 5. 注意事项

### 5.1 输入要求

- 输入类型支持 `SurfaceMesh` 与 `UnstructuredMesh`（自动提取表面）；
- 属性维度必须 ≥ 3（向量），标量属性不出现在 GUI 候选列表中；
- 点向量（IG_POINT）与单元向量（IG_CELL）均支持，附着类型执行后保持不变
  （PerpendicularScale 模式例外：数组变为 1 分量标量，附着类型不变）。

### 5.2 写回语义（与旧版区别）

结果**直接替换输入向量数组**（同名写回），不再新增输出数组、
也没有「输出属性名」配置项——这是与 ParaView 对齐的行为。
如需保留原始向量，请先自行复制属性再执行。

### 5.3 数值精度

- 与 ParaView 对标时，个别点（如球面极点）的向量分量可能存在 < 1e-6 的浮点差异，
  属不同法向计算路径的正常舍入噪声；核心分量与模长完全一致即判定对齐；
- 网格环绕方向不一致时（如分块三角化的球面极点带），本 Filter 已做一致性修正；
  未修正的实现会让点法向偏移 16~26 度。

### 5.4 零法向处理

退化面（顶点共线）叉积长度为 0，归一化后仍为零向量，
Parallel 模式下投影结果等于原向量。

### 5.5 测试模型

`Examples/Models/` 目录下提供与 ParaView 逐点比对的测试模型：

| 文件名 | 描述 | 属性 |
|--------|------|------|
| SurfaceVectors_A_sphere.vtk | 球面（三角形网格，含环绕不一致的极点带） | Velocity 等（点/单元向量） |
| SurfaceVectors_B_hill.vtk | 山丘曲面（四边形网格） | 点/单元向量 |
| SurfaceVectors_C_cylinder.vtk | 圆柱面 | 点/单元向量 |

测试代码位于 `Examples/Filter/TestSurfaceVectors.cpp`，共 35 项断言
（覆盖三模型 × 点/单元 × 三模式的 ParaView 实测真值回归锁），运行后自动输出 PASS/FAIL 统计。
