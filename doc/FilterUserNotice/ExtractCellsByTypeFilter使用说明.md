# ExtractCellsByTypeFilter 使用说明（按单元类型提取过滤器）

## 一、功能概述

`ExtractCellsByTypeFilter`（按单元类型提取）根据用户勾选的单元类型，从输入网格中提取对应单元组成新网格（`UnstructuredMesh`），在模型树中以独立新模型显示。

- **保留输入模型**：整个流程不修改、不覆盖用户原始导入的网格；
- **属性不丢失**：点属性按"被保留的点"搬运、单元属性按"被选中的单元"搬运，且**保留数组原始类型**（Double / Int / 64 位 ID 类型与精度均保留，不会降成 float）；
- **输出命名**：$\text{ExtractCellsByType}_n$，$n$ 为第几个提取的模型（全局递增）；
- **适配框架全部单元类型**：三角形、四边形、多边形、四面体、六面体、三棱柱、金字塔、多面体以及二次/拉格朗日单元。

## 二、调用方式

### 1. GUI 调用（推荐）

菜单栏【算法处理】→ **【按单元类型提取 (Extract Cells By Type)】**。

### 2. 命令行 / 示例程序

```bat
REM 运行示例测试（在 Examples 构建目录下）
testExtractCellsByType.exe --no-show
```

### 3. C++ 接口

```cpp
#include <MyFilter/iGameExtractCellsByTypeFilter.h>

auto filter = iGame::ExtractCellsByTypeFilter::New();
filter->SetInput(mesh);                         // 输入：任意网格
// 扫描输入实际存在的单元类型（供 UI 勾选）
auto types = filter->GetAvailableCellTypes();
// 设置要提取的单元类型（空集合 = 不提取任何单元）
filter->SetExtractCellTypes({IG_TETRA, IG_HEXAHEDRON});
filter->Execute();
auto out = filter->GetOutput();                 // UnstructuredMesh
```

## 三、使用示例

### 示例 1：GUI 提取四面体

1. 导入一个含单元网格的模型（如 `.vtk`）；
2. 菜单【算法处理】→【按单元类型提取 (Extract Cells By Type)】；
3. 左侧功能栏列出该模型所有单元类型勾选框（默认全选，已自动提取一次）；
4. 模型树新增 $\text{ExtractCellsByType}_1$，**原模型保留不动**；
5. 只勾选"四面体 (Tetra)" → 点【提取】→ $\text{ExtractCellsByType}_1$ 原地更新为仅含四面体的网格；
6. 再次使用 → 生成 $\text{ExtractCellsByType}_2$，互不干扰。

### 示例 2：读取框架自带的混合测试模型

```cpp
auto obj = iGame::FileIO::ReadFile("./Models/ExtractCellsByType_mixed.vtk"); // 混合单元测试模型
auto filter = iGame::ExtractCellsByTypeFilter::New();
filter->SetInput(obj);
auto types = filter->GetAvailableCellTypes();   // {三角形, 四边形, 四面体, 六面体}
filter->SetExtractCellTypes(types);             // 全选
filter->Execute();
auto out = filter->GetOutput();                 // 与输入等价的完整副本
```

`Examples/Models/` 下已提供两个可直接读取的测试模型：

| 模型 | 内容 | 用途 |
|------|------|------|
| `ExtractCellsByType_mixed.vtk` | 三角形 + 四边形 + 四面体 + 六面体，带 float/double 属性 | 验证多类型提取 + 属性类型保留 |
| `ExtractCellsByType_surface.vtk` | 三角形 + 四边形（纯表面） | 验证表面/2D 单元提取 |

## 四、注意事项

| 注意点 | 说明 |
|--------|------|
| **输入模型不被覆盖** | 提取结果始终是独立新模型，源模型保持原样 |
| **属性类型保留** | 覆盖层按输入数组的实际类型（`GetArrayType()`）创建同类型数组并类型化拷贝，不再统一转成 `FloatArray` |
| **输出点序 = 首次被引用顺序** | 提取后点的编号会重排（与输入不同），但每个点的属性值随映射正确搬运，不影响一一对应 |
| **`dataRange` 复用输入范围** | 属性范围（云图配色用）复用了输入范围，是子集的超集，不影响数值正确性；如需精确配色可自行重算 |
| **多面体编码特例** | 多面体在连接表里是编码序列 `[面数, 面0顶点数, 面0顶点...]`，提取只重映射真正的顶点编号，面数/顶点数等描述字段原样保留 |
| **结构化网格按维度单一类型** | 结构化网格（2D 全四边形 / 3D 全六面体）无法是混合网格，按 `GetDimension()` 判定即可 |
| **已知限制** | 1D 结构化网格（线）暂不支持；非常规顶点数的体网格（非 4/5/6/8 且非多面体标记）会明确拒绝；多块网格（MultiBlockMesh）暂不支持遍历子块 |
| **菜单层级** | 菜单项挂在【算法处理】一级菜单下，无子菜单嵌套 |
| **n 命名语义** | $n$ 表示"第几个提取的模型"：每次重新点菜单会新建一次会话（新的 $n$），修改勾选重提取则在**同一**模型上原地更新 |
