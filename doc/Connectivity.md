# Connectivity（连通区域过滤器）

`ConnectivityFilter`：把曲面按连通性分组。

## 行为

- 连通判定：两个面存在**公共顶点**即为相邻；多个相邻面构成连通区域。
- 邻接关系：过滤器内部构建点→面 CSR 邻接（不修改输入），用双波前 BFS 遍历。
- 输出：`SurfaceMesh`。`ALL_REGIONS` 保留全部面；其余模式抽取子网格。

## 参数

| 方法 | 说明 | 默认 |
| --- | --- | --- |
| `SetExtractionMode(int)` | 6 种模式（见下） | `LARGEST_REGION` |
| `SetColorRegions(bool)` | 写 `RegionId` 点/单元标量 | `true` |
| `SetRegionIdAssignmentMode(int)` | `UNSPECIFIED` / `CELL_COUNT_ASCENDING` / `CELL_COUNT_DESCENDING` | `UNSPECIFIED` |
| `SetScalarConnectivity(bool)` + `SetScalarRange(lo,hi)` | 叠加标量区间连通条件 | 关 |
| `SetFullScalarConnectivity(bool)` | 单元所有顶点需在区间内 | `false` |
| `SetScalarArrayName(name)` | 标量连通用的点属性名 | 自动选取 |
| `InitializeSeedList()/AddSeed(id)` | 点/单元种子 | — |
| `InitializeSpecifiedRegionList()/AddSpecifiedRegion(id)` | 指定区域编号 | — |
| `SetClosestPoint(x,y,z)` | 最近点模式坐标 | (0,0,0) |

抽取模式：`POINT_SEEDED_REGIONS=1`、`CELL_SEEDED_REGIONS=2`、`SPECIFIED_REGIONS=3`、`LARGEST_REGION=4`、`ALL_REGIONS=5`、`CLOSEST_POINT_REGION=6`。

## 相关文件

- `iGameCore/Filters/Connectivity/iGameConnectivityFilter.h/.cpp`
- `Examples/Filter/Connectivity/TestConnectivity.cpp`
- `Examples/Filter/Connectivity/TestConnectivitySelfCheck.cpp`
- `Examples/Models/AIGen_Surface_ThreeIslands.obj`、`..._MultiRegion.obj`
- `doc/FilterUserNotice/ConnectivityFilter使用说明.md`
