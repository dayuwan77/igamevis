# Ribbon（带状面）使用说明

适用范围：iGameCore 带状面 Filter（类名 `RibbonFilter`）；Qt 界面入口为菜单「算法处理 → 开发中filter/第二批 → 带状面 (Ribbon)」，弹出通用参数面板。
语义基线：VTK `vtkRibbonFilter`（`Filters/Modeling/vtkRibbonFilter.cxx`），逐条对齐其平均切向、正交框架、半宽、偏转角与退化跳过语义。

## 功能

把输入的点/折线沿「线切向 × 线法向」构成的框架扫成有向带状面，输出一个新的 `SurfaceMesh`：

- **每条线/折线独立处理**（不同折线共享端点也不会互相污染），逐点构造正交框架：

| 量 | 定义 | 说明 |
| --- | --- | --- |
| 平均切向 `s` | `normalize((sPrev + sNext) / 2)` | 转角处自动倒角，首点用第一段、末点用最后一段 |
| 带宽方向 `w` | `normalize(cross(s, n))` | `n` 为该点的线法向（默认法向或输入点法向） |
| 框架第三轴 `nP` | `normalize(cross(w, s))` | 与 `w`、`s` 正交 |
| 张开方向 `v` | `w * cos(Angle) + nP * sin(Angle)` | `Angle = 0°` 时沿 `w`，`90°` 时沿 `nP`（线法向投影到垂直于切向的平面） |
| 两侧点 | `p∓ = p ∓ Width * v` | **`Width` 是半宽**，因此带宽恒为 `2 × Width` |

- **输出点**：每条成功折线输出 `2 × npts` 个点，顺序与 VTK 一致——**偶数索引是 `p-`（减号侧）、奇数索引是 `p+`（加号侧）**；输出点数 = `2 × Σnpts`（只统计被接受的折线）；
- **输出单元**：VTK 的 triangle strip 在此展开为逐段四边形，第 `i` 段输出 `(p-ᵢ, p+ᵢ, p+ᵢ₊₁, p-ᵢ₊₁)`；输出面数 = `Σ(npts − 1)`；
- 输出对象名 = 输入对象名 + `_Ribbon`；**输入对象不被修改**，输出是新对象；
- 输出统一为 `SurfaceMesh`（输出单元全是四边形面）。

支持的输入：

| 输入 | 处理 |
| --- | --- |
| `UnstructuredMesh` 的 `IG_LINE`（2 点） | 生成 1 段带面 |
| `UnstructuredMesh` 的 `IG_POLY_LINE`（≥ 2 点） | 逐段生成 `npts − 1` 个四边形 |
| `SurfaceMesh` 的**显式边数组**（legacy VTK `POLYDATA` 的 `LINES` 段） | 与上同；**不会**从面自动抽边，也不看面 |
| 其他单元类型（三角/四边/体/高阶…） | 不参与生成，数量与类型写入 `GetMessage()`；一条线都没有时 `Execute()` 失败 |

属性传播：

- 每个输出点记录来源点 ID、每个输出四边形记录来源线单元 ID，按此映射逐个复制；
- **数组名、具体类型（float/int/long long…）、维度、属性类型、挂载类型与数据范围全部保持**，不偷换成 `DoubleArray`；
- 与 VTK 的 `CopyNormalsOff()` 一致：**输入里的旧法向不会被复制**；本版本也**不生成新的输出点法向**。需要法向请在之后接 `SurfaceNormalsFilter`。

## 与 VTK / ParaView 的差异

