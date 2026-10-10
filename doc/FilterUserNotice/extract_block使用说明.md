# 29 extract_block 使用说明

## 功能

`iGame::ExtractBlockFilter` 从多块模型中按零起始索引路径或唯一名称提取一个块或子树。
输出独立深拷贝，原模型的层级、几何、连接关系、数值属性不变。
支持点集、表面网格、普通体网格、非结构化网格，以及框架现有子对象接口和
`CompositeDataObject` 子节点接口。叶块保留相应网格类型，容器输出带子对象的 `DrawObject`。
复制支持的数值点/单元属性，保留内存类型和分量数，重新计算属性范围。
本实现不依赖 20 的 Extract Selection，属性复制工具位于本实现内部。

## 调用方式

```cpp
#include "Selection/iGameExtractBlockFilter.h"
#include <iostream>

auto filter = iGame::ExtractBlockFilter::New();
filter->SetInput(multiblock);
filter->SetBlockIndex(1); // 第二个直接子块
// filter->SetBlockPath({1, 0}); // 第二个子块下的第一个子块
// filter->SetBlockName("ExtractSelectionTetrahedra");
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << std::endl;
} else {
    auto result = filter->GetOutput();
}
```

最后一次调用的选择设置生效。名称在全部后代中必须唯一，重名时使用索引路径。
`GetBlocks(input)` 返回直接子块，可列出索引与名称。
现有 VTM 读取器通常使用引用文件的文件名作为叶块名称，不保证采用 VTM 的 `name` 字段。

## Qt 操作

1. 打开 `Examples/Models/ExtractBlockFlat.vtm`，选择模型树中多块模型的根节点。
2. 进入“算法处理 > 提取分块 (Extract Block)”，这是一级菜单项。
3. 选择“索引路径”，输入 `1`；或选择“块名称”，输入 `ExtractSelectionTetrahedra`。
4. 结果模型名称后缀为 `_extracted_block`。隐藏原多块模型，并点击结果模型名称。
5. 结果应有 **5 点、2 个四面体**。查看结果的点属性 `temperature` 与单元属性 `material`。
6. 对 `ExtractBlockNested.vtm` 使用路径 `1,0`，得到同样的四面体叶块；路径 `1` 提取整个第二子树。

VTM 和其引用的两个 VTU 文件必须放在同一目录，不可只移动 VTM。
模型文件名含 ExtractSelection 是沿用人工测试数据名称，不代表调用 20 的滤镜。
模型为人工构造，不使用 `Tet_Plane.vtk`。录制时请使用全英文的数据路径。

## 测试模型

| 文件 | 内容 |
| --- | --- |
| ExtractBlockFlat.vtm | 直接包含三角形块和四面体块 |
| ExtractBlockNested.vtm | 两个容器，每个包含一个叶块 |
| ExtractSelectionTriangles.vtu | 5 点、2 个三角形，含一个闲置点 |
| ExtractSelectionTetrahedra.vtu | 5 点、2 个共享面的四面体 |

四面体块的 `temperature` 范围为 [-5, -1]，`material` 为 100、200。
平铺索引 `1`、嵌套路径 `1,0`、唯一名称均应提取该块。

## 构建与自动测试

从项目根目录运行，GUI 不需要参与核心测试：

```text
cmake -S . -B build-block -DENABLE_QT_MODULE=OFF -DBLOCK_EXTRACTION_TESTS=ON
cmake --build build-block --config Release --target TestExtractBlock BlockExtractionExamples
ctest --test-dir build-block -C Release -R "^(TestExtractBlock|BlockExtractionExamples)$" --output-on-failure
```

`TestExtractBlock` 覆盖空输入、非法索引、空路径、缺失/重复名称、嵌套路径、子树、
独立几何/属性复制、64 位整数精度、向量及单元属性、空槽位、循环引用、短属性和错误后清除输出。
`BlockExtractionExamples` 自动读取两种 VTM，验证索引/路径及名称，结果写入 `ExtractionResults/`。
直接运行示例时工作目录必须为项目根目录；两个示例都不依赖 20 的测试程序。

## 原生对比

在示例生成结果后，从项目根目录执行：

```text
pvpython Examples/Filter/Selection/CompareBlockVTK.py
pvpython Examples/Filter/Selection/CompareBlockParaView.py
```

第一条运行原生 VTK `vtkExtractBlock`，第二条运行原生 ParaView `ExtractBlock`。
脚本比较点/单元数量、坐标、单元类型、连接关系、数值属性名称、分量、元组数量和数值。
不使用额外清理滤镜代替原生结果。两个案例的实际叶块均为 5 点、2 个四面体，数值对比通过。
ParaView 脚本在 `ExtractionResults/paraview-block/` 保存截图、报告和状态文件。
状态文件含绝对路径，换机器需要重新生成或重新定位数据文件。
该自动对比和截图不是 GUI 操作录屏；PR 仍需附 iGameVis 与 ParaView 两段操作视频。

## ParaView 操作

1. 打开相同 `ExtractBlockFlat.vtm`，点击 `Apply`。
2. 搜索 `Extract Block`，选择四面体块 `ExtractSelectionTetrahedra`，点击 `Apply`。
3. 隐藏原输入，显示并选中提取结果，设为 `Surface With Edges`。
4. 在 `Information` 检查 5 点、2 单元，以及属性范围。
5. 嵌套案例同样选中四面体叶块，不要把容器的 flat index 当成每层位置索引。

## 差异与限制

- 本接口使用每层零起始位置；VTK 使用包含根节点的前序 flat index。平铺 `1` 对应 flat index `2`，嵌套 `1,0` 对应 `4`。
- 本实现直接输出块/子树；VTK 和 ParaView 可能保留外层多块容器，应比较实际叶块。
- 非 Composite 子对象映射按对象唯一 ID 排序，不保证等于外部文件中不连续的 `index`。
- 空块不可提取，复制子树会跳过空槽位；循环引用、重名、未支持类型会报错。
- 普通结构化网格归一化为体网格，不保留结构化维度元数据。
- 专用多面体 `VolumeMesh` 暂不支持，需先转为 `UnstructuredMesh`。
- 不复制任意字段元数据、渲染配置、选择状态；不承诺完全保留容器元数据。
- 文件写出使用现有 VTKWriter，部分单元属性可能变为 float；滤镜内存中的数值类型保持不变。
- `Execute()` 失败时输出为空，`GetLastError()` 返回原因。

## 本次验证边界

上传中的 CMake 和 Qt 主窗口以 `d849c2d1bf85dcff44c1773ab84ca1eda75af45f` 为基线。
29 滤镜实现及两个程序已独立重新编译、运行；复用了此前构建的 iGameCore 库。
原生 VTK 和 ParaView 5.13.0 的两种分块对比均通过。
最新核心库和完整 Qt GUI 尚未全量重编译，不应把现有可执行程序称为本上传包的全量构建结果。
