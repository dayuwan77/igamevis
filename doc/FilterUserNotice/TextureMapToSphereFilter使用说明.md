# 球面纹理坐标 TextureMapToSphere 使用说明

> 对应需求：`texture_map_to_sphere` —— 生成球面纹理坐标，参数是**球心**，实现方式是**逐点映射**。
> 对应 ParaView / VTK 里的 `Texture Map To Sphere`（VTK 类 `vtkTextureMapToSphere`）。

## 一、功能

给模型里**每个点**算一对球面纹理坐标 `(s, t)`，写成点属性：

- 数组类型：`IG_TCOORD`（纹理坐标），数组分量：2
- 数组默认名字：`TextureCoordinates`（可以用 `SetOutputArrayName()` 改）
- 内容是 `0 ~ 1` 的归一化坐标：`s` 是经度方向，`t` 是纬度方向
- 支持的输入：点集（PointSet）/ 表面网格（SurfaceMesh）/ 体网格（VolumeMesh）/
  非结构网格（UnstructuredMesh）
- **逐点映射**：每个点只看自己相对球心的位置，不看邻居、不看拓扑，计算量 O(点数)
- **不修改输入**：输出是输入的深拷贝，只多一份纹理坐标数组，输出节点名 `<输入名>_texcoords`
  （这样才能对别的 filter 的输出继续处理）
- **默认参数和 VTK / ParaView 保持一致**：球心自动取包围盒中心、`PreventSeam` 打开

球面纹理坐标是"给球状模型贴一张图"最常用的参数化方式；它也是
「表面切向量 (Generate Surface Tangents)」的前置数据。

## 二、参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| 球心 (X, Y, Z) | 输入包围盒中心 | 球面映射的球心；决定极点、赤道、0 度经线在哪 |
| 自动使用包围盒中心 | 开 | 勾上就忽略手工填的球心，直接用包围盒中心（= VTK 的 `AutomaticSphereGeneration`） |
| 防止接缝 PreventSeam | 开 | 和 VTK / ParaView 默认一致；打开时用 `acos` 展开（背面镜像、没有接缝），关掉时是真正的经度（绕一圈 0~1） |
| 输出数组名 | `TextureCoordinates` | 写在点属性上的数组名字 |

> 一个界面上的事实：ParaView 的 `Texture Map To Sphere` 面板里**只暴露 `PreventSeam`**，
> 球心固定用自动包围盒中心；想自己指定球心必须用 VTK 类。iGameVis 这边两种都能设。

## 三、算法（逐点映射）

设点为 `p`、球心为 `c`，`d = p - c`：

```
rho = sqrt(dx^2 + dy^2)         点到旋转轴的距离
r   = sqrt(rho^2 + dz^2)        点到球心的距离
```

**纬度方向 `t`**（两个分支相同）：

```
t = acos(dz / r) / pi           +z 一端 t = 0，-z 一端 t = 1，赤道 t = 0.5
```

**经度方向 `s`**：`vtkTextureMapToSphere` 在 `PreventSeam` 开 / 关时用的是**两套不同公式**，
本 filter 与它逐分支对齐：

```
PreventSeam 开（VTK 默认）：s = acos(dx / rho) / pi
PreventSeam 关：            s = atan2(dy, dx) / (2 * pi)      （负值再 +1）
```

两者的区别就是"怎么处理经度 ±180 度那条接缝"：

- `PreventSeam = 开`：`acos` 把经度**折叠**到 `[0, 1]`，背面的纹理是镜像的，但跨接缝的三角形
  不会出现"从 0.997 插值到 0.003 绕一整圈"的瑕疵 —— 这就是 VTK 用折叠换连续的做法；
- `PreventSeam = 关`：得到的是真正的经度（绕一圈 `0 ~ 1`），纹理不会镜像，但接缝处的三角形
  会出现绕圈插值。真实模型里要在接缝处复制一列顶点才能用这一支。

**两个退化情况**（和 VTK 的取值一致）：

- 点正好落在旋转轴上（`rho = 0`）：`s` 在 VTK 里没有定义，实测取值是
  「`+z` 方向 → 0；`-z` 方向 → 开接缝时 0.5、关接缝时 0.25」，本 filter 照做；
- 点正好落在球心上（`r = 0`）：`t = 0`。

一处**有意的稳健性差异**：VTK 直接算 `acos(dx / rho)`，浮点误差让 `|dx/rho|` 略大于 1 时
会得到 `NaN`；本 filter 先把比值夹到 `[-1, 1]` 再算 `acos`，不会产生 `NaN`。

## 四、调用方式

### 4.1 图形界面（iGameVis）

