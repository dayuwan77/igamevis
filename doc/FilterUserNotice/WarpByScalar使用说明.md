# WarpByScalar（按标量变形）使用说明

> 过滤器名称：`WarpByScalar`（按标量变形 / Warp By Scalar）
> 头文件：`iGameCore/Filters/WarpByScalar/iGameWarpByScalarFilter.h`
> 公共辅助：`iGameCore/Filters/WarpByScalar/iGameWarpSupport.h`（`WarpSupport` 命名空间）
> 命名空间：`iGame`
> 参考实现：VTK `vtkWarpScalar` / ParaView「Warp By Scalar」

## 1. 功能

`WarpByScalar` 按点上的标量值沿指定方向移动点的位置，用于把标量场“拉伸”成可见的三维起伏（地形图、曲面抬升、变形场可视化等）。对每个点 `i` 计算：

```text
x_new[i] = x[i] + normal[i] * scalar[i] * ScaleFactor
```

其中 `normal[i]`（位移方向）的取法与 VTK 一致，按以下优先级确定：

| 优先级 | 条件          | 位移方向                                                     |
| ------ | ------------- | ------------------------------------------------------------ |
| 1      | `XYPlane` 打开 | 不使用法向，直接把 Z 分量**赋值**为 `scalar * ScaleFactor`（carpet plot 行为） |
| 2      | `UseNormal` 打开 | 使用用户通过 `SetNormal()` 指定的法向                        |
| 3      | 默认           | 优先使用输入的点法向数组（先找名为 `Normals` 的法向数组，再退回第一个 3 分量的法向点数组） |
| 4      | 法向缺失       | 退回用户指定的 `Normal` 实例变量（默认 `(0, 0, 1)`），并打印警告 |

数据处理规则：

| 数据 | 处理方式 |
| ---- | -------- |
| 点（坐标） | 复制输入点后逐点改写，输出点数为输入点数 |
| 点属性（`IG_POINT`） | 全部深拷贝到输出，类型、维度、数值保留（标量数组本身不会被修改） |
| 单元 / 单元属性 | 拓扑与单元属性原样复制，变形不改变连接关系 |
| 网格名称 | 继承输入网格名称 |

输入 / 输出：

- 输入：`PointSet` 派生类型，支持 `PointSet`、`SurfaceMesh`、`VolumeMesh`、`StructuredMesh`、`UnstructuredMesh`（Qt 面板会先按此范围校验）；
- 输出：与输入同一类型的新数据对象，只有点坐标不同，**不修改输入对象**。

## 2. 调用方式

### 2.1 C++ API 调用

```cpp
#include <WarpByScalar/iGameWarpByScalarFilter.h>

using namespace iGame;

// 1. 创建过滤器
auto filter = WarpByScalar::New();

// 2. 设置参数
filter->SetScalarsArrayName("height");  // 用于变形的点标量数组名（留空则取第一个点属性）
filter->SetScaleFactor(1.0);            // 缩放系数
filter->SetUseNormal(true);             // 是否强制使用下面的法向
filter->SetNormal(0.0, 0.0, 1.0);       // 法向（UseNormal 打开或输入没有点法向时使用）
filter->SetXYPlane(false);              // XY 平面模式（carpet plot）

// 3. 设置输入并执行
filter->SetInput(obj);                  // 等价于 filter->SetInput(0, obj)
if (!filter->Execute()) {
    // 失败处理：输入为空 / 非点集 / 找不到标量数组 / 标量数组长度不足
    return;
}

// 4. 取输出（新对象，输入未被修改）
auto output = filter->GetOutput();
```

参数一览：

| 参数 | 设置接口 | 读取接口 | 默认值 | 说明 |
| ---- | -------- | -------- | ------ | ---- |
| 标量数组名 | `SetScalarsArrayName(const std::string&)` | `GetScalarsArrayName()` | `""` | 为空时自动取第一个点属性；只使用该数组的**第 0 个分量** |
| 缩放系数 | `SetScaleFactor(double)` | `GetScaleFactor()` | `1.0` | 位移 = `法向 × 标量值 × 缩放系数` |
| 使用指定法向 | `SetUseNormal(bool)` | `GetUseNormal()` | `false` | 打开后忽略输入的点法向 |
| 法向 | `SetNormal(double nx, double ny, double nz)` | `GetNormal()` | `(0, 0, 1)` | 仅在 `UseNormal` 打开或输入没有点法向时生效 |
| XY 平面模式 | `SetXYPlane(bool)` | `GetXYPlane()` | `false` | 打开后忽略法向，直接把 Z 设为 `标量 × 缩放系数` |

