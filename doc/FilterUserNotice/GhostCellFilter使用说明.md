# GhostCellFilter（生成 Ghost 单元标记）使用说明

## 一、功能

GhostCellFilter 根据网格点上的 ghost 标记（`GhostPoints` 点属性，非 0 表示 ghost 点），
为网格中的每个单元生成 ghost 标记：只要一个单元包含任意一个 ghost 点，就把该单元标记为
ghost 单元（1），否则标记为正常单元（0）。结果写入单元属性 `GhostCells`。

该标记用于渲染、筛选或统计时区分 ghost 单元，与 VTK 的 ghost cell 数组语义一致。

支持三类网格：

- 表面网格（SurfaceMesh）
- 体网格（VolumeMesh）
- 非结构化网格（UnstructuredMesh）

## 二、调用方式

### 1. 命令行 / 代码调用

```cpp
#include <GhostCell/iGameGhostCellFilter.h>

auto filter = iGame::GhostCellFilter::New();
filter->SetInput(0, mesh);      // mesh 为任意支持类型的网格
if (filter->Execute()) {
    // 成功后，网格单元属性中会新增 GhostCells
}
```

输入模型的点属性名默认为 `GhostPoints`，可通过
`filter->SetPointGhostArrayName("其他名字")` 修改。

### 2. 可视化界面调用

1. 打开 iGameVis，加载模型（模型需带 `GhostPoints` 点属性）；
2. 菜单：**算法处理 → Ghost 单元标记 (Ghost Cells)**；
3. 执行完成后，模型树中自动展开并选中 `GhostCells` 属性，可查看每个单元的 ghost 值。

## 三、使用示例

`Examples/Filter/GhostCell/TestGhostCellModel.cpp` 提供了自动测试示例，无需手动输入：

```powershell
cmake --build build --config Release --target testGhostCellModel --parallel
F:\igamevis\build\Examples\Release\testGhostCellModel.exe
```

示例会自动读取 `Examples/Models/` 下的两个测试模型并核对输出：

- `GhostCell_Pyramid.vtk`：金字塔（4 个侧面 + 2 个底面三角形），顶点为 ghost 点，
  预期 `GhostCells = [1, 1, 1, 1, 0, 0]`；
- `GhostCell_TwoTets.vtk`：两个四面体，其中一个含 ghost 点，
  预期 `GhostCells = [0, 1]`。

运行结束应输出 `ALL TESTS PASSED`。

## 四、注意事项

1. 输入网格必须有点属性 `GhostPoints`（点上有 0/1 标记）。若没有该属性，filter 不会报错，
   但所有单元都会被标记为 0。
2. 输出属性名为 `GhostCells`；若网格上已存在同名属性，会被覆盖为本次计算结果。
3. 模型文件需要放在仓库的 `Examples/Models/` 目录下，示例使用相对路径自动定位，无需手动输入路径。
