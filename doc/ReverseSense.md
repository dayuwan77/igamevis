# Reverse Sense（反转面朝向过滤器）

`ReverseSenseFilter`：翻转曲面朝向。

## 行为

| 开关 | 默认 | 作用 |
| --- | --- | --- |
| `SetReverseCells(bool)` | `true` | 反转每个面的顶点环序，翻转几何法向 |
| `SetReverseNormals(bool)` | `true` | 把点/单元 `IG_NORMAL` 法向数组逐分量取反 |

## 输入 / 输出

- 输入：`SurfaceMesh`。
- 输出：新的独立 `SurfaceMesh`，点数/面数/单元类型不变。
- 属性：全部按原数组类型搬运；仅 `IG_NORMAL` 在开关打开时取反。
- 面环序改变后会重建边与点→面/边→面邻接。

## 相关文件

- `iGameCore/Filters/ReverseSense/iGameReverseSenseFilter.h/.cpp`
- `Examples/Filter/ReverseSense/TestReverseSense.cpp`
- `Examples/Filter/ReverseSense/TestReverseSenseSelfCheck.cpp`
- `Examples/Models/AIGen_Surface_ReverseSenseDemo.obj`、`..._WaveSheet.obj`、`..._AxisFaces.obj`
- `doc/FilterUserNotice/ReverseSenseFilter使用说明.md`

## 属性与失败行为

- 支持的数值属性按原始数组类型独立复制，64 位整数不经过浮点转换。
- 不支持的数组类型会明确失败；失败时输出清空，不保留上次执行的结果。
- 法向取反若超出原类型可表示范围，会失败并给出说明；关闭法向取反时可原样复制。
