# 表面切向量 GenerateSurfaceTangents 使用说明

> 对应需求：`generate_surface_tangents` —— 生成表面切向量，**根据表面几何和纹理坐标计算**。
> 对应 ParaView / VTK 里的 `vtkPolyDataTangents`（`Compute Point Tangents` 打开、
> `Compute Cell Tangents` 关闭时的默认行为）。

## 1. 功能（这个 filter 做什么）

根据**表面几何**（点坐标 + 单元拓扑）和**纹理坐标**（点属性 `IG_TCOORD`，2 个分量）
算出每个点的**切向量**，写在点属性上：

- 数组类型：`IG_VECTOR`（普通三分量向量）
- 数组分量：3
- 数组默认名字：`Tangents`（可以用 `SetPointTangentsArrayName()` 改）
- 内容是**单位向量**，方向是"纹理坐标 u 增大的方向"（也就是 `dP/du` 在表面上的方向）
- 另外两个可选输出（默认关闭）：
  - 单元切向量：类型 `IG_VECTOR`、附着在单元上、**不归一化**（和 VTK 一样，是原始的 `dP/du`）
  - 副切向量 `Bitangents`：点属性，单位向量，方向是"纹理坐标 v 增大的方向"（`dP/dv`）
- 支持的输入：表面网格（SurfaceMesh）；非结构网格（UnstructuredMesh）里的
  **三角形 / 四边形 / 多边形**单元；四边形和多边形内部按扇形三角化
- **不需要法向量**，也不修改输入：输出是输入的深拷贝，只多出切向量数组
  （这样才能对别的 filter 的输出继续处理）

切向量是"法线贴图 / 各向异性光照"这类渲染需要的东西；对网格处理来说，
它也是描述纹理坐标在表面上怎么"拉伸"的量。

## 2. 参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| 纹理坐标（数组名） | 自动（第一个 `IG_TCOORD` 点属性） | 用哪一份纹理坐标来计算 |
| 输出点切向量 | 开 | 写点属性 `Tangents`（单位向量） |
| 输出单元切向量 | 关 | 写单元属性 `Tangents`（原始 `dP/du`，不归一化） |
| 输出副切向量 | 关 | 再写一份点属性 `Bitangents`（单位向量） |
| 输出数组名 | `Tangents` / `Tangents` / `Bitangents` | 三个名字都可以用 setter 改 |

## 3. 算法（表面几何 + 纹理坐标）

### 3.1 每个三角形算一次 `dP/du`、`dP/dv`

把每个单元扇形三角化成若干三角形（三角形单元就是它自己）：`(p0, p1, p2)`，
纹理坐标 `(u0, v0)`、`(u1, v1)`、`(u2, v2)`。记

```
e1 = p1 - p0        e2 = p2 - p0
d1 = uv1 - uv0      d2 = uv2 - uv0
den = d1.u * d2.v - d2.u * d1.v
```

这就是经典的"由纹理坐标求切空间基"（Lengyel, *Computing Tangent Space Basis Vectors
for an Arbitrary Mesh*）：

```
dP/du = (e1 * d2.v - e2 * d1.v) / den
dP/dv = (e2 * d1.u - e1 * d2.u) / den
```

### 3.2 点切向量 = 周围三角形**等权**累加后归一化

每个三角形把自己的 `dP/du` 加到它的三个顶点上，**不乘面积权重**，最后每个点再归一化：

```
T[i] = normalize( Σ 所有含点 i 的三角形 dP/du )
```

这里特意用"等权"而不是"面积加权"，因为 ParaView 的 `vtkPolyDataTangents` 就是等权：
测试场景 2 用一个面积比 100:1 的两个三角形专门验证了这一点（等权 → `(0.99995, 0.01, 0)`，
面积加权 → `(1, 0.0001, 0)`，实测 ParaView 给的是前者）。测试场景 3 用一个不规则网格
和 ParaView 的输出逐点对齐，最大差 `6e-8`（只有 float32 的舍入误差）。

### 3.3 副切向量

把 `dP/dv` 也按同样方式累加到点上，然后**对点切向量做一次正交化并归一化**：

```
B[i] = normalize( Σ dP/dv - T[i] * dot(T[i], Σ dP/dv) )
```

于是 `T · B = 0`、`|T| = |B| = 1`，而 `(T, B)` 的手性由纹理坐标决定
（`v` 翻转时 `B` 也跟着翻）。

### 3.4 两个退化情况

- **纹理坐标退化的三角形**（同一个三角形的三个点 uv 共线，`den ≈ 0`）：直接跳过。
  `den` 用相对阈值判断（`|den| ≤ 1e-7 * |d1| * |d2|`），避免 `1/den` 爆成天文数字。
  **VTK 在这类单元上会算出 NaN / Inf**，我们不会（测试场景 5 专门验证了这点）。
- **点的切向量正好抵消成零**（孤立点，或者周围三角形的 `dP/du` 正好互相抵消）：
  该点输出 `(0, 0, 0)`，和 VTK 一致。

## 4. 调用方式

### 4.1 图形界面（iGameVis）

