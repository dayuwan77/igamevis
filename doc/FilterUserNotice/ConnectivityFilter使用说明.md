# Connectivity 使用说明

连通区域（Connectivity）过滤器：把曲面按连通性分组。

**连通定义**：两个面只要存在**公共顶点（共享点）**即视为相邻；连续相邻的面构成一个连通区域（region）。注意这与"共享边"不同。

目前仅支持 `SurfaceMesh` 输入，输出为一份**独立**的 `SurfaceMesh`。

## 一、功能

抽取模式与编号着色是两个独立选项：

- **抽取模式**决定保留全部区域，还是只输出选中的区域。
- **编号着色**给输出点属性与单元属性各写一条 `RegionId`，可以与任意抽取模式组合（`SetColorRegions(true)`，默认开）。再次运行会替换旧编号。取消勾选后，界面不自动启用区域着色。

6 种抽取模式：

| 模式 | 枚举 | 含义 |
| --- | --- | --- |
| 点种子 | `POINT_SEEDED_REGIONS` | 取给定点所在的区域 |
| 单元种子 | `CELL_SEEDED_REGIONS` | 取给定单元所在的区域 |
| 指定区域 | `SPECIFIED_REGIONS` | 抽取指定编号的区域 |
| 最大区域 | `LARGEST_REGION`（默认） | 抽取面数最多的区域 |
| 全部区域 | `ALL_REGIONS` | 编号全部区域（用于染色） |
| 最近点 | `CLOSEST_POINT_REGION` | 取离指定坐标最近点所在的区域 |

## 二、界面入口

【算法处理】→「开发中filter/第二批」→「**连通区域 (Connectivity)**」。对话框默认 **抽取模式 = 全部区域**、勾选 **按区域编号着色**；执行后输出作为新节点加入模型树，并按 `RegionId` 着色。

## 三、调用方式（代码）

```cpp
#include "Connectivity/iGameConnectivityFilter.h"

auto filter = iGame::ConnectivityFilter::New();
filter->SetInput(surfaceMesh);                                   // 输入 SurfaceMesh
filter->SetExtractionMode(iGame::ConnectivityFilter::ALL_REGIONS);
filter->SetColorRegions(true);                                   // 写 RegionId 用于染色
// filter->SetRegionIdAssignmentMode(ConnectivityFilter::CELL_COUNT_DESCENDING); // 按面数重编号
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << "\n";
    return 1;
}
auto output = filter->GetOutput();                               // 独立 SurfaceMesh
std::cout << "regions: " << filter->GetNumberOfExtractedRegions() << "\n";

// 抽取示例：最大区域 / 指定区域 / 种子 / 最近点
filter->SetExtractionMode(iGame::ConnectivityFilter::LARGEST_REGION);
filter->SetExtractionMode(iGame::ConnectivityFilter::SPECIFIED_REGIONS);
filter->InitializeSpecifiedRegionList();
filter->AddSpecifiedRegion(0);
filter->SetExtractionMode(iGame::ConnectivityFilter::CELL_SEEDED_REGIONS);
filter->InitializeSeedList();
filter->AddSeed(0);
filter->SetExtractionMode(iGame::ConnectivityFilter::CLOSEST_POINT_REGION);
filter->SetClosestPoint(x, y, z);
```

标量连通（可选）：单元除几何相邻外，还要求顶点标量落在给定区间内才算连通。

```cpp
filter->SetScalarConnectivity(true);
filter->SetScalarArrayName("v");        // 指定点属性数组；留空则自动选取
filter->SetScalarRange(0.0, 1.0);       // 区间
// filter->SetFullScalarConnectivity(true); // 要求单元所有顶点都在区间内
```

## 四、使用示例（自动测试，无需手动输入）

仓库自带程序化/AI 生成模型（`Examples/Models/`）：

- `AIGen_Surface_ThreeIslands.obj`：三个互不连通、面数分别为 9 / 4 / 1 的四边形面片，GUI 示例默认使用。
- `AIGen_Surface_MultiRegion.obj`：面数更多的示例（**107 个四边形面**），由 5 个部件组成：A(8×8) 与 C(4×4) **仅共享 1 个角点** → 因"共享点即连通"合并为一块；B(4×4)、D(2×5)、E(1×1) 各自独立。预期为 **4 个连通区域，大小 80 / 16 / 10 / 1**，用于演示"部分面相连、部分不相连"及最大区域抽取（会取 80 面的 A+C 块）。

运行（构建后工作目录为 `Examples`，路径写死、自动读取）：

```powershell
cd cmake-build-connectivity\Examples
.\Release\testConnectivity.exe

ctest -R testConnectivity
```

无 GUI 自动回归（程序内构造网格，断言全部 PASS 返回 0）：

```powershell
.\Release\testConnectivitySelfCheck.exe   # 无 GUI 自检
ctest -R testConnectivitySelfCheck
```

## 五、注意事项

- 连通按**共享点**判定，不是共享边；仅共享一个角点的两个面也会被归为同一区域。
- `RegionId` 同时写在点属性和单元属性上，均名为 `RegionId`；示例程序通过 `ViewCloudPicture` 按点属性 `RegionId` 着色。
- `CELL_COUNT_DESCENDING` 让面数最多的区域编号为 0，`CELL_COUNT_ASCENDING` 从面数最少的区域开始编号，`UNSPECIFIED` 保持遍历顺序；大小相同的区域保持原相对顺序。
- **指定区域**填写当前编号方式排序后的编号。排序即使在取消编号着色后，也会影响指定区域抽取。
- 种子/区域编号必须为范围内的非负整数，不能输入小数；多个编号用逗号或空格分隔。最近点坐标需填写三个有限数值。
- 切换模式后只显示对应的种子、区域或坐标输入。格式错误会在窗口内提示，并保留参数以便修改。
- 输入仅支持 `SurfaceMesh`；`UnstructuredMesh` / 体网格 / 空网格会返回 `false`（原因见 `GetMessage()`）。
- 点/单元种子模式没有有效种子时执行失败；最近点模式自动从所填坐标寻找种子。
- 开启标量连通时，普通模式要求至少一个顶点的有限标量落在区间内，全标量模式要求所有顶点命中。非种子模式中不符合条件的面各自保留为独立区域；种子模式没有符合条件的种子则失败。
- 输出为独立对象，不修改输入模型。

## 六、相关文件

| 文件 | 说明 |
| --- | --- |
| `iGameCore/Filters/Connectivity/iGameConnectivityFilter.h/.cpp` | 过滤器实现 |
| `Examples/Filter/Connectivity/TestConnectivity.cpp` | GUI 示例（默认读 `AIGen_Surface_ThreeIslands.obj`，按 RegionId 着色） |
| `Examples/Filter/Connectivity/TestConnectivitySelfCheck.cpp` | 无 GUI 自动回归 |
| `Examples/Models/AIGen_Surface_ThreeIslands.obj`、`..._MultiRegion.obj` | 程序化/AI 生成测试模型 |