| 项目 | VTK `vtkRibbonFilter` / ParaView | iGameVis |
| --- | --- | --- |
| 输入 | `vtkPolyData` 的 lines | `UnstructuredMesh`（`IG_LINE`/`IG_POLY_LINE`）或带显式边的 `SurfaceMesh` |
| 输出 | `vtkPolyData`，每条折线 **1 个 triangle strip** | `SurfaceMesh`，每条折线展开为 **`npts − 1` 个 `IG_QUAD`**（iGame 没有可持久化的 strip 容器，VTK reader 对 `TRIANGLESTRIP` 同样直接跳过）。**点数相同（`2Σnpts`），cell 数与 VTK 不同** |
| 点法向来源 | `UseDefaultNormal` 关且输入无点法向时，按折线自动生成「滑动法向」 | 首版**未实现滑动法向**：此时回退到 `DefaultNormal`，并在 `GetMessage()` 里说明；`UseDefaultNormal` 关且输入带 `IG_NORMAL` 时按 VTK 使用输入点法向 |
| `UseDefaultNormal` 默认值 | 关（0） | **开**（首版用显式法向保证行为确定、可复现） |
| 变宽 | `VaryWidth` + `WidthFactor`（按标量变宽） | 不做，面板不露出 |
| 纹理坐标 | `GenerateTCoords` + `TextureLength` | 不做 |
| 输出点法向 | 写入框架轴 `nP` | 不写入（与线性拉伸一致） |
| 折线中途退化 | 已插入的孤立点会留在输出里 | 先把整条折线的框架算完再写入，**不产生孤立点** |
| `Width <= 0` | `vtkSetClampMacro` 夹成 0，产出零宽退化面 | **失败**并给出原因 |
| `Angle` 越界 | 夹到 `[0, 360]` | 相同（setter 内夹取） |
| 非线单元 | 只遍历 lines，其余静默忽略 | 同样只处理线，但把忽略的数量与类型写进 `GetMessage()` |
| `Piece Invariant` | 基类提供的分片选项 | 无对应项。iGame 单进程整网格一次性执行，输出天然与分片无关 |

## 调用方式

### C++ 接口

```cpp
#include <Modeling/iGameRibbonFilter.h>
```

```cpp
auto filter = iGame::RibbonFilter::New();
filter->SetWidth(0.5);                     // 半宽，默认 0.5，必须 > 0
filter->SetAngle(0.0);                     // 偏转角（度），默认 0，超出 [0,360] 会被夹取
filter->SetDefaultNormal(0.0, 0.0, 1.0);   // 默认线法向，默认 (0,0,1)，必须非零
filter->SetUseDefaultNormal(true);         // 默认 true：全部点使用默认线法向
filter->SetInput(mesh);                    // UnstructuredMesh（IG_LINE/IG_POLY_LINE）或带显式边的 SurfaceMesh
if (!filter->Execute()) {
    std::cout << filter->GetMessage() << std::endl;   // 失败原因
    return;
}
auto out = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput());
std::cout << out->GetNumberOfPoints() << " points, " << out->GetNumberOfFaces() << " faces\n";
```

使用输入自带的点法向（VTK 的同名分支）：

```cpp
filter->SetUseDefaultNormal(false);
// 输入点属性里需要有 IG_NORMAL 数组（挂载类型 IG_POINT、维度 >= 3、元组数覆盖全部点）；
// 没有可用点法向时回退到 DefaultNormal，Execute() 仍返回 true，说明写在 GetMessage() 里。
// iGame 没有额外的「回退状态」标志，需要区分时检查 GetMessage() 是否为空。
```

参数默认值：`Width = 0.5`（半宽，与 `vtkRibbonFilter` 构造函数实际赋值一致）、`Angle = 0`、`DefaultNormal = (0,0,1)`、`UseDefaultNormal = true`。

### GUI 操作

1. 打开模型并在模型树中选中它（输入需包含线/折线单元，见「测试模型」）；
2. 菜单「算法处理 → 开发中filter/第二批 → 带状面 (Ribbon)」→ 弹出参数面板；
3. 在面板里设置：
   - **半宽 (Width)**：默认 `0.5`，带宽 = 2 × 半宽；
   - **偏转角/度 (Angle)**：默认 `0`，0–360，超出会被夹到区间内（与 VTK 一致）；
   - **使用默认线法向 (Use Default Normal)**：默认勾选；取消勾选时优先取输入的点法向，没有则回退默认法向；
   - **默认法向 X / Y / Z**：默认 `0 / 0 / 1`，不能是零向量；
4. 点「应用」：结果作为新节点（命名 `原模型名_Ribbon`）加入模型树，视口随即刷新；若本次执行忽略过非线单元或有折线被跳过，会额外弹一次信息提示说明原因。

半宽不是数字、半宽 ≤ 0、默认法向为零向量、执行失败（无输入 / 无线单元 / 全部折线被跳过）都会用深色提示弹窗给出具体原因，不会静默出空结果。

## 提示与错误信息

`GetMessage()` 同时承载「失败原因」与「非致命提示」，常见文案：

