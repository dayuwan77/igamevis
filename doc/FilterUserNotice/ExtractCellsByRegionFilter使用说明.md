# ExtractCellsByRegionFilter 使用说明

## 功能

从非结构化网格提取 Box（可旋转盒子）、Sphere（球体）或 Plane（平面半空间）内的完整单元，生成独立子网格。输出保留单元类型、共享点拓扑以及点属性和单元属性，原模型不变。

“全部顶点在内”勾选时保留全部顶点在区域内的单元；取消时保留至少一个顶点在区域内的单元。区域边界包含在内。宽松模式不会额外检测“所有顶点在外、只有边或面穿过区域”的单元。

Plane 保留 `(点 - 原点) · 法向 <= 0` 的一侧；点击“翻转法向”改变保留方向。提取的是完整单元，不会沿平面切开单元。

## 图形界面调用

1. 选中非结构化网格，打开“按区域提取单元”，选择 Box、Sphere 或 Plane。
2. 鼠标悬停在可操作组件上时高亮；左键拖动高亮组件。未命中组件时，左键继续旋转模型。区域组件不接管中键、右键或滚轮。
3. Box 复用 Transform 的交互：拖动红色面控制点调整局部尺寸，拖边或中心轴线平移，拖面旋转。参数显示中心和局部尺寸，数值修改保留当前旋转方向；提取区域与旋转后的组件一致。
4. Sphere 保留空心经纬线：拖中心点或线框平移，拖红色半径控制点改变半径。参数显示中心和半径。
5. Plane 显示包围盒、平面轮廓、原点、法向线两端的控制点和旋转环。拖原点平移，拖平面沿法向移动，拖法向两端的点或圆环旋转。端点以拾取深度反投影，直接跟随鼠标；悬停点变绿。可输入原点、法向，也可点击 X/Y/Z 法向按钮。
6. 调整组件、切换模式或修改参数时，仅更新组件与参数框，不遍历输入单元、不计算选择、不高亮单元。没有自动或手动单元预览；点击“执行”才计算提取结果。
7. 执行后窗口保持打开，再次执行更新同一个结果节点；空区域提示没有单元，不创建空结果，也不覆盖已有结果。关闭窗口清除组件并恢复原先的框选组件；原有单元选择在整个交互期间保持不变。

Box 的尺寸限制沿用 Transform / DynamicBox；Sphere 的最小半径为 `max(模型最大跨度 × 10⁻⁶, 10⁻⁷)`。控制点重叠或某轴接近视线时，可先旋转模型视角。

例如 BoxCases 模型：Box 中心设为 `(0,0,0)`，局部尺寸设为 `(2,2,2)`，未旋转时严格模式提取 1 个单元、宽松模式提取 3 个单元。拖动期间不计算单元数，执行后才显示数量。
## C++ 调用示例

```cpp
#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameFileIO.h>

auto input = iGame::FileIO::ReadFile("./Models/ExtractCellsByRegion_BoxCases.vtk");
auto filter = iGame::ExtractCellsByRegionFilter::New();
filter->SetInput(input);
filter->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
filter->SetRequireAllPoints(false); // ParaView: Extract Intersected = On
if (filter->Preview()) {
    const auto& inputCellIds = filter->GetSelectedCellIds();
    // 此模型的预期 ID 为 {0, 1, 3}。
}
if (filter->Execute()) {
    auto output = filter->GetOutput(); // 三个单元、十二个点
}
```

`Preview()` 只计算输入单元 ID，`Execute()` 用同一规则构建输出。改变区域或严格选项后，可从 C++ 显式调用 Preview 或 Execute；GUI 只在执行时计算。无效 Box（任一最小值大于最大值）或非正球半径会返回 `false`。

## 测试与 ParaView 对照

测试模型固定为 `Examples/Models/ExtractCellsByRegion_BoxCases.vtk` 和 `Examples/Models/ExtractCellsByRegion_SphereCases.vtk`，分别含 4、5 个四面体，覆盖完全在内、跨边界、完全在外、恰好落在边界，以及盒内但球外的单元。两份模型都内置 float 点属性 `PreviewPointValue` 和 int 单元属性 `MaterialId`。示例不再使用 `Tet_Plane.vtk`。

| 模型 | 区域 | 严格（ParaView 相交关闭） | 宽松（ParaView 相交开启） |
| --- | --- | --- | --- |
| BoxCases | Box `[-1,1]³` | `{0}` | `{0,1,3}` |
| BoxCases | Sphere 中心 `(0,0,0)` 半径 `1` | `{0}` | `{0,1,3}` |
| SphereCases | Box `[-1,1]³` | `{0,4}` | `{0,1,3,4}` |
| SphereCases | Sphere 中心 `(0,0,0)` 半径 `1` | `{0}` | `{0,1,3}` |

使用 ParaView 6.2 的 `pvpython` 从仓库根目录运行：

```powershell
pvpython Examples/Filter/Selection/ParaViewExtractCellsByRegionReference.py
```

脚本用 ParaView 原生 Filter 读取同一模型、设置 `Extraction Side = inside`，分别运行 Box/Sphere 和 `Extract Intersected` 开/关，直接断言原始单元 ID、点数与两类属性值。手工 GUI 复现时，打开模型，选择 `Filters → Extract Cells By Region`，设置相同参数并点 `Apply`；拖动其区域控件后再次点 `Apply`，对照选中单元与属性。

iGameVis C++ 示例目标为 `testExtractCellsByRegion`，构建后从 `Examples` 的构建目录运行。程序使用固定的 `./Models/...` 相对路径，无需输入参数。测试还校验预览与执行 ID 一致、连续修改参数后预览更新、空区域、无效参数和 float 点属性/int 单元属性的完整保留。

交互参考：Box 复用 iGameVis Transform 的 DynamicBox；Sphere 沿用已有经纬线组件；Plane 参考 VTK `vtkImplicitPlaneRepresentation` 的原点、法向旋转与沿法向推移。这里统一使用左键操作。界面回归覆盖新版基本交互、取消实时单元高亮及三种模式执行。

- [Box 控件事件](https://vtk.org/doc/nightly/html/classvtkBoxWidget2.html)
- [Sphere 控件事件](https://vtk.org/doc/nightly/html/classvtkSphereWidget2.html)
- [Box 9.7.1 源码](https://github.com/Kitware/VTK/blob/v9.7.1/Interaction/Widgets/vtkBoxRepresentation.cxx)
- [Sphere 9.7.1 源码](https://github.com/Kitware/VTK/blob/v9.7.1/Interaction/Widgets/vtkSphereRepresentation.cxx)

- [Plane 控件参考](https://vtk.org/doc/nightly/html/classvtkImplicitPlaneRepresentation.html)