1. 打开 `F:\igamevis\build\Release\iGameVis.exe`
2. `文件 -> 打开文件`，选 `F:\igamevis\Examples\Models\TextureMapSphere.vtk`
3. 在模型树里点一下这个模型，让它成为当前模型
4. 菜单 `算法处理`（`ui->menu_filters`）→ `球面纹理坐标 (Texture Map To Sphere)`
5. 面板里：
   - `自动使用包围盒中心`（默认勾选）
   - `球心 X` / `球心 Y` / `球心 Z`（默认值就是当前模型的包围盒中心）
   - `防止接缝 (PreventSeam)`（默认勾选，和 ParaView 一致）
6. 点「应用」，模型树里多出一个新节点 `TextureMapSphere_texcoords`，原模型不动
7. 在「查找信息」面板里可以看到点属性 `TextureCoordinates`（2 个分量）

> iGameVis 现在的渲染管线还没有"贴一张纹理图"的功能，所以**画面本身不会变化**，
> 结果体现在数据上（属性面板 / 导出文件 / 测试程序）。对照"贴图效果"请看 ParaView（见第五节）。

### 4.2 C++ 调用

```cpp
#include "TextureMapToSphere/iGameTextureMapToSphereFilter.h"

auto filter = iGame::TextureMapToSphereFilter::New();
filter->SetAutomaticCenter(true);        // 球心 = 包围盒中心（默认就是 true）
// filter->SetCenter(0.0, 0.0, 1.0);     // 或者手工指定球心（会自动关掉自动球心）
filter->SetPreventSeam(true);            // 默认 true，和 VTK / ParaView 一致
filter->SetOutputArrayName("TextureCoordinates");
filter->SetInput(0, dataObject);
if (filter->Execute()) {
    auto out = filter->GetOutput();      // 独立输出，输入不变
}
```

### 4.3 跑测试程序

```powershell
cd F:\igamevis
cmake --build build --config Release --target testTextureMapToSphere --parallel
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
F:\igamevis\build\Examples\Release\testTextureMapToSphere.exe
```

看到最后一行 `ALL TESTS PASSED`、退出码 0 就算通过（一共 9 组场景，其中包含与 ParaView
参考值逐点对比）。

## 五、使用示例

下面这条流程就是录屏里演示的流程，**用的是仓库自带的 `TextureMapSphere.vtk`，
不使用 `Tet_Plane.vtk`**。

**iGameVis 侧**

1. 打开 `iGameVis.exe` → `文件 -> 打开文件` → `Examples\Models\TextureMapSphere.vtk`
2. 模型树里选中它 → `算法处理 -> 球面纹理坐标 (Texture Map To Sphere)`
3. 面板里保持默认（自动球心 + PreventSeam 打开）→ 点「应用」
4. 模型树里出现 `TextureMapSphere_texcoords`，点一下它让它成为当前模型
5. 在「查找信息」里能看到 `TextureCoordinates`：演示模型 18 个点的 `(s, t)`，
   例如北极点 `(0, 0)`、赤道正 x 方向 `(0, 0.5)`、正 y 方向 `(0.5, 0.5)`、南极点 `(0.5, 1)`
6. 换成手工球心（把「自动使用包围盒中心」的勾去掉，球心填 `0 0 0.5`）再点「应用」，
   可以看到 `s`、`t` 会整体变化 —— 这就是"球心参数"的作用

**ParaView 侧（同一个模型，做对照）**

1. `File -> Open` 打开同一个 `TextureMapSphere.vtk` → `Apply`
2. `Filters -> Search` 里输入 `Texture Map To Sphere` → `Apply`
   - 面板里只有 `PreventSeam`（默认勾选），和 iGameVis 的默认一致
3. 想看**数值**：`View -> Spreadsheet View`，把 `Texture Map To Sphere1` 拖动到
   Spreadsheet View 里，就能看到 18 个点的 `Texture Coordinates[0]`（= s）、`[1]`（= t）
4. 想看**贴图效果**（iGameVis 暂时没有的能力）：先 `File -> Open` 一张贴图（png/jpg），
   再在 RenderView 左上角的显示属性里把 `Texture` 选成刚才那张图 ——
   球面上就会出现贴图，这时能直观看到 `s` 是经度、`t` 是纬度
5. 两个窗口里的数值逐点一致（见第六节），贴图方向也和 iGameVis 的 `(s, t)` 一一对应

> 录屏小提示：ParaView 里为了看清 `PreventSeam` 的作用，可以把它取消勾选再 `Apply`，
> 然后把贴图换成一圈明显的图案（比如带文字的图），接缝处的绕圈插值会很明显；
> iGameVis 侧只要在面板里取消勾选「防止接缝」再应用，数据上就能看到 `s` 从
> `[0, 0.125, 0.25, …]` 变成真正的经度序列。

## 六、和 ParaView 对得上吗

对得上，而且是**逐点对过数**的。

