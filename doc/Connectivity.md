# Connectivity（连通区域过滤器）

`ConnectivityFilter`：把曲面按连通性分组。

## 行为

- 连通判定：两个面存在**公共顶点**即为相邻；多个相邻面构成连通区域。
- 邻接关系：过滤器内部构建预统计容量的点→面邻接（不修改输入），用双波前 BFS 遍历。
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

## 属性与失败行为

- 支持的数值属性按原始数组类型独立复制，64 位整数不经过浮点转换。
- 不支持的数组类型会明确失败；失败时输出清空，不保留上次执行的结果。

## 区域编号与标量连通边界

- 指定区域使用当前编号方式排序后的编号；排序对抽取也生效，与是否生成 RegionId 无关。
- 大小相同的区域在升序、降序下均保持遍历顺序。
- 生成 RegionId 时替换已有同名点/单元数组；关闭生成时原样复制已有属性。
- 普通标量连通要求至少一个顶点的有限标量落在闭区间内，全标量连通要求所有顶点命中。
- 全部/最大/指定区域模式中，不符合标量条件的面作为独立区域保留，不向邻居扩散；种子模式只从符合条件的面开始，没有合格种子则失败。
- 标量数组必须是完整的单分量点标量数组；标量范围和最近点坐标必须为有限值。
- 遍历在入队时标记面，每个合格点的邻接仅展开一次，避免重复入队和高连接度顶点的重复扫描。