| 文案（节选） | 触发条件 | `Execute()` |
| --- | --- | --- |
| `requires Width > 0 (Width is the ribbon half width)` | 半宽 ≤ 0 | 失败 |
| `requires a non-zero DefaultNormal` | 默认法向为零向量且需要它定向 | 失败 |
| `found no line cell to ribbon in the input` | 输入里没有任何线/折线单元 | 失败 |
| `skipped every line cell; no ribbon could be generated` | 所有折线都因退化被跳过 | 失败 |
| `input SurfaceMesh has no explicit edges` | `SurfaceMesh` 输入没有显式边数组 | 失败 |
| `only processes line cells; ignored N non-line cell(s) of type …` | 混有三角/四边/体单元等 | 成功（提示） |
| `skipped Line cell 3 which has coincident points` | 折线含相邻重复点 | 成功（提示，该折线无输出） |
| `skipped PolyLine cell 5 which is parallel to its line normal` | 切线与该点法向平行（例如竖直线配默认法向 (0,0,1)） | 成功（提示，该折线无输出） |
| `found no usable point normals; UseDefaultNormal is off, so the DefaultNormal is used instead` | 关掉默认法向但输入没有点法向 | 成功（提示，按默认法向执行） |

## 使用示例

### 命令行用例

测试用例在 `Examples/Filter/TestRibbon.cpp`（ctest 名 `testRibbon`）。**不带参数时使用仓库自带模型**，也可以传模型目录或直接传一个 `.vtk` 文件：

```bash
# Windows：在 Examples 构建目录下
testRibbon.exe                # 默认读 ./Models/Ribbon_Polylines2D.vtk（模型 B 与混合模型同目录取）
ctest -R testRibbon --output-on-failure
testRibbon.exe D:/igamevis/Examples/Models
testRibbon.exe D:/some/path/MyLines.vtk
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例覆盖（12 组）：

- 单条直线：4 点 1 个四边形、`|p+ − p−| = 2 × Width`、截面中点仍在原线上、偶数索引在 `−v` 侧、四边形绕序给出的法向与 `nP` 同向；
- L 形折线：6 点 2 个四边形，中间点由平均切向 `(p₂ − p₀)` 解析算出，全部坐标有限，输入未被修改；
- `Angle = 90°`：带面沿 `nP` 张开，与 0° 情形正交；
- 属性传播：点属性两份副本同值、单元属性每个四边形都取到来源线单元的值、数组名/类型/维度保持、旧法向不复制；
- 多单元来源映射：四边形数 = `Σ(npts − 1)`，每个四边形拿到**自己那条**线单元的属性；
- 退化：竖直线配默认法向、含重复点的折线被跳过且不产生孤立点与 NaN；全部跳过 / `Width = 0` / 零法向失败；关掉默认法向且无点法向时回退；
- 非法输入：无输入、只有体单元、无单元、无点、`SurfaceMesh` 无显式边 → 失败且不保留上一次输出；失败后紧接一次合法执行仍成功；
- 两个自带模型文件与「混合单元真实文件」的端到端断言。

### 命令行示例：L 形折线生成带状面

```cpp
#include <iostream>
#include <iGameCellArray.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <Modeling/iGameRibbonFilter.h>

