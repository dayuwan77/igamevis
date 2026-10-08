# 20 extract_selection 使用说明

## 功能

`iGame::ExtractSelectionFilter` 将当前模型中选中的点或单元提取成独立模型。
点模式输出 `PointSet`；单元模式输出 `UnstructuredMesh`，只保留被选单元使用的点，重建点编号和连接关系。
支持点集、表面网格、普通体网格和非结构化网格；非结构化网格中的多面体面流会保留。
复制对应的数值型点属性和单元属性，保留分量数及内存中的数值类型，并重新计算属性范围。
原模型、原选择和原属性保持不变。

## 调用方式

```cpp
#include "Selection/igameextractselectionfilter.h"
#include "iGameSelection.h"
#include <iostream>
#include <vector>

// mesh 为已加载的网格；本例选择编号为 0 的单元。
mesh->GetSelection()->SelectionCallBackEvent(
    IG_CELL, std::vector<igIndex>{0}, iGame::Selection::Add);
auto filter = iGame::ExtractSelectionFilter::New();
filter->SetInput(mesh);
filter->SetSelectionType(IG_CELL); // IG_POINT 为点模式，默认 IG_POINT
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << std::endl;
} else {
    auto result = filter->GetOutput();
}
```

## Qt 界面示例

1. 打开 `Examples/Models/ExtractSelectionTriangles.vtk`。
2. 点击工具栏“选择”，在面板勾选“开启”，对象类型选“面/体”，依据变量选“无”。
3. 当前界面默认使用包围盒框选。调整画面中的白色选择框，使它包住整个方形，确认两个三角形都出现紫红色高亮边线。可以通过选择框的红色控制点调整范围；“最大半径”不是切换为单击选择的开关。
4. 进入“算法处理 > 提取选中数据 (Extract Selection)”，选择“选中的单元”。
5. 模型树新增后缀 `_selected_cells` 的模型。它与原模型位置相同，隐藏原模型后可独立查看。

三角形输入有 5 个点、2 个三角形，其中一个点不被任何单元使用。
提取单个三角形应有 **3 点、1 单元**；提取两个三角形应有 **4 点、2 单元**。
`ExtractSelectionTetrahedra.vtk` 有 5 点、2 个共享面的四面体；两个全选时应有 **5 点、2 单元**。
因此，原模型统计与提取结果统计不一定相同。请点击结果模型本身查看统计。

### 三角形示例的数据组成

点编号从 0 开始，坐标如下：

| 原始点编号 | 坐标 | 用途 |
| --- | --- | --- |
| 0 | (0, 0, 0) | 三角形 0 |
| 1 | (1, 0, 0) | 两个三角形共用 |
| 2 | (0, 1, 0) | 两个三角形共用 |
| 3 | (1, 1, 0) | 三角形 1 |
| 4 | (2, 1, 0) | 不属于任何单元 |

单元 0 由点 0、1、2 组成；单元 1 由点 2、1、3 组成。
选中两个单元后，输出保留前四个点并移除闲置点 4，所以得到 4 点、2 单元。
模型中的 `temperature`、`velocity` 是点属性，`material` 是单元属性。

点模式示例：在选择面板将对象类型改为“点”，选择需要的点，再在提取对话框选择“选中的点”。
输出是独立点集，不包含原来的三角形。代码调用时对应使用 `IG_POINT` 选择并设置 `SetSelectionType(IG_POINT)`。

## 自动测试

从项目根目录配置 `-DEXTRACTION_FILTER_TESTS=ON`，然后构建：

```text
cmake --build build --config Release --target TestExtractSelection ExtractionExamples
ctest --test-dir build -C Release -R "Extract" --output-on-failure
```

`ExtractionExamples` 以项目根目录为工作目录，自动读取相对路径模型，不需要交互。
直接启动程序时，也应将工作目录设为项目根目录。输出保存在 `ExtractionResults/`。
`TestExtractSelection` 覆盖空输入、空选择、非法编号、独立复制、整数精度、向量属性、
共享顶点、表面/体网格、多面体及错误后清除旧输出。
测试模型为本次任务构造的人工模型，不使用 `Tet_Plane.vtk`。

## 对比验证

使用相同输入文件和相同单元编号分别运行 iGameVis 与 ParaView 的 Extract Selection。
除了查看几何形状，还应比较点数、单元数、点属性、单元属性及范围，并保留结果截图或录屏。

### ParaView 界面操作与本例差异

1. 在 ParaView 5.13.0 打开同一个 `ExtractSelectionTriangles.vtk`，点击 `Apply`。
2. 显示模式选 `Surface With Edges`。鼠标位于渲染画面时按 `S`，用左键拖动矩形框包住两个三角形，确认高亮。
3. 按 `Ctrl + Space` 搜索 `Extract Selection`，创建过滤器并点击 `Apply`；`Preserve Topology` 保持不勾选。
4. 隐藏原输入，显示并选中新建的 `ExtractSelection1`。将结果设为 `Surface With Edges`，打开 `Information` 查看统计。

本次实际验证结果：

| 项目 | iGameVis | ParaView 5.13.0 |
| --- | --- | --- |
| 输入 | 5 点、2 单元 | 5 点、2 单元 |
| 选择 | 单元 0、1 | 单元 0、1 |
| 输出 | 4 点、2 单元 | 5 点、2 单元 |
| 闲置点 (2, 1, 0) | 移除 | 保留 |

两个输出都包含这两个三角形，但本例的点列表、点属性范围和边界范围并非完全相同。
iGameVis 始终按选中单元重建所需点；本机 ParaView 在该全单元选择案例中仍保留闲置点。
例如输出 `temperature` 范围分别为 [10, 13] 与 [10, 14]，X 范围分别为 [0, 1] 与 [0, 2]。
这一差异需要在截图、视频及对比说明中如实保留，不应通过额外清理过滤器来冒充原生提取结果一致。
`ExtractionExamples` 的三角形案例选择单元 0，输出 3 点、1 单元，不等同于此处全选两个单元的视频案例。

## 注意事项

- `Execute()` 返回 `false` 时输出为空，调用 `GetLastError()` 获取原因。
- 点模式只复制点属性，不保留原单元；不是按选中点自动提取相邻单元。
- 不支持 `VolumeMesh` 内部专用多面体表示，请先转成 `UnstructuredMesh`。
- 不复制任意字段元数据、渲染设置、原模型的选择状态。
- 相比 VTK，不附加 `vtkOriginalPointIds` 和 `vtkOriginalCellIds`。
- 文件写出经过现有 VTKWriter；部分单元属性可能被写成 float。Filter 内存中的类型不变。
- ParaView 对比时使用同一输入、相同点/单元编号及 Extract Selection，不要用表面选择结果与体单元编号混比。
- 模型树的眼睛图标控制显隐，点击模型名称才会切换当前查看对象。录制结尾必须选中提取结果，不能用原输入的统计代替结果统计。
