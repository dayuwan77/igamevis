# ExtractCellsByRegionFilter 使用说明

## 功能

`iGame::ExtractCellsByRegionFilter` 根据轴对齐盒子（Box）或球体（Sphere）区域，从 `UnstructuredMesh` 中提取满足条件的完整单元，输出新的子网格。

- 严格模式：单元的所有顶点都在区域内才保留，默认启用。
- 宽松模式：单元至少一个顶点在区域内即保留。
- 区域边界上的点算在区域内。
- 输出压缩点集、重新映射单元点号，并按对应关系复制点属性和单元属性。

## 调用方式

头文件：`#include <Selection/iGameExtractCellsByRegionFilter.h>`。

| 接口 | 作用 |
|---|---|
| `ExtractCellsByRegionFilter::New()` | 创建 Filter |
| `SetInput(0, mesh)` | 设置输入非结构网格 |
| `SetBox(min, max)` | 设置盒子的最小点与最大点，两者为 `Vector3d` |
| `SetSphere(center, radius)` | 设置球心 `Vector3d` 与半径 |
| `SetRequireAllPoints(true/false)` | 选择严格/宽松模式 |
| `Execute()` | 执行；失败返回 `false` |
| `GetOutput()` | 获取输出，再转为 `UnstructuredMesh` |

`SetBox` 和 `SetSphere` 选择不同区域，最后一次调用决定本次使用的区域。

## 使用示例

以下代码从固定相对路径读取新模型，不需要手动输入参数。工作目录为仓库的 `Examples`。

```cpp
#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameFileIO.h>

int main() {
    auto obj = iGame::FileIO::ReadFile("./Models/ExtractCellsByRegion_BoxCases.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh.IsNull()) return 1;
    auto filter = iGame::ExtractCellsByRegionFilter::New();
    filter->SetInput(0, mesh);
    filter->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    filter->SetRequireAllPoints(true);
    if (!filter->Execute()) return 1;
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    return !output.IsNull() && output->GetNumberOfCells() == 1 ? 0 : 1;
}
```

球体宽松提取时，将区域和模式设置替换为：

```cpp
filter->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
filter->SetRequireAllPoints(false);
```

## 新模型与自动测试

两个 ASCII VTK 非结构四面体模型均由 AI（Codex）为本 Filter 新生成，未使用旧测试模型。每个单元有四个独立顶点，方便核对提取后的点集压缩。

1. `Examples/Models/ExtractCellsByRegion_BoxCases.vtk`：16 点、4 个四面体，分别位于区域内、跨越区域边界、完全在区域外、仅一个顶点位于边界 `(1,0,0)`。
2. `Examples/Models/ExtractCellsByRegion_SphereCases.vtk`：20 点、5 个四面体。在上述四类基础上增加盒内但球外的对角区域单元，用来区别 Box 与 Sphere 的几何判断。

测试区域：盒子 `[-1,1] × [-1,1] × [-1,1]`；球心 `(0,0,0)`，半径 `1`。

| 模型 | Box 严格 | Box 宽松 | Sphere 严格 | Sphere 宽松 |
|---|---:|---:|---:|---:|
| BoxCases | 1 | 3 | 1 | 3 |
| SphereCases | 2 | 4 | 1 | 3 |

表中数字为预期输出单元数；预期点数为单元数的 4 倍。测试还验证远离模型的盒子返回空网格、非法盒子与零/负半径被拒绝，每个模型共 8 项检查。

完整自动测试位于 `Examples/Filter/Selection/TestExtractCellsByRegion.cpp`。沿用已合并接口的 `testExtractCellsByRegion` 构建目标。在已配置好 iGameCore 和 Examples 的环境中，从仓库根目录执行（以 Windows、构建目录 `examples-build` 为例）：

```powershell
cmake --build examples-build --config Release --target testExtractCellsByRegion
Set-Location Examples
..\examples-build\Release\testExtractCellsByRegion.exe
```

请确保 iGameCore 及依赖 DLL 可被程序找到，按项目现有环境设置 PATH 或放入可执行文件目录。构建目录名称可随本地配置调整；模型路径和测试参数已经固定，无需交互输入。程序自动执行全部测试，成功输出 `ALL TESTS PASSED` 并返回 `0`；失败返回 `1`。

## 注意事项

- 输入必须是有效的 `UnstructuredMesh`，读取失败或类型不符时先停止调用。
- Box 每个坐标轴都必须满足 `min <= max`；球半径必须大于零。使用有限数值参数。
- 这是基于顶点的整单元筛选，不切割单元。即使单元与区域相交，只要所有顶点都在外部，宽松模式也不会选中。
- 相对路径相对于进程当前工作目录，不是源代码或可执行文件所在目录。推荐从 `Examples` 启动；也可从已经包含新 `Models` 文件的 Examples 构建目录启动。
- 没有单元满足条件时，成功返回空网格属于正常结果。
- 输出点号会重排，不能直接把输出点号当作原始点号使用。
- 新模型专门验证区域选择和点集压缩，不带属性数组；本示例不覆盖属性复制正确性的全部情形。