1. 打开 `F:\igamevis\build\Release\iGameVis.exe`
2. `文件 -> 打开文件`，选 `F:\igamevis\Examples\Models\SurfaceTangents.vtk`
   （演示模型：39 点 / 48 三角形的圆柱面，自带 `TCoords` 和 `Normals`）
3. 在模型树里点一下这个模型，让它成为当前模型
4. 菜单 `算法处理`（`ui->menu_filters`）→ `表面切向量 (Generate Surface Tangents)`
5. 面板里：
   - `纹理坐标`：下拉框列出当前模型里所有 `IG_TCOORD` 点属性（默认选第一个）
   - `输出点切向量 Tangents`（默认勾选）
   - `输出单元切向量`（默认不勾）
   - `输出副切向量 Bitangents`（默认不勾）

   如果当前模型**没有**纹理坐标，会直接弹提示（切向量必须由纹理坐标算出来），
   这时先做一次「球面纹理坐标 (Texture Map To Sphere)」之类的 filter 即可。
6. 点「应用」，模型树里会多出一个新节点 `SurfaceTangents_tangents`，原模型不动。
   在模型树里点一下这个新节点，让它成为当前模型（这样下一步的「矢量场」面板才能挑到
   `Tangents`）
7. 在「查找信息」面板里可以看到点属性 `Tangents`（3 个分量）
8. 把切向量画成箭头：点工具栏的「向量 / Glyph」按钮（`action_Vector` / `action_Glyph`），
   左侧「向量」面板里把向量名选成 `Tangents`，需要的话调一下箭头头部/尾部比例和绘制间隔，
   再点「绘制」，模型上就会出现一排箭头 —— 箭头方向就是纹理 u 增大的方向

### 4.2 C++ 调用

```cpp
#include "GenerateSurfaceTangents/iGameGenerateSurfaceTangentsFilter.h"

auto filter = iGame::GenerateSurfaceTangentsFilter::New();
// filter->SetTextureCoordinatesArrayName("TCoords");  // 不设 = 自动找第一个 IG_TCOORD
filter->SetComputePointTangents(true);                 // 点切向量（默认就是 true）
// filter->SetComputeCellTangents(true);               // 可选：单元切向量
// filter->SetComputeBitangents(true);                 // 可选：副切向量
filter->SetInput(0, dataObject);
if (filter->Execute()) {
    auto out = filter->GetOutput();                    // 独立输出，输入不变
}
```

### 4.3 跑测试程序

```powershell
cd F:\igamevis
cmake --build build --config Release --target testGenerateSurfaceTangents --parallel
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
F:\igamevis\build\Examples\Release\testGenerateSurfaceTangents.exe
```

看到最后一行 `ALL TESTS PASSED`、退出码 0 就算通过（一共 10 个场景）。

## 5. 使用示例（从演示模型到箭头显示）

下面这条流程就是录屏里演示的流程，全部用仓库自带的模型，**不需要 Tet_Plane.vtk**：

**iGameVis 侧**

1. 打开 `iGameVis.exe` → `文件 -> 打开文件` → `Examples\Models\SurfaceTangents.vtk`
2. 模型树里选中它 → `算法处理 -> 表面切向量 (Generate Surface Tangents)`
3. 面板里保持默认（勾「输出点切向量 Tangents」），点「应用」
4. 模型树里出现 `SurfaceTangents_tangents`，点一下它让它成为当前模型
5. 点工具栏「向量 / Glyph」→ 左侧「向量」面板 → 向量名选 `Tangents` → 点「绘制」
   → 画面上出现一圈沿圆周方向的箭头
6. 想看数值：在「查找信息」面板里选 `Tangents`，能看到 39 个点的 (x, y, z)，都是单位向量
7. （可选）勾上「输出单元切向量」再跑一次，单元属性里也会多一份 `Tangents`

**ParaView 侧（同一个模型，做对照）**

1. `File -> Open` 打开同一个 `SurfaceTangents.vtk` → `Apply`
2. `Filters -> Search` 里输入 `Surface Tangents`（就是 VTK 的 vtkPolyDataTangents）→ `Apply`
   - 面板里 `Input` 选 `TCoords`，`ComputeCellTangents` 保持不勾，和 iGameVis 默认一致
3. `Filters -> Glyph`：
   - `Orientation Array` 选 `Tangents`，`Scale Array` 选 `Tangents`（或改成 `No scale array`）
   - `Scale Factor` 调小到箭头长短合适，`Apply`
4. 两个窗口里的箭头方向、个数一致；数值上最大差 1e-7（float32 精度）

> 录屏小提示：iGameVis 的箭头大小/间隔在「向量」面板里调；ParaView 的箭头大小用 Glyph 的
> `Scale Factor` 和 `Glyph Mode` 调，两边调到看起来差不多大，对比最直观。

## 6. 和 ParaView 对得上吗

对得上，而且是逐点对过数的：

1. **不规则 16 点网格**（测试场景 3）：先用 `pvpython` 跑 `vtkPolyDataTangents` 拿到
   参考值，写死在测试里，我们算出来的最大差是 `6e-8`（float32 精度）。
