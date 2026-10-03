# TextureMapToCylinderFilter 使用说明

对应任务：简单任务 —— 按圆柱面为网格的每个点生成 2D 纹理坐标（`texture_map_to_cylinder`）。

## 1. 功能

`TextureMapToCylinderFilter` 把一个圆柱面作为纹理空间，为输入网格的**每个点**计算一对纹理坐标
`(s, t)`，并以 **Point Data** 的形式写入一个**独立输出的非结构化网格**：

| 输出属性名 | 类型 | 挂载位置 | 含义 |
| --- | --- | --- | --- |
| `Texture Coordinates` | FloatArray（2 分量） | 点（point） | `s` = 绕圆柱轴的角度映射，`t` = 沿轴的归一化位置 |

**几何、拓扑、其余属性一律不变**：该 Filter 只给点数据追加一个数组，不改动任何已有数据。

### 1.1 计算公式

设圆柱轴为 `Point1 → Point2`（`axis = Point2 - Point1`，长度记为 `L`），
`ref` 为与轴垂直的参考方向（由 `axis × (1,0,0)` 构造，轴与 X 平行时改用 `(0,1,0)`）：

| 坐标 | 公式 | 取值范围 |
| --- | --- | --- |
| `t`（轴向） | `t = ((p - Point1) · axis) / L²`，即点沿轴的归一化投影：`Point1` 处为 0、`Point2` 处为 1（**轴长影响 t**） | 一般在 [0,1]，超出轴段时可能略越界 |
| `s`（圆周） | 令 `v` = 点相对轴的**径向单位向量**（点减去它在轴所在直线上的垂足后归一化），`thetaX = acos(v · ref)`，`thetaY = axis · (ref × v)`<br>`PreventSeam = true`（默认）：`s = thetaX / π`<br>`PreventSeam = false`：`s = thetaX / (2π)`，若 `thetaY < 0` 则 `s = 1 - s` | [0,1] |

`PreventSeam = true` 时 `s` 绕轴一周会先 `0→1`（0°~180°）再 `1→0`（180°~360°），
用于避免纹理在接缝处出现跳变；`false` 时整圈单调 `0→1`。

### 1.2 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `SetAutomaticCylinderGeneration(bool)` | `true` | 自动求轴 |
| `SetPoint1(x,y,z)` / `SetPoint2(x,y,z)` | `(0,0,-1)` / `(0,0,1)` | 手动指定轴的两个端点。默认取覆盖常见单位模型（±1）的长度，使不手动设轴时 `t` 落在 [0,1] 内 |
| `SetPreventSeam(bool)` | `true` | 是否防止接缝（见上表） |
| `SetTCoordsArrayName(name)` | `"Texture Coordinates"` | 输出数组名 |

## 2. 支持的数据类型

| 类型 | 说明 |
| --- | --- |
| `UnstructuredMesh` | 直接处理 |
| `SurfaceMesh` | 内部统一转成 UnstructuredMesh 表示后处理 |
| `VolumeMesh` | 同上（纹理坐标只关心点，体网格同样适用） |

以下情况返回 `false`，原因见 `GetMessage()`（**绝不把原模型当结果返回**）：

| 情况 | 提示信息 |
| --- | --- |
| 没有输入 / 输入为空指针 | `no input data` / `input data is null` |
| 类型不支持 | `unsupported data type (support: UnstructuredMesh / SurfaceMesh / VolumeMesh)` |
| 输入 0 个点 | `input mesh has no points, there is no texture coordinate to generate` |
| 轴退化（`Point1 == Point2`） | `degenerate cylinder axis (Point1 and Point2 coincide)` |
| 自动求轴失败（点云在主轴上没有延展） | `cannot determine a cylinder axis automatically (degenerate point cloud); ...` |

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个网格模型，在模型树中选中它；
2. 主菜单 **算法处理 → 圆柱纹理坐标 (Texture Map To Cylinder)**，打开左侧面板；
3. 勾选/取消 **自动求轴**（开启时轴的两个端点输入框自动禁用），按需修改 **防止接缝**；
4. 点 **执行**：结果作为独立节点（`<原名>_TCoords`）加入模型树，
   面板下方会显示本次写出的数组名与**实际使用的轴端点**（自动求轴时是算出来的结果，可用于核对）。

