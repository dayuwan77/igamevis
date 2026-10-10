# OutlineCornerFilter 使用说明

## 功能

根据输入数据的轴对齐包围盒生成八个角部标记。每个角沿 X、Y、Z 方向各生成一条短线，共 32 个点、24 个线单元。输出是独立的 UnstructuredMesh，输入模型不变。

此功能提取的是包围盒角部，不是模型表面的几何角点，也不是视角相关的轮廓线。

## 调用方式

```cpp
#include "FeatureExtraction/iGameOutlineCornerFilter.h"
#include "iGameFileIO.h"

auto input = iGame::FileIO::ReadFile("Models/OutlineCorners_Box.vtk");
if (!input) return 1;
auto filter = iGame::OutlineCornerFilter::New();
filter->SetInput(input);
filter->SetCornerFactor(0.2f);
if (!filter->Execute()) return 1;
auto output = filter->GetOutput();
auto lines = filter->GetResult();
```

Execute 返回成功与否，GetResult() 返回线网格，GetOutput() 返回同一输出数据对象。界面可将结果作为单独节点与原模型同时显示。

CornerFactor 默认 0.2，表示各短线长度占该轴包围盒边长的比例。数值限制在 [0.001, 0.5]；非有限数恢复默认值。

## 使用示例

源码：`Examples/Filter/FeatureExtraction/OutlineCorners.cpp`。

在已配置 Examples 的构建目录中构建：

```powershell
cmake --build build --target testOutlineCorners
cd build/Examples
./testOutlineCorners.exe
```

Linux 下运行 `./testOutlineCorners`。示例无需参数，无交互输入，固定读取 `Models/OutlineCorners_Box.vtk` 和 `Models/OutlineCorners_Plane.vtk`。现有 CMake 会复制 Models 目录；手动执行时须在包含 Models 子目录的 example 目录运行。CTest 已设置该工作目录：

```powershell
ctest --test-dir build/Examples -R "^testOutlineCorners$" --output-on-failure
```

测试模型均位于 Examples/Models：

| 模型 | 预期 |
| --- | --- |
| OutlineCorners_Box.vtk | 包围盒 X=[-2,4]、Y=[1,5]、Z=[-3,7]；比例 0.2 时各轴短线长度为 1.2、0.8、2；另检查比例 0.5 |
| OutlineCorners_Plane.vtk | 包围盒 X=[-3,5]、Y=[-2,4]、Z=[2,2]；比例 0.2 时各轴短线长度为 1.6、1.2、0 |

两个小型合成模型具有明确可核验的范围，不依赖 Tet_Plane.vtk。示例自动检查角坐标、短线端点、连接关系、输出数量及输入对象保持情况。终端打印测试结果，全部通过返回 0，失败返回非零。

## 注意事项

- 支持能提供有效包围盒的数据对象，包括点集、曲面、体网格、结构化与非结构化网格；不依赖具体单元拓扑。
- 包围盒与全局坐标轴对齐，不是最小体积的旋转包围盒。
- 平面或直线数据会出现重合角点及零长度短线；仍保留 32 个点、24 个线单元。
- 缺少输入、空包围盒或非有限坐标时 Execute 返回 false；不要在失败后使用结果。
- 参数校正及输入错误沿用项目 igDebug/igError；GetMessage() 返回执行说明，日志输出位置由项目配置管理。