执行逻辑：`Execute()` 先校验输入与标量数组，再按上面规则确定位移方向，随后按输入类型复制出一份输出（拓扑、属性深拷贝），最后逐点改写坐标并调用 `ForceReConvertToDrawableData()` 刷新绘制数据。

### 2.2 Qt 界面调用

1. 在模型树中选中一个模型（点集 / 表面网格 / 体网格 / 非结构网格 / 结构网格）；
2. 菜单：**算法处理 → 按标量变形 (Warp By Scalar)**；
3. 在弹出面板中设置参数，点击应用。

面板参数：

| 参数 | 默认值 | 说明 |
| ---- | ------ | ---- |
| 标量数组 (Scalars) | 第一个点属性 | 下拉框列出当前模型的全部点属性数组，显示为「名称 (类型, N 分量)」 |
| 缩放系数 (Scale Factor) | `1.0` | 必须是数值 |
| 使用指定法向 (Use Normal) | 不勾选 | 勾选后使用下面的法向 |
| 法向 X / Y / Z (Normal) | `0.0 / 0.0 / 1.0` | 必须是数值；未勾选「使用指定法向」且模型带点法向时被忽略 |
| XY 平面模式 (XY Plane) | 不勾选 | 勾选后忽略法向，Z 直接取标量值 |

执行成功后，结果以「算法」节点加入模型树，名称为 `<原模型名>_warp_scalar`，并刷新渲染窗口。

## 3. 使用示例

### 3.1 完整测试示例

示例程序：`Examples/Filter/WarpByScalar/TestWarpByScalar.cpp`
注册位置：`Examples/CMakeLists.txt` 中 `igame_add_example(testWarpByScalar Filter/WarpByScalar/TestWarpByScalar.cpp)`

测试模型：`Examples/Models/warp_grid3d_normals.vtk`

```text
DATASET STRUCTURED_GRID
DIMENSIONS 21 21 3          → 1323 个点
POINT_DATA 1323
  SCALARS height float 1     （用于变形的标量）
  SCALARS ptid float 1       （点编号）
  NORMALS pt_normals float   （点法向数组）
  SCALARS region float 1     （分区编号）
```

示例代码：

```cpp
#include <WarpByScalar/iGameWarpByScalarFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <string>

int main() {
    auto scene = iGame::Scene::New();

    const std::string fileName = "./Models/warp_grid3d_normals.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        return 1;
    }

    auto filter = iGame::WarpByScalar::New();
    filter->SetScalarsArrayName("height");  // 按 height 标量变形
    filter->SetScaleFactor(1.0);            // 位移 = height × 1.0
    filter->SetUseNormal(true);             // 强制使用 (0, 0, 1)
    filter->SetNormal(0.0, 0.0, 1.0);
    filter->SetXYPlane(false);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        return 0;
    }

    obj = filter->GetOutput();
    scene->AddModel(obj);

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
}
```

该示例的预期行为：

- 输入 1323 个点，输出同样 1323 个点，单元连接与全部点属性（`height` / `ptid` / `pt_normals` / `region`）保持一致；
- 每个点沿 `(0, 0, 1)` 方向移动 `height` 的距离，原本平坦的 21×21 平面网格被抬升成三维起伏曲面；
- 示例用相对路径读取 `./Models/warp_grid3d_normals.vtk`，无需运行时手动输入路径。源码目录 `Examples/Models` 会由 `iGameCopyExampleAssets` 目标拷贝到构建目录 `build/Examples/Models`，因此执行时的工作目录应是包含 `Models` 子目录的构建目录（CTest 即用 `WORKING_DIRECTORY build/Examples` 运行该示例）。

编译与运行（Windows / MSVC 为例，`build` 为实际构建目录）：

```powershell
cmake --build build --config Release --target testWarpByScalar --parallel
# 在包含 Models 子目录的构建目录（如 build/Examples）下执行
.\Release\testWarpByScalar.exe
```