做法：用 ParaView 自带的 `vtkTextureMapToSphere`（通过 `pvpython`，见
`Examples/Filter/TextureMapToSphere/参考值_生成与核对_ParaView.py`）对同一个演示模型
算出参考值，再和本 filter 的结果逐点比较。
测试 `Test 9` 就是拿这批参考值当"标准答案"的：

| 配置 | 参考来源 | 结果 |
|---|---|---|
| 球心 (0,0,0)、PreventSeam 开（ParaView 默认） | `pvpython` 跑 `vtkTextureMapToSphere` | 18 个点全部一致（差 < 1e-5，float32 精度） |
| 球心 (0,0,0)、PreventSeam 关 | 同上 | 18 个点全部一致 |

复现参考值（需要本机装了 ParaView，比如 `D:\paraview`）：

```powershell
cd F:\igamevis\Examples\Filter\TextureMapToSphere
D:\paraview\bin\pvpython.exe 参考值_生成与核对_ParaView.py
```

### 和 VTK 的两处差异

| 情况 | VTK `vtkTextureMapToSphere` | 本 filter |
|---|---|---|
| `dx/rho` 的绝对值因浮点误差略大于 1 | `acos` 得到 `NaN` | 先把比值夹到 `[-1, 1]`，不会 NaN |
| 输出数组名 | VTK 9.x 叫 `Texture Coordinates`（带空格） | 叫 `TextureCoordinates`（和 iGameVis 其他 filter 的命名习惯一致） |

除此之外（含默认参数、极点取值）都和 ParaView 一致，测试里对过数。

## 七、演示模型

`Examples\Models\TextureMapSphere.vtk`：半径 1 的 UV 球，**18 个点、32 个三角形**。

- 北极 1 个点、南极 1 个点，中间两圈纬度（z = ±0.5）各 8 个点（每 45 度一个）
- 赤道方向的两个点正好落在 `+x`、`+y` 方向，方便肉眼核对 `s`、`t`
- 极点正好在旋转轴上，顺带覆盖 `rho = 0` 这个退化情况

**为什么不用 `Tet_Plane.vtk`**：它是一块平面网格，不是球状模型，球面映射在这种模型上
会退化（`rho`、`r` 的分布完全没有球面特征，看不出映射对不对），所以本 filter 的测试用例
和演示录屏都使用专门的 `TextureMapSphere.vtk`。

## 八、注意事项

- **`PreventSeam` 的语义要理解对**：打开（默认）时 `s` 是 `acos` 折叠值 —— 背面纹理是镜像的、
  但没有接缝；关掉时 `s` 是真正绕一圈的经度 —— 不镜像，但接缝处的三角形会绕圈插值。
  两种都和 VTK 一致，选哪种取决于你要"不镜像"还是"不接缝"。
- **`t` 的方向**：`t = acos(dz/r)/pi`，所以 `+z` 一端 `t = 0`、`-z` 一端 `t = 1`，
  和"从南极开始"的直觉相反 —— 这是 VTK 的定义，保持一致才能和 ParaView 对上。
- **球心很关键**：同一份几何换个球心，纹理坐标完全不同；模型不是球状时也照样按公式映射，
  只是贴图效果不一定好看（本 filter 是纯几何映射，不做形状假设）。
- **不修改输入**：结果写在输入的深拷贝上（节点名 `<输入名>_texcoords`），可以连续叠加其它 filter。
- **iGameVis 现在不渲染纹理**：结果体现在点属性上（「查找信息」面板 / 导出文件 / 测试程序）；
  想看贴图效果去 ParaView 对照。
- **别用 `Tet_Plane.vtk` 做这个 filter 的测试模型**（平面的球面参数化没有意义）。
- **编译报 `LNK2038 RuntimeLibrary 不匹配`**：Debug / Release 混用了。删掉
  `F:\igamevis\build\iGameCore.lib`，全部按 `--config Release` 重编。
- **注意重新编译**：改完 `igQtMainWindow.cpp` 后必须重编 `iGameVis` 目标，菜单才会出现。

## 九、常见问题

- **菜单里看不到「球面纹理坐标」**：`igQtMainWindow.cpp` 没改成功，或者没重新编译 `iGameVis`。
  重新跑一遍安装脚本，再 `cmake --build build --config Release --target iGameVis --parallel`。
- **应用后画面没变化**：这是正常的，iGameVis 目前不渲染纹理，结果在数据里（见第八节）。
- **提示"当前模型没有点数据"**：输入数据里没有点（例如只有属性没有几何）。
- **提示"不支持的网格类型"**：输入不是点集 / 表面网格 / 体网格 / 非结构网格。
- **数值和 ParaView 差得比较多**：先确认三件事 ——（1）球心是否相同（ParaView 面板里球心
  固定是包围盒中心）；（2）`PreventSeam` 是否相同；（3）是不是在看同一个点（Spreadsheet 的
  行号和 iGameVis 的编号可能因模型点序不同而不同）。
- **控制台中文乱码**：先执行 `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8`。
