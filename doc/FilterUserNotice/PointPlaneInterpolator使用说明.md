# PointPlaneInterpolator（点平面插值）使用说明

> - 头文件：`PointPlaneInterpolator/iGamePointPlaneInterpolatorFilter.h`
> - 界面入口：菜单 **算法处理 → 开发中filter → 新增filter/第二批 → 点平面插值 (Point Plane Interpolator)**
> - 界面控件：默认嵌入左侧「工具面板」的 Tab 页签中，包含「平面参数」与「插值设置」两个分组框
> - 自动测试示例：`Examples/Filter/PointPlaneInterpolator/TestPointPlaneInterpolator.cpp`（CMake 目标 `testPointPlaneInterpolator`）
> - 配套测试模型：`Examples/Models/analysis_vtu_subcase1.vtu`

---

## 一、功能

基于散乱点云或网格模型，在指定的 3D 平面区域上进行空间距离插值，生成参数化的二维平面网格（并自动附带标量云图）。

- **深度对齐 ParaView**：支持三种核函数（Voronoi、Gaussian、Shepard）与两种邻域搜索策略（Radius、N Closest）。
- **3D 实时交互手柄**：在 3D 视图中提供红色的操作手柄（中心圆球用于平移，两端箭头用于旋转法向），手柄参数与右侧 UI 面板双向实时同步。
- **Apply 更新机制**：为了防止大网格计算卡顿，拖动平面时不会触发算法；只有点击「应用」后，才会更新云图。
- **原地更新**：对同一个输入模型多次应用，只会在模型树中维持一个 `_PlaneInterpolated` 结果节点，不会无限堆叠。
- **沉浸式生命周期**：面板关闭后，3D 视图左键会自动恢复为默认的旋转视角功能。

---

## 二、参数与接口

### 2.1 接口一览

| 接口 | 说明 |
| --- | --- |
| `SetOrigin(const Point& p)` | 设置平面的中心原点 |
| `SetNormal(const Point& n)` | 设置平面的法向量（内部自动归一化） |
| `SetResolution(int res)` | 平面分辨率（`N x N` 的网格，N 需 ≥ 2） |
| `SetKernelType(KernelType type)` | 插值核：`VORONOI` (默认)、`GAUSSIAN`、`SHEPARD` |
| `SetKernelFootprint(KernelFootprint footprint)` | 邻域搜索：`RADIUS` (默认)、`N_CLOSEST` |
| `SetRadius(double radius)` | Radius 搜索策略下的搜索半径 |
| `SetNumberOfPoints(int n)` | N-Closest 搜索策略下的邻点数 |
| `SetSharpness(double s)` | Gaussian 核的高斯锐度 |
| `SetPowerParameter(double p)` | Shepard 核的幂次 |
| `SetNullPointsStrategy(NullPointsStrategy strategy)` | 空点处理：`MASK_POINTS`、`NULL_VALUE`、`CLOSEST_POINT` |
| `bool Execute()` | 执行插值，成功返回 `true` |
| `DataObject::Pointer GetOutput()` | 获取输出模型（`UnstructuredMesh`） |

### 2.2 界面控件说明

| 控件 | 说明 |
| --- | --- |
| 平面参数：原点 X / Y / Z | 初始定位在模型包围盒中心 |
| 平面参数：法向 X / Y / Z | 初始值为 `1.0 / 0.0 / 0.0`（对齐底层手柄默认朝向） |
| 插值设置：分辨率 | 默认 `100`，生成 `100x100` 的四边形网格 |
| 插值设置：插值核 | 下拉选择，切换时会自动联动隐藏/显示下方相关参数 |
| 邻域搜索 / 半径 / 邻点数 | 仅在 `Gaussian` 或 `Shepard` 核下可见 |
| 应用 | 深灰底色的按钮，点击后触发算法并刷新模型树与 3D 视图 |

---

## 三、调用方式

### 3.1 代码调用

```cpp
#include <PointPlaneInterpolator/iGamePointPlaneInterpolatorFilter.h>
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"

auto input = iGame::FileIO::ReadFile("./Models/analysis_vtu_subcase1.vtu");
if (!input) return;

auto filter = iGame::PointPlaneInterpolatorFilter::New();
filter->SetInput(input);

// 设置平面参数
filter->SetOrigin(iGame::Point(0.0, 0.0, 0.0));
filter->SetNormal(iGame::Point(0.0, 0.0, 1.0));
filter->SetResolution(64);

// 设置插值参数
filter->SetKernelType(iGame::PointPlaneInterpolatorFilter::GAUSSIAN);
filter->SetKernelFootprint(iGame::PointPlaneInterpolatorFilter::RADIUS);
filter->SetRadius(100.0);
filter->SetSharpness(2.0);

if (!filter->Execute()) {
    std::cerr << "执行失败" << std::endl;
    return;
}

auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
std::cout << "输出点数：" << out->GetNumberOfPoints() << std::endl; // 64 * 64 = 4096
std::cout << "输出单元数：" << out->GetNumberOfCells() << std::endl; // 63 * 63 = 3969
```