### 3.2 取消勾选「使用指定法向」

把 `SetUseNormal(false)`、`SetXYPlane(false)` 后重新执行，位移方向改为取用模型自带的点法向数组 `pt_normals`（模型中全部为 `(1, 0, 0)`），此时网格沿 X 方向被拉伸，可与 3.1 的结果对照。

### 3.3 XY 平面模式（carpet plot）

```cpp
filter->SetScalarsArrayName("height");
filter->SetScaleFactor(1.0);
filter->SetXYPlane(true);   // 忽略法向，Z = height × 1.0
```

此时原始点的 Z 坐标被**直接覆盖**为标量值，适合把二维标量场（如 `height`）直接当作高度显示。

## 4. 注意事项

1. **标量数组必须挂在点上**：查找只在 `IG_POINT` 属性中进行，单元属性（`IG_CELL`）不能作为变形标量；Qt 面板也只列出点属性数组。若模型只有单元标量，可先用「转换为点数据」把属性转到点上。
2. **数组名与自动选择**：`SetScalarsArrayName("")`（或界面未选择）时取**第一个点属性**，此时数组的类型与分量数都不受限制，但取到的可能不是期望的标量，建议显式指定名字。
3. **只使用第 0 个分量**：标量数组不限制分量数（与 VTK 一致），向量 / 张量数组也能被选中，但变形只读取第 0 个分量，其余分量不参与计算。
4. **数组长度必须足够**：`Execute()` 要求标量数组元素个数 ≥ 点数，否则直接失败返回 `false`，不会做部分变形。
5. **输入必须是点集派生类型**：`PointSet` / `SurfaceMesh` / `VolumeMesh` / `StructuredMesh` / `UnstructuredMesh` 之外的输入会导致 `Execute()` 返回 `false`；输入为空、非 `PointSet`、点数为 0 同样失败。
6. **输出与输入分离**：输出是按输入类型新创建的副本（点、拓扑、属性均深拷贝，`StructuredMesh` 会调用 `GenStructuredCellConnectivities()` 补齐单元），因此变形不污染管线中的原始数据，也不改变单元的连接关系；结果须通过 `GetOutput()` 获取。
7. **XYPlane 是赋值而不是叠加**：`XYPlane` 打开时执行 `p[2] = scalar * ScaleFactor`，原始 Z 坐标会被丢弃；需要“在原有高度上继续抬升”时应使用 `UseNormal(true)` + `SetNormal(0, 0, 1)`（加法）。
8. **参数优先级**：`XYPlane` > `UseNormal` > 输入点法向 > `Normal` 实例变量。勾选 `XYPlane` 时界面上的法向参数完全不生效。
9. **法向应预先归一化**：位移量按 `normal × scalar × ScaleFactor` 计算，程序不做归一化；法向长度为 2 时位移也会翻倍。数值法向建议写成单位向量。
10. **缺少点法向会退化并警告**：输入没有 3 分量点法向数组（或该数组元素数少于点数）时，过滤器会打印 `no point normals available, using the Normal instance variable ...` 的警告并退回 `Normal` 实例变量（默认 `(0, 0, 1)`），结果可能不是预期的方向。
11. **缩放系数**：`ScaleFactor = 0` 时输出与输入坐标相同；负值会使变形方向相反；数值过大可能让模型大幅超出相机范围，可配合「重置视图」查看。
12. **标量值本身不会被改写**：变形只改坐标，`height` 等标量数组在输出中保持原值，可继续用于着色或后续算法。
13. **坐标精度**：点坐标以 `float` 存储，位移在 `float` 上累加，极小标量（如 1e-8 量级）× 小缩放的位移可能因精度而不可见。
14. **界面重复应用**：面板每次点击应用都是基于打开面板时选中的输入对象重新执行一次，结果会再添加一个 `<原名>_warp_scalar` 节点，**不会在上一次结果上累加**；需要叠加变形时，请对上一次的输出模型再次执行。
15. **日志**：执行开始 / 结束（点数、数组名、缩放系数）走 `IGAME_CORE_INFO`，失败原因（输入为空、非点集、找不到数组、数组长度不足、类型不支持、输出无点）走 `igError`，可在日志中定位问题。
