# ExtractEdgesFilter 使用说明

对应任务：中等任务（#28）——提取网格的边（去重），供可视化与导出。

## 1. 功能

`ExtractEdgesFilter` 从输入网格中提取**全部唯一边**（两个端点连成的 1 维线段单元），
并输出一个新的非结构化网格 `UnstructuredMesh`，其中的每个单元都是 `IG_LINE`（2 点线段）：

- 支持面网格（三角形/四边形）与体网格（四面体/六面体等）：按单元的**拓扑边**提取；
- 输入中已有的线单元（`IG_LINE`）原样保留，折线（`IG_POLY_LINE`）会被拆分为多条线段；
- **共享边自动去重**：相邻单元共用的边只保留一条（如两个三角形共享的边不会被统计两次）；
- 输出网格复用输入的点坐标，不复制点数据，属性集随网格一起透传。

典型用途：把体/面网格变成"线框"，展示网格拓扑结构，或将提取出的边导出为 `.vtk` 文件
（配合 `ExportEdgesFilter` 一键导出）。

## 2. 支持的数据类型

输入可为任意网格（`SurfaceMesh`、`VolumeMesh`、`UnstructuredMesh`、`StructuredMesh` 等）。
Filter 内部先将输入统一转为 `UnstructuredMesh` 表示，再执行提取，因此：
- 输入无单元或转换失败时：直接透传输入并返回成功，不崩溃；
- 输出一定是 `UnstructuredMesh`，单元类型全部为 `IG_LINE`。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个网格模型（推荐体网格或面网格，边提取效果更明显）；
2. 主菜单 **算法处理 → 边提取**，在左侧面板点击 **执行**；
3. 视图区即显示提取出的边（线框）；如需导出，点击面板中的 **导出边为 VTK**，选择保存路径即可。

### 3.2 代码方式

```cpp
#include <ExtractEdges/iGameExtractEdgesFilter.h>
#include <ExportEdges/iGameExportEdgesFilter.h>

// 1. 创建 Filter 并执行
auto filter = iGame::ExtractEdgesFilter::New();
filter->SetInput(mesh);
filter->Execute();

// 2. 输出：全部为 IG_LINE 的边网格
auto edgesMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());

// 3.（可选）导出为 .vtk 文件
auto exporter = iGame::ExportEdgesFilter::New();
exporter->SetInput(edgesMesh);
exporter->SetFilePath("edges.vtk");
exporter->Execute();
```

### 3.3 命令行测试

```bash
cd Examples
./testExtractEdges
```

测试程序自动读取 `Examples/Models/ExtractEdges_hexa_grid.vtk`（相对路径，无需手动输入），
校验输出网格每个单元都是 `IG_LINE` 且恰有 2 个互异端点，全部通过输出 PASS。

## 4. 使用示例

使用仓库自带的 AI 生成测试模型 `Examples/Models/ExtractEdges_hexa_grid.vtk`：
一个 `2×2×1` 的六面体（Hexahedron）连续网格，共 18 个顶点、4 个六面体单元，单元之间共享顶点与棱。

执行 `ExtractEdges` 后：
- 输入 4 个六面体（每个 12 条棱，共 48 条棱）中的共享棱被去重；
- 输出 33 条**唯一边**（`IG_LINE`），恰好等于该 3D 线框网格的全部棱：
  - 沿 X 方向：`nx·(ny+1)·(nz+1) = 2·3·2 = 12`
  - 沿 Y 方向：`(nx+1)·ny·(nz+1) = 3·2·2 = 12`
  - 沿 Z 方向：`(nx+1)·(ny+1)·nz = 3·3·1 = 9`
  - 合计 `12 + 12 + 9 = 33`

在 GUI 中执行后即可看到完整的六面体线框；用 ParaView 打开导出的 `.vtk` 文件同样显示为线框，
文件内 `CELL_TYPES` 均为 `3`（vtkLine），为标准 VTK 线单元。

## 5. 注意事项

1. **边是"拓扑边"**：提取依据是单元的拓扑连接（如四面体的 6 条棱、六面体的 12 条棱），
   与几何形状是否弯曲无关；高次（二次/拉格朗日）单元按基础单元拓扑提取。
2. **共享边只保留一次**：相邻单元（如共享一条棱的两个四面体）的公共边会自动去重，
   因此输出边数 ≤ 各单元棱数之和。
3. **输入中的边不丢失**：输入若本身含 `IG_LINE` 单元，会被透传保留；折线 `IG_POLY_LINE` 会逐段拆成多条 `IG_LINE`。
4. **单点单元（VERTEX）会跳过**：0 维点单元不产生边（与读取器行为一致，读取 VTK 时 VERTEX 不建单元）。
5. **输出是新的网格对象**：与输入相互独立，改输出不会影响输入；点数据为浅共享（同一份坐标）。
6. 若要把边网格保存到文件，建议使用 **VTK（.vtk）格式**：
   面模型类格式（STL/OBJ/PLY/OFF）不支持 1 维线单元，只有 VTK/VTU 等非结构化格式能完整保存。