### 3.2 GUI 交互操作

1. 加载模型，在模型树中选中它。
2. 点击菜单 **算法处理 → 开发中filter → 新增filter/第二批 → 点平面插值**。
3. 左侧工具栏将自动弹出「点平面插值」页签。
4. **3D 交互**：
   - 鼠标左键按住红色的中心球可任意平移平面。
   - 鼠标左键按住两端的红色箭头可自由旋转法向。
   - **提示**：在手柄激活期间，鼠标左键被独占用于拖拽；如需旋转整个 3D 视角，请使用**鼠标右键**拖动。
5. 在左侧面板调整插值参数后，点击 **直接应用**。模型树中会新增或更新一个名为 `<原模型名>_PlaneInterpolated` 的彩色网格。

---

## 四、输出说明

- **输出类型**：`UnstructuredMesh`。
- **网格拓扑**：输出为 `Quad`（四边形）单元。点数和单元数满足公式：`Points = res * res`，`Cells = (res - 1) * (res - 1)`。
- **属性传播**：输入模型中所有的点属性都会被插值到新的平面上。支持多分量（标量/向量/张量）。如果是 `Char/Int` 类型，会被提升为 `Float` 以保留插值精度。
- **尺寸自适应**：生成的平面大小由底层算法根据输入模型的包围盒在平面上的投影决定。

---

## 五、算法说明

1. **构建平面坐标系**：根据输入的法向量计算平面内的两个正交切向量 U 和 V。
2. **确定包围盒投影**：将输入模型的 8 个包围盒角点投影到 U 和 V 轴上，计算出平面的物理尺寸，并将其居中于 Origin。
3. **网格生成**：在确定的物理平面上生成 `N x N` 的规则四边形网格点。
4. **空间插值（KD-Tree）**：
   - 利用底层 `PointKdTree` 快速查找网格点周围邻域内的原始点云。
   - 根据选择的核心（Voronoi 取 1 个最近点，Gaussian/Shepard 取 Radius/N-Closest 内的多点）计算权重。
   - 对属性进行加权求和并写入输出网格。
5. **空点策略**：如果搜索范围内没有任何原始点，根据策略填充空值、掩码或强制寻找全局最近点。

---

## 六、示例程序与自动测试

测试程序路径：`Examples/Filter/TestPointPlaneInterpolator.cpp`。

该测试为无渲染窗口控制台自检程序，共包含三个核心测试模块：

| 场景 | 覆盖点 |
| --- | --- |
| **Test 1**: 基础参数与 Voronoi | 验证拓扑结构（4x4 网格生成 16 点 9 面）；验证平面法向坐标正确；验证改变分辨率后输出重建；验证 `StaticDisplacement` 多分量属性传递。 |
| **Test 2**: Gaussian 与 Shepard | 验证核函数切换是否正常执行，覆盖 `RADIUS` 搜索策略。 |
| **Test 3**: 空点策略与非法拦截 | 故意将平面置于极远处；测试 `MASK_POINTS` 掩码、`NULL_VALUE` 空值、`CLOSEST_POINT` 强制找最近点逻辑；验证 `Resolution < 2`、`Radius <= 0`、`Normal == 0` 等非法参数会被直接拦截并返回 `false`。 |

**运行方式**（需在 `cmake-build-examples` 目录下）：

```powershell
# 运行自动测试（无窗口）
set IGAME_EXAMPLE_NO_RENDER=1
Release\testPointPlaneInterpolator.exe

# 运行可视化验证（会弹窗展示）
set IGAME_EXAMPLE_NO_RENDER=
Release\testPointPlaneInterpolator.exe
```

---

## 七、注意事项

1. **交互绑定**：手柄操作激活时，左键会锁定为平移/旋转平面。关闭左侧 Tab 页签后，主窗口会自动重置交互器状态，左键恢复为旋转模型视角。
2. **法向归一化**：无论 UI 输入的法向长度是多少，在传入算法与 3D 手柄绘制前都会被严格归一化为单位向量。
3. **防重复生成**：为了保证 ParaView 一致的单输出体验，点击“直接应用”时，系统会在场景中全局搜索同名结果节点，覆盖数据更新而非新建节点。如果需要对比不同平面的结果，请在点击前先在模型树中重命名或移除上一次的结果。
4. **禁止零向量**：法向向量不能为 `(0, 0, 0)`，否则会导致平面无法构建，算法会返回失败。
5. **性能提醒**：如果原始模型点数极大（如千万级），且采用较高的分辨率（如 `1000x1000`），生成网格时会因为频繁的 KD-Tree 邻域查询导致计算耗时较长，建议从 `64x64` 开始调试。
6. **输入限制**：仅支持具有点集和网格的模型（如 `UnstructuredMesh`、`SurfaceMesh`）。如果输入是不带点的纯几何或空节点，滤镜会报错。