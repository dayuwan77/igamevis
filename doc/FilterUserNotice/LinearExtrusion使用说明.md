# LinearExtrusion（线性拉伸）使用说明

适用范围：iGameCore 线性拉伸 Filter（类名 `LinearExtrusionFilter`）；Qt 界面入口为菜单「算法处理 → 线性拉伸 (Linear Extrusion)」，弹出独立置顶面板（不占用左侧工具面板）。
语义基线：VTK `vtkLinearExtrusionFilter`（`Filters/Modeling/vtkLinearExtrusionFilter.cxx`），逐条对齐其位移公式、单元生成规则与属性拷贝策略。

## 功能

把输入网格沿指定规则"扫掠"出第二层几何，输出一个新的数据对象：

- 每个输入点复制为两层：第 1 层保持原坐标（输出 ID `[0, N)`），第 2 层为位移后的点（输出 ID `[N, 2N)`）；因此输出点数恒为 `2N`；
- 位移量 `Δ` 由模式决定，最终位移后坐标为 `p + ScaleFactor * Δ`：

| 模式 | Δ | 说明 |
| --- | --- | --- |
| Vector（沿向量） | `Vector` | 所有点沿同一方向平移；`Δ = ScaleFactor × Vector`，**不对 Vector 做归一化** |
| Normal（沿点法向） | 该点的点法向 | 从输入点属性的 `IG_NORMAL` 数组读取；**输入没有可用点法向时自动回退成 Vector 模式**，并在 `GetMessage()` 中说明 |
| Point（沿点缩放） | `p - ExtrusionPoint` | 即 `p + ScaleFactor × (p - ExtrusionPoint)`；ScaleFactor 为负时朝基点收缩 |

- 输出单元按输入单元维度生成（与 VTK 的 `dim == 0 / 1 / 2` 三分支一致）：

| 输入单元 | 输出 | Capping 影响 |
| --- | --- | --- |
| `IG_VERTEX` | `IG_LINE`：`[p, p+N]` | 无 |
| `IG_LINE` | `IG_QUAD`：`[p1, p2, p1+N, p2+N]` | 无 |
| `IG_POLY_LINE` | 每段一个 `IG_QUAD`（点序同上） | 无 |
| `IG_TRIANGLE` / `IG_QUAD` / `IG_POLYGON` / `IG_FACE` | 端面（见下）+ 自由边侧裙 | 有 |

- **端面（Capping）**：打开时，每个二维单元输出两份——原端面（连接表原样）与位移端面（连接表整体 `+N`，顶点顺序不反转）；关闭时两份都不输出；
- **侧裙**：只对**自由边**（只被一个二维单元使用的边）生成 `IG_QUAD`；被两个单元共享的内部边不生成任何东西。因此二维单元部分的面数满足：

```
Capping 开：2*F + B        Capping 关：B
F = 输入二维单元数，B = 输入自由边数
```

- 输出对象名 = 输入对象名 + `_LinearExtrusion`；**输入对象不被修改**，输出是新对象；
- 输出统一为 `UnstructuredMesh`，因为它能同时容纳线、四边形与多边形等混合单元。

支持与不支持的输入：

- 输入可以是 `SurfaceMesh`（只有面）或 `UnstructuredMesh`；
- `UnstructuredMesh` 接受的单元类型：`IG_VERTEX`、`IG_LINE`、`IG_POLY_LINE`、`IG_TRIANGLE`、`IG_QUAD`、`IG_POLYGON`、`IG_FACE`；
- 体单元（`IG_TETRA` / `IG_HEXAHEDRON` / `IG_PRISM` / `IG_PYRAMID` 等）、高阶单元（`IG_QUADRATIC_*` 等）、`IG_POLYHEDRON` **明确报错**（`m_Message` 里带上 cellId 与类型名），不做静默跳过；
- 无输入、无点、无单元、`SurfaceMesh` 无面，均失败并给出原因。

属性传播：

