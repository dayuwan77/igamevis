# CountCellVerticesFilter 使用说明

对应任务：简单任务（#5）——统计网格中每个单元的顶点数。

## 1. 功能

`CountCellVerticesFilter` 遍历输入网格的全部单元，统计**每个单元拥有的顶点（节点）数**，
并把结果作为 **cell 属性（单元数据）** 写回到输入网格：

| 输出属性名 | 类型 | 附加位置 | 含义 |
| --- | --- | --- | --- |
| `cell_vertex_count` | DoubleArray（一维） | 单元（cell） | 每个单元的顶点数，长度 = 单元数 |

- 对三角形单元输出 3、四边形输出 4、四面体输出 4、六面体输出 8，其余类型依此类推（以单元实际存储的节点数为准）。
- 该 Filter 是**原地（in-place）计算**：不改变几何与拓扑，只在输入网格的属性集中新增 `cell_vertex_count`，
  输出数据对象与输入相同（可继续串联下游 Filter）。
- 计算完成后会自动刷新渲染数据，界面上可按 `cell_vertex_count` 着色查看。

## 2. 支持的数据类型

输入只需具备单元连接表（CellArray），框架中的网格均可直接使用，包括：

- `UnstructuredMesh`（非结构化网格，支持任意混合单元）
- `SurfaceMesh`（表面网格）
- `VolumeMesh`（体网格）
- `PointSet` 及其它派生网格

若输入没有单元（`numCells == 0`）或输入为空，Filter 直接透传输入并返回 `true`，不报错。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个网格模型；
2. 主菜单 **算法处理 → 统计单元顶点数**，在左侧面板点击 **执行**；
3. 结果属性 `cell_vertex_count` 会出现在属性列表中，可选择它按顶点数着色。

### 3.2 代码方式

```cpp
#include <CountCellVertices/iGameCountCellVerticesFilter.h>

// 1. 创建 Filter
auto filter = iGame::CountCellVerticesFilter::New();
// 2. 喂入网格（任意网格类型）
filter->SetInput(mesh);
// 3. 执行
filter->Execute();
// 4. 结果已写回 mesh 属性集：
//    mesh->GetAttributeSet() 中名为 "cell_vertex_count" 的 cell 属性
```

### 3.3 命令行测试

```bash
cd Examples
./testCountCellVertices
```

测试程序自动读取 `Examples/Models/CountCellVertices_mixed_cells.vtk`（相对路径，无需手动输入），
校验 `cell_vertex_count` 存在、长度等于单元数、且每个值与单元实际节点数一致，全部通过输出 PASS。

## 4. 使用示例

使用仓库自带的 AI 生成测试模型 `Examples/Models/CountCellVertices_mixed_cells.vtk`
（一个包含多种单元类型的展示网格：六面体、三棱柱、金字塔、四面体、四边形、三角形、线段）：

| 单元类型 | 顶点数 |
| --- | ---: |
| 六面体 Hexahedron | 8 |
| 三棱柱 Prism (Wedge) | 6 |
| 金字塔 Pyramid | 5 |
| 四面体 Tetrahedron | 4 |
| 四边形 Quad | 4 |
| 三角形 Triangle | 3 |
| 线段 Line | 2 |

执行后生成的 `cell_vertex_count` 依次为：`8, 6, 5, 4, 4, 4, 3, 3, 2`，
正好对应各单元的顶点数，可用于直观核对结果。

## 5. 注意事项

1. **属性是附加（新增）而不是替换**：多次执行会在属性集中重复新增同名属性，建议执行前先清理旧的同名属性。
2. **单点单元（VERTEX）不入统计**：iGame 的 VTK 读取器在读入文件时不会为 `VERTEX`（1 点）单元建立 Cell，
   因此 0 维点单元不会出现在统计结果中；线段（LINE，2 点）单元正常统计为 2。
3. **原地计算语义**：Filter 的输出与输入是同一个数据对象，若下游需要保留"未统计"的原始网格，请先拷贝一份。
4. **统计口径**：顶点数取自单元连接表中实际存储的节点个数（`CellArray::GetCellSize`），
   与几何坐标中是否重复无关，也与单元类型枚举的固定节点数无关（混合/高次单元按实际节点数统计）。
5. 输入为空、无单元等边界情况不会崩溃，Filter 直接返回成功并透传输入。