2. **圆柱演示模型的 39 个点**（测试场景 10）：测试会打印
   `demo tangent[i] = (x, y, z)`，和 `参考值_生成与核对_ParaView.py` 打印的同一批数字
   逐点对比，最大差 `1e-7`。
3. **累加权重**（测试场景 2）：专门构造了能区分"等权 / 面积加权"的例子，确认和 ParaView 一致。

复现 ParaView 参考值（需要本机装了 ParaView，比如 `D:\paraview`）：

```powershell
D:\paraview\bin\pvpython.exe 参考值_生成与核对_ParaView.py
```

### 和 VTK 有意的两处差异

| 情况 | VTK `vtkPolyDataTangents` | 本 filter |
|---|---|---|
| 输入里有四边形 / 多边形 | 直接报错：`This filter only supports triangles` | 内部按扇形三角化，正常算出来 |
| 三角形的纹理坐标退化（uv 共线） | 输出 NaN / Inf | 跳过该三角形，其余点照常算 |

除此之外，点切向量的数值和 ParaView 是一致的（测试里对过数）。

## 7. 演示模型

`Examples\Models\SurfaceTangents.vtk`：半径 1 的圆柱面，12 段 × 3 圈。

- 39 个点：13 列（第 13 列是接缝处复制的一份，u 从 0 到 1）× 3 圈（z = 0, 1, 2）
- 48 个三角形
- `TEXTURE_COORDINATES TCoords 2 float`：u 沿圆周（0 到 1），v 沿高度（0 到 1）
- `NORMALS Normals float`：径向法向量（本 filter 不用它，只是让模型更真实）

为什么把接缝那一列复制一份？因为如果第 12 列直接接回第 0 列，u 会从 `0.9167` 突然跳回 `0`，
跨接缝的那几个三角形的 `dP/du` 会被这个跳变带歪（ParaView 和我们都一样）。
真实模型里 UV 接缝处都是复制顶点的，所以演示模型也这么建。

期望结果：切向量沿着"绕圆周"的方向（`u` 增大的方向）。靠接缝的两列只有单侧邻居，
会差到 15°（`cos15° = 0.966`）；其余点基本就是解析方向。

## 8. 注意事项

- **必须有纹理坐标**：本 filter 根据「表面几何 + 纹理坐标」算切向量，模型里没有 `IG_TCOORD`
  点属性时会直接失败并给出提示；先用「球面纹理坐标 (Texture Map To Sphere)」之类的 filter
  生成一份 uv，或者读一个自带 `TEXTURE_COORDINATES` 的模型。
- **不需要法向量**：输入有没有 `Normals` 都不影响结果（ParaView 的 vtkPolyDataTangents 同样不用法向量）。
- **只针对表面**：体网格（VolumeMesh / StructuredMesh）和纯点集不支持，体网格请先做
  「转换为表面网格 (Convert To Surface Mesh)」。
- **不修改输入**：结果写在输入的副本上（输出节点名 `<输入名>_tangents`），所以可以对别的
  filter 的输出继续算切向量。
- **四边形 / 多边形也可以**：内部按扇形三角化（VTK 遇到非三角形单元会直接报错）。
- **纹理坐标退化的三角形会被跳过**：同一个三角形的三个点 uv 共线时 `1/den` 会爆掉，
  这里直接跳过该三角形（VTK 在这种单元上会输出 NaN）。
- **uv 接缝处要复制顶点**：如果一圈模型的 uv 在接缝处从 `0.9167` 直接跳回 `0`，
  跨接缝那几个三角形的切向量会被这个跳变带歪（ParaView 也一样）。正确做法是接缝处
  复制一列顶点（演示模型就是这么建的）。
- **切向量方向 = 纹理 u 增大的方向**：如果你觉得箭头方向"反了"，先检查模型的 uv 是不是反的，
  而不是去改 filter。

## 9. 常见问题

- **菜单里看不到「表面切向量」**：说明 `igQtMainWindow.cpp` 没改成功或没重新编译 `iGameVis`
  目标。重新跑一遍安装脚本，再 `cmake --build build --config Release --target iGameVis --parallel`。
- **提示"输入里没有纹理坐标点属性"**：模型确实没有 `IG_TCOORD`。先做一次
  「球面纹理坐标 (Texture Map To Sphere)」，或者读一个自带 `TEXTURE_COORDINATES` 的模型。
- **提示"不支持的输入类型"**：输入是体网格（VolumeMesh / StructuredMesh）或纯点集。
  体网格请先做「转换为表面网格 (Convert To Surface Mesh)」。
- **提示"输入里没有三角形/四边形/多边形单元"**：模型只有线单元或点（比如点云）。
- **提示"所有三角形的纹理坐标都退化"**：纹理坐标在同一个三角形内是常数 / 共线的，
  这种 uv 没有"u 方向"可言，换一份纹理坐标。
- **编译报 `LNK2038 RuntimeLibrary 不匹配` / `LNK2001 __imp__invalid_parameter`**：
  Debug 和 Release 混用了。删掉 `F:\igamevis\build\iGameCore.lib`，全部按 `--config Release` 重编。
- **控制台中文乱码**：先执行 `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8`。