- 点属性按"输出点 → 输入点"的映射逐个复制，单元属性按"输出单元 → 输入单元"的映射复制，**数组名、具体类型（float/int/long long…）、维度、属性类型、挂载类型与数据范围都保持**，不偷换成 `DoubleArray`；
- 顶点生成的线、线/折线生成的四边形、两端面、边界侧裙，都会拿到其来源输入单元的属性值；
- 与 VTK 的 `CopyNormalsOff()` 一致，**旧法向数组不会被复制到输出**（几何已经改变，旧法向不再成立）。需要法向请在拉伸之后重新计算（如 `SurfaceNormalsFilter`）。

## 与 VTK / ParaView 的差异

| 项目 | VTK `vtkLinearExtrusionFilter` / ParaView | iGameVis |
| --- | --- | --- |
| 输入 | `vtkPolyData` | `SurfaceMesh` 或 `UnstructuredMesh` |
| 输出 | `vtkPolyData`（Verts/Lines/Polys/Strips 四类容器） | 统一 `UnstructuredMesh`（iGame 没有与 vtkPolyData 等价的对象） |
| 带面（线/折线拉伸结果） | `vtkTriangleStrip`（每段 4 点） | `IG_QUAD`（点序与 VTK 内部 strip 存储顺序相同：`p1, p2, p1+N, p2+N`，这个顺序是**环状**的，可安全拆三角形）。iGame 没有可持久化的 triangle strip 容器，VTK reader 对 `TRIANGLESTRIP` 也是直接跳过、不产出单元，因此用等价的四边形表达 |
| 顶点 | 只有 VTK reader 不产出的情况下才有 `verts`；iGame 侧 VTK reader 同样不产出 `IG_VERTEX` | 手工程序化构造 `IG_VERTEX` 时按 `dim==0` 正常生成线 |
| 体单元输入 | 直接拒绝（输入不是 vtkPolyData） | 明确报错并指出是哪个 cell |
| `Piece Invariant` | 基类 `vtkPolyDataAlgorithm` 提供的选项，默认勾选：多分片执行时先合并输入再计算，保证结果与分片方式无关 | **无对应项**。iGame 是单进程、整网格一次性执行，不存在分片路径，输出天然与分片无关，因此面板上也没有这个开关 |
| 输出是"体"吗 | 不是。生成的是**零厚度的壳**（顶点在三维空间、单元仍是二维），VTK 同样不产生四面体/六面体 | 相同。需要真三维单元请对闭合壳再做体网格化（iGame 有 `iGameMeshTetrahedralize`；ParaView 可用 `Delaunay 3D`） |

## 调用方式

### C++ 接口

```cpp
#include <Modeling/iGameLinearExtrusionFilter.h>
```

```cpp
auto filter = iGame::LinearExtrusionFilter::New();
filter->SetExtrusionTypeToVectorExtrusion();   // Vector；另有 SetExtrusionTypeToNormalExtrusion() / ToPointExtrusion()
filter->SetVector(0.0, 0.0, 1.0);              // 方向向量，默认 (0,0,1)
filter->SetScaleFactor(2.0);                   // 距离系数，默认 1.0，可为负
filter->CappingOn();                           // 端面封闭，默认开；CappingOff() 关闭
filter->SetInput(mesh);                        // SurfaceMesh 或 UnstructuredMesh
if (!filter->Execute()) {
    std::cout << filter->GetMessage() << std::endl;   // 失败原因
    return;
}
auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
```

Point 模式（以基点为缩放中心）：

```cpp
filter->SetExtrusionTypeToPointExtrusion();
filter->SetExtrusionPoint(0.0, 0.0, 0.0);      // 默认 (0,0,0)
filter->SetScaleFactor(2.0);                   // 位移后 p' = p + 2 * (p - 基点)
```

Normal 模式（跟随点法向）：

```cpp
filter->SetExtrusionTypeToNormalExtrusion();
filter->SetScaleFactor(1.0);
// 输入点属性里需要有 IG_NORMAL 数组（维度 >= 3、元组数覆盖全部点）；
// 没有时自动按 Vector 模式执行，成功返回 true，说明写在 GetMessage() 里。
// iGame 不自带一个"只读 GetMessage 之外"的回退状态标志，需要区分时请检查 GetMessage() 是否为空。
```

参数默认值（与 VTK 构造函数一致）：`ExtrusionType = Normal`、`Capping = 开`、`ScaleFactor = 1.0`、`Vector = (0,0,1)`、`ExtrusionPoint = (0,0,0)`。