int main() {
    // 3 点 L 形折线：(0,0,0) -> (1,0,0) -> (1,1,0)
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0.0, 0.0, 0.0);
    points->AddPoint(1.0, 0.0, 0.0);
    points->AddPoint(1.0, 1.0, 0.0);
    mesh->SetPoints(points);

    auto cells = iGame::CellArray::New();
    auto types = iGame::UnsignedIntArray::New();
    igIndex ids[3] = {0, 1, 2};
    cells->AddCellIds(ids, 3);
    types->AddValue(iGame::IG_POLY_LINE);   // 注意：iGame 的单元类型常量在 iGame 命名空间下
    mesh->SetCells(cells, types);

    auto filter = iGame::RibbonFilter::New();
    filter->SetWidth(0.5);                  // 半宽 -> 带宽 1.0
    filter->SetAngle(0.0);
    filter->SetDefaultNormal(0.0, 0.0, 1.0);
    filter->SetInput(mesh);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto out = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput());
    // 3 点折线 -> 6 个输出点、2 个四边形；转角处的截面由平均切向决定
    std::cout << "points=" << out->GetNumberOfPoints() << " faces=" << out->GetNumberOfFaces() << "\n";
    return 0;
}
```

## 测试模型

两个模型放在 `Examples/Models`，纯 ASCII legacy VTK，iGameVis 与 ParaView 都能读取；都是 `DATASET UNSTRUCTURED_GRID`（线单元用 `CELL_TYPES` 3 = `LINE`、4 = `POLY_LINE`），并带点/单元属性用于校验属性映射：

| 文件 | 内容 | 期望结果（`Width = 0.5`、`Angle = 0`、默认法向 `(0,0,1)`） |
| --- | --- | --- |
| `Ribbon_Polylines2D.vtk` | z=0 平面内：5 点阶梯折线 + 3 点共线折线 + 1 条 2 点线段，共 **10 点 / 3 单元**；点属性 `point_id`、单元属性 `chain_id`(10/20/30) | **20 点 / 7 个四边形**（4+2+1）；每个四边形继承自己来源线单元的 `chain_id` |
| `Ribbon_Helix3D.vtk` | 16 点螺旋线（半径 1、z 从 0 到 3）+ 1 条斜线 `(3,0,0)→(3,1,1)`，共 **18 点 / 2 单元**；点属性 `height`、单元属性 `curve_id`(100/200) | **36 点 / 16 个四边形**（15+1）；所有切向都不与默认法向平行，无折线被跳过 |

**在 ParaView 里的对照做法**（重要，参数不对齐会看到完全不同的结果）：

1. 打开模型后先 `Filters → Convert To PolyData`（`vtkRibbonFilter` 的输入类型是 `vtkPolyData`，而我们的模型是 `UnstructuredGrid`；也可用 `Extract Surface`）；
2. 再 `Filters → Ribbon`，参数设：
   - `Width = 0.5`（同一半宽）、`Angle = 0`；
   - **勾选 `Use Default Normal`，并把 `Default Normal` 设为 `(0, 0, 1)`** —— ParaView 默认不勾，会按折线自动生成「滑动法向」，带面朝向与 iGame 不同（这正是上表差异里的第一条）；
3. 用 `Information` 面板对比：**点数应与上表一致**（`2Σnpts`）；**cell 数会不同**（ParaView 每条折线 1 个 triangle strip，iGame 是 `npts − 1` 个四边形），这是容器差异而非几何差异，看渲染效果即可。

## 注意事项

1. **半宽不是总宽**：面板/接口里的 `Width` 是半宽，最终带宽是 `2 × Width`。写 0.5 得到 1.0 宽的带子。
2. **切线与法向平行时整条折线被跳过**：最典型的是「竖直线 + 默认法向 (0,0,1)」——此时 `cross(s, n) = 0`，带面方向无法确定（VTK 同样放弃该折线）。换一个不平行的默认法向，或给输入点法向。
3. **相邻重复点会让整条折线被跳过**：与 VTK 的 `Coincident points!` 行为一致。上游数据请先清理重复点（ParaView 的 `Clean` / iGame 的相应预处理）。
4. **输入只吃线/折线**：`UnstructuredMesh` 需要 `IG_LINE`/`IG_POLY_LINE`；`SurfaceMesh` 需要文件里显式的 `LINES` 段，**只有面的模型不会自动抽边**（先用 `Extract Edges` 之类得到线单元）。
5. **混有面/体单元时不报错**：只忽略它们并在 `GetMessage()` 里说明忽略了多少个、什么类型；但如果一条线都没有，`Execute()` 失败。
6. **输出是零厚度壳体**：带面是二维单元（顶点在三维空间），不含厚度方向的信息；需要实体请对结果另做处理。
7. **输出单元顺序**：按输入线单元的顺序依次输出，每条折线内按段顺序；每个四边形都带该来源线单元的单元属性，因此下游按序处理与按属性处理都能对上。
8. **失败时不产生半成品**：某条折线只要命中退化条件就整条不写入（不会像 VTK 那样留下没有单元引用的孤立点）；全部折线都被跳过时 `Execute()` 返回 `false`，`GetOutput()` 为空。
9. **本版本不做**：按标量变宽（`VaryWidth`/`WidthFactor`）、纹理坐标（`GenerateTCoords`/`TextureLength`）、滑动法向自动生成、输出点法向。面板上也没有这些选项，不要照着 VTK 全参数逐项对比。
10. 用例按相对路径 `./Models/...` 读取模型，需在 Examples 构建目录下运行；在其它目录运行请传入模型目录或 `.vtk` 文件路径。
