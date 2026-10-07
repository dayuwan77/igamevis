# CellCenterFilter（单元几何中心）使用说明

## 一、功能

遍历输入网格的所有单元，计算每个单元的几何中心（单元所有顶点坐标的平均值），
输出一个 `PointSet`（点集）：每个输入单元对应一个输出中心点，可直接作为点云渲染。

属性处理：

- **点属性**：插值到单元中心（取该单元所有顶点属性值的均值）；
- **单元属性**：按单元索引拷贝（第 i 个中心点 ↔ 第 i 个单元），统一标记为 `IG_POINT`，
  输出点集上可直接用于着色；
- 输出数组按输入数组的实际类型创建（`Double`/`Int` 保留类型与精度，不降级为 `Float`）。

适用输入：

- `UnstructuredMesh`（非结构网格）
- `VolumeMesh`（体网格）
- `SurfaceMesh`（面网格）
- `StructuredMesh`（结构网格，内部自动生成隐式连接表后处理）

## 二、调用方式

### 2.1 界面调用（菜单）

菜单路径：**算法处理 → 单元几何中心 (Cell Center)**

步骤：

1. 加载一个网格数据（.vtk / .obj / .cgns 等）；
2. 在模型树中选中该模型；
3. 点击菜单 **算法处理 → 单元几何中心 (Cell Center)**；
4. 结果（点集，命名形如 `原模型名_cell_center`）自动加入模型树并显示。

> 仅支持含单元的数据（非结构/体/面/结构网格）；纯点集（PointSet）会提示执行失败。

### 2.2 代码调用

```cpp
#include <MyFilter/iGameCellCenterFilter.h>
#include <iGameFileIO.h>

auto obj = iGame::FileIO::ReadFile("./Models/CellCenter_hexa_grid.vtk");
if (!obj) return 1;

auto filter = iGame::CellCenterFilter::New();
filter->SetInput(obj);                    // Filter：设置输入
if (!filter->Execute()) return 1;         // Filter：执行

auto out = filter->GetOutput();           // Filter：取输出（PointSet）
auto centers = iGame::DynamicCast<iGame::PointSet>(out);
std::cout << "输出中心点数: " << centers->GetNumberOfPoints() << std::endl;
```

## 三、使用示例

仓库提供 2 个自动生成的测试模型（`Examples/Models/`）：

| 模型 | 内容 | 用途 |
|------|------|------|
| `CellCenter_hexa_grid.vtk` | 2 层 3×3 格点构成 4 个六面体，带 float/double 点属性、double 单元属性 | 默认测试模型：中心坐标可精确校验（4 个中心分别为 (0.5/1.5, 0.5/1.5, 0.5)） |
| `CellCenter_surface_mixed.vtk` | 平面三角+四边形表面网格，带 double 属性 | 表面网格场景 |

运行测试用例（示例代码已写死相对路径，无需手动输入）：

```bat
cd cmake-build-examples
testCellCenterFilter.exe
```

预期输出：

```text
Input cells:  4
Output points: 4
PASS: one output point per input cell
PASS: cell centers match expected coordinates
PASS: all attribute lengths match output points
PASS: CellCenterFilter test finished
```

同时弹出渲染窗口显示 4 个单元中心点。

## 四、注意事项

1. **几何中心 ≠ 质心**：当前实现是"单元顶点坐标的均值"，不是按体积/面积加权的质心；
2. **输出是点集（PointSet）**：没有单元拓扑，不能再作为网格处理；若要作为网格，需另行转换；
3. **渲染显示**：输出 PointSet 默认视图样式是"填充面"，但点集没有面，界面中需点模型树的"点"按钮
   （或代码里 `SetViewStyle(IG_POINTS)`）才能看到点云；
4. **属性标记**：输出中的点属性与单元属性都统一标记为 `IG_POINT`（第 i 个中心点 ↔ 第 i 个单元），
   均可用于云图着色；
5. **数值精度**：输出数组类型与输入一致（Double/Int 不会被降级为 Float）；
6. **结构化网格**：`StructuredMesh` 的 `GetCellArray()` 返回空，本 filter 会自动调用
   `GenStructuredCellConnectivities()` 生成隐式连接表后处理，无需额外设置；
7. **空输入/无单元数据**：`Execute()` 返回 `false`，调用方需处理失败情况。