### 3.2 代码方式

```cpp
#include <TextureMapToCylinder/iGameTextureMapToCylinderFilter.h>

auto filter = iGame::TextureMapToCylinderFilter::New();
filter->SetAutomaticCylinderGeneration(false);   // 手动指定轴（结果完全确定）
filter->SetPoint1(0.0, 0.0, -1.0);
filter->SetPoint2(0.0, 0.0,  1.0);
filter->SetPreventSeam(true);
filter->SetInput(mesh);
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << std::endl;   // 失败原因
} else {
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // out 的点数据里多了一个 "Texture Coordinates"（2 分量）
}
```

## 4. 使用示例

模型 `Examples/Models/TextureMapToCylinder_tube.vtk`：半径 0.5 的圆管，
轴向 `z = -1 / 0 / +1` 各 8 个点（24 点 / 16 个四边形），并带 `p_val`（点）/ `c_val`（单元）两个数组。

| 场景 | 期望结果 |
| --- | --- |
| 默认（自动求轴 + PreventSeam） | 轴解得为 Z 轴两端的 `(0,0,∓1)`；三圈的 `t` 为 `0 / 0.5 / 1`；同一圈按 45° 步长的 `s` 为 `0, 0.25, 0.5, 0.75, 1, 0.75, 0.5, 0.25` |
| `PreventSeam = false` | `s` 绕整圈单调覆盖 `0, 0.125, 0.25, …, 0.875` |
| 手动轴 `(0,0,-1) → (0,0,1)` | `t` 严格为 `0 / 0.5 / 1`，各圈同角度的 `s` 完全一致 |
| 轴退化 / 0 个点 | 返回 `false` 并给出原因 |

`p_val` / `c_val` 在执行后必须原样保留（长度分别 24 / 16），输入模型不得被改动。

## 5. 注意事项

1. **自动求轴用的是主成分分析（PCA）**：对轴对齐网格与圆柱 / 管类模型能稳定给出轴向；
   各向同性模型（立方体、球体）主轴不唯一，此时建议关闭自动、手动指定轴。
2. **轴的正负不保证朝向**：自动求轴时轴的方向由主轴特征向量的符号决定，
   因此 `t` 的正负（以及 `PreventSeam = false` 时 `s` 的绕行方向）可能整体反向。
   需要完全确定的坐标朝向时必须手动指定 `Point1` / `Point2`。
3. **落在 0° 位置的点**可能因浮点零符号被映射到 `s = 1`（而不是 `0`）—— 两者在纹理上等价，
   不影响映射结果。
4. **iGameVis 目前没有网格贴图渲染通路**，纹理坐标只能在属性面板里查看数值
   （在「数据查找」中按分量查看 `Texture Coordinates` 的 `s` / `t`）。
5. **导出为 `.vtk` 时的两个框架约定**（不是本 Filter 的行为，所有 Filter 都受影响）：
   - 数组名中的空格会被百分号编码：`Texture Coordinates` → `Texture%20Coordinates`；
   - 导出模块只写 float/double，且没有 `IG_TCOORD` 分支，纹理坐标会被写成 `SCALARS` 而非
     `TEXTURE_COORDINATES`。
6. 重复执行不会堆积同名数组（写结果前先删同名点数组）。

## 6. 测试

```bash
cd Examples
./testTextureMapToCylinder
```

覆盖：默认参数（自动求轴 + 接缝）、`PreventSeam = false`、手动轴（逐点核对 `s`/`t`）、
失败场景（轴退化、0 个点）、重复执行；并校验输入模型与其余属性不被改动。断言全部通过才输出 `PASS`。