### GUI 操作

1. 打开模型并在模型树中选中它；
2. 菜单「算法处理 → 线性拉伸 (Linear Extrusion)」→ 弹出**独立置顶面板**（非模态，点右上角 X 才关闭，关闭后再打开复用同一面板）；
3. 在面板里设置：
   - **拉伸模式**：沿向量 (Vector) / 沿点法向 (Normal) / 沿点缩放 (Point)，默认沿点法向；
   - **方向向量 X / Y / Z**：默认 `0 / 0 / 1`，仅 Vector 模式参与（其它模式下整组变灰）；
   - **缩放系数（距离系数）**：默认 `1.0`，三种模式都参与，允许负数；
   - **拉伸基点 X / Y / Z**：默认 `0 / 0 / 0`，仅 Point 模式参与；
   - **端面封闭 (Capping)**：默认勾选；
4. 点「应用」：首次执行在模型树新增结果节点（命名 `原模型名_LinearExtrusion_序号`），对同一输入再次执行则更新该节点，不重复新建。结果节点可继续作为其它 Filter 的输入。

数值解析失败会明确报出是哪个字段（例如"方向向量 Y 不是有效数字"），不会静默按 0 处理；执行失败的原因（无输入、不支持的单元类型、找不到点法向等）原样显示在深色提示弹窗里。

## 使用示例

### 命令行用例

测试用例在 `Examples/Filter/TestLinearExtrusion.cpp`（ctest 名 `testLinearExtrusion`）。用例自带两个测试模型，不需要任何参数即可完整运行：

```bash
# Windows：在 Examples 构建目录下
testLinearExtrusion.exe
# 或
ctest -R testLinearExtrusion --output-on-failure
# 也可用第一个参数覆盖模型目录
testLinearExtrusion.exe D:/igamevis/Examples/Models
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例覆盖：

- 开放曲面 + Vector + Capping 开：点数 `2N`、单元数 `2*F+B`、两层坐标、位移端面连接表整体 `+N`、尾部 `B` 个侧裙；
- 同一模型 Capping 关：点数仍 `2N`、单元数 `= B`；
- 封闭曲面：Capping 开得 `2F` 个端面、**Capping 关得 0 个单元**（Execute 仍成功）；
- 三种位移模式（Vector / Point / Normal）的坐标语义、负 ScaleFactor、Normal 找不到点法向时回退 Vector 并写 `GetMessage()`；
- 点属性 `point_id` 与单元属性 `patch_id` 的映射与类型保持；旧法向不被复制；
- 非法输入（无输入、四面体网格、无单元网格、无点网格）必须失败、给出消息且不保留上一次输出；
- `SurfaceMesh` 输入分支与默认参数值。

### 命令行示例：拉伸开放曲面片

```cpp
#include <iostream>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <Modeling/iGameLinearExtrusionFilter.h>

int main() {
    auto data = iGame::FileIO::ReadFile("./Models/LinearExtrusion_OpenSurface.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(data);
    if (mesh == nullptr) return 1;

    auto filter = iGame::LinearExtrusionFilter::New();
    filter->SetExtrusionTypeToVectorExtrusion();
    filter->SetVector(0.0, 0.0, 1.0);
    filter->SetScaleFactor(2.0);
    filter->CappingOn();
    filter->SetInput(mesh);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // 开放曲面片：26 点 16 面 16 条自由边 -> 输出 52 点、2*16+16 = 48 个单元
    std::cout << "points=" << out->GetNumberOfPoints() << " cells=" << out->GetNumberOfCells() << "\n";
    return 0;
}
```

## 测试模型

两个模型放在 `Examples/Models`，纯 ASCII legacy VTK，iGameVis 与 ParaView 都能直接读取：

| 文件 | 格式 | 内容 | 期望结果（Vector=(0,0,1)，ScaleFactor=2） |
| --- | --- | --- | --- |
| `LinearExtrusion_OpenSurface.vtk` | `DATASET POLYDATA`（iGame 读成 `SurfaceMesh`） | 4×4 开放四边形片：**26 点 / 16 个四边形 / 自由边 B=16**；点属性 `point_id`、单元属性 `patch_id`(10..43) | 52 点；Capping 开 **48** 个单元（16 原面 + 16 位移面 + 16 侧裙）；Capping 关 **16** 个单元 |
| `LinearExtrusion_ClosedCube.vtk` | `DATASET POLYDATA` | 封闭立方体：**8 点 / 12 个三角面 / 自由边 B=0**；点属性 `point_id`(0..7)、单元属性 `face_id`(10..61) | 16 点；Capping 开 **24** 个单元（12 + 12，无侧裙）；Capping 关 **0** 个单元 |

在 ParaView 里的对照做法：直接打开这两个文件（本身就是 `vtkPolyData`），选中后 `Filters → Linear Extrusion`，参数设 `Extrusion Type = Vector`、`Vector = (0,0,1)`、`Scale Factor = 2`、`Capping` 勾选/取消，用 `Information` 面板读出的点数与单元数应分别是上表的 52/48、52/16、16/24、16/0。

## 注意事项

1. **输出是"壳"不是"体"**：拉伸只把点复制两层，生成的单元仍是二维的（顶点在三维空间）。要真三维单元，请对闭合壳再做体网格化（iGame 的 `iGameMeshTetrahedralize`，或 ParaView 的 `Delaunay 3D`）。
2. **拉伸方向必须离开原始平面**：若 Vector 完全落在输入平面内（例如平面在 z=0、Vector=(1,0,0)），两层端面会共面重叠、侧裙退化，壳不闭合，后续体网格化会失败或产生垃圾。
3. **封闭曲面 + Capping 关 = 空结果**：没有自由边就没有侧裙，没有 Capping 就没有端面，`Execute()` 仍返回 `true` 但输出 **0 个单元**（点仍然是 `2N` 个）。这不是错误，是 VTK 的语义。
4. **Vector 不做归一化**：位移量就是 `ScaleFactor × Vector`，所以 `Vector=(0,0,10)`、`ScaleFactor=1` 与 `Vector=(0,0,1)`、`ScaleFactor=10` 等价；`Vector` 为零向量时不报错，只是两层点重合（与 VTK 一致）。ScaleFactor 为负表示反向拉伸。
5. **Normal 模式需要输入带点法向**：只认"挂载类型 = `IG_POINT`、属性类型 = `IG_NORMAL`、维度 ≥ 3、元组数覆盖全部点"的数组，不靠数组名猜测。找不到就回退成 Vector 模式并写 `GetMessage()`，`Execute()` 返回 `true`。
6. **旧法向不会被复制到输出**：拉伸后几何已变，沿用旧法向会得到错误结果（与 VTK `CopyNormalsOff()` 一致）。需要法向请重新计算。
7. **不支持的单元类型会直接失败**：体单元/高阶单元/多面体输入的 `Execute()` 返回 `false`，`GetMessage()` 里带 cellId 与类型名。先做 `Extract Surface` 之类的转换再拉伸。
8. **输出单元顺序**：先输出所有线/四边形带（来自顶点与线/折线），然后对每个二维单元**成对**输出「原端面、位移端面」——即第 `2i` 个输出单元是输入面 `i` 的原端面、第 `2i+1` 个是它的位移端面（连接表整体 `+N`）——最后输出全部 `B` 个自由边侧裙。与 VTK 的属性拷贝顺序一致；依赖顺序做下游处理时请按这个约定。
9. **侧裙的顶点顺序是环状的** `[a, b, b+N, a+N]`，**不是**照抄 VTK 内部 strip 的存储顺序 `[a, b, a+N, b+N]`。后者在几何上是个自交的"蝴蝶结"四边形，而 iGame 把四边形按「从 0 号点出发的扇形」拆三角形（见 `iGameSurfaceMesh.cpp` 的 `ConvertToDrawableData`），拆出的两个三角形会互相重叠并各留一条缝，侧面上表现为**条状空隙**。环状顺序拆出的 `(a,b,b+N)` 与 `(a,b+N,a+N)` 正好铺满，ParaView 与 iGameVis 都渲染正确。
10. 用例按相对路径 `./Models/...` 读取模型，需在 Examples 构建目录下运行；在其它目录运行请传入模型目录参数（`testLinearExtrusion.exe D:/igamevis/Examples/Models`）。
