# ResampleToImageFilter 使用说明

## 功能

**Resample To Image（重采样到图像网格）** 过滤器，是 VTK `vtkResampleToImage` 的移植实现。
它把输入网格（`PointSet` 及其子类，如 `UnstructuredMesh` / `StructuredMesh` / `SurfaceMesh` /
`VolumeMesh`）在指定的采样区域上建立一个规则的图像网格，并在每一个网格格点处对输入的**点属性**
进行探针插值（probe），从而把网格上的场重采样为图像上的点场。

输出是一个 `StructuredMesh`（等价 `vtkImageData`）：

- 原点 `origin` = 采样区域的 `(xmin, ymin, zmin)`；
- 间距 `spacing[i]` = `(dims[i] == 1) ? 0 : (bounds[2i+1] - bounds[2i]) / (dims[i] - 1)`；
- 维度 = `SamplingDimensions`（默认 `10 × 10 × 10`）；
- 点数据中包含：
  - 输入点属性数组的**插值结果**（同名数组，落在网格外的格点取默认值 0）；
  - `char` 数组 **`vtkValidPointMask`**：格点落在输入网格内为 1，否则为 0；
  - `unsigned char` 数组 **`vtkGhostType`**（点/单元两级）：无效点标记为隐藏点，任意角点为无效
    点的单元标记为隐藏单元——与 VTK `SetBlankPointsAndCells` 的 ghost 空白化行为一致。

## 参数

| 参数 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `SamplingDimensions` | `int[3]` | `{10, 10, 10}` | 图像各轴的格点数（大于 0） |
| `SamplingBounds` | `double[6]` | `{0,1, 0,1, 0,1}` | 显式采样区域，仅当 `UseInputBounds=false` 时生效 |
| `UseInputBounds` | `bool` | `true` | 使用输入数据包围盒作为采样区域（向内收缩 epsilon） |

## 调用方式

```cpp
#include <Convert/iGameResampleToImageFilter.h>
#include <iGameStructuredMesh.h>

auto filter = iGame::ResampleToImageFilter::New();
filter->SetInput(input);                          // 输入：PointSet 子类
filter->SetSamplingDimensions(64, 64, 64);        // 可选，采样分辨率
// filter->SetSamplingBounds(x0, x1, y0, y1, z0, z1); // 可选，显式采样区域
// filter->SetUseInputBounds(true);               // 可选，默认 true
if (filter->Execute()) {
    auto out = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
    // out 即重采样后的图像（点场 + vtkValidPointMask + vtkGhostType）
}

// 有效点掩膜数组名为固定值
const char* maskName = iGame::ResampleToImageFilter::GetMaskArrayName(); // "vtkValidPointMask"
```

## 使用示例

示例程序：`Examples/Filter/Convert/TestResampleToImage.cpp`
测试模型：`Examples/Models/ResampleCube.vtk`（标量字段）、`Examples/Models/ResampleCubeVector.vtk`（三分量字段）

示例硬编码相对路径并在读入后自动执行重采样，输出各轴分辨率、原点、间距、有效点数量、首个有效格点的
插值场，并读取 `vtkGhostType` 抽出有效单元表面进行显示：

- `ResampleCube.vtk` / `ResampleCubeVector.vtk`：一个挖掉角部小立方体的四面体立方体（27 点 /
  42 个四面体），带点字段 `field`（分别为 1 分量 `x²+y²+z²` 与 3 分量 `(x, y, z)`），烘焙矩形区域外
  的 ghost 空白化效果。
- 运行方式（工作目录为 `build-msvc`）：`.\Release\testResampleToImage.exe`

## 注意事项

1. **输入类型**：只接受 `PointSet` 及其子类；空数据或非网格输入会报错并返回 `false`。
2. **支持的单元类型**：`顶点 / 线段 / 三角形 / 四边形 / 四面体 / 六面体`；`棱柱 / 金字塔 / 多边形 /
   多面体 / 二阶单元` 暂不支持，采样点落在这些单元内时按「不在单元内」处理（`vtkValidPointMask`=0）。
3. **插值缓冲区**：按输入属性的最大分量数动态分配，任意分量数（含 >16 分量）均无越界风险。
4. **单元数据（cell data）语义**：与 `vtkProbeFilter` 一致——与某点数组同名的单元数组被丢弃（点数据
   优先），其余单元数组作为输出点数组「快照」。
5. **性能**：点定位采用「单元包围盒预过滤 + 遍历」，`SamplingDimensions` 过大或输入单元数过多时
   耗时线性增长，可按需调低分辨率。
6. **空白化显示**：iGame 渲染默认不会自动隐藏 ghost 单元；如需与 ParaView 一致的「有效区域」显示，
   请用 `ModelGeometryFilter` 读取输出上的 `vtkGhostType` 数组（测试用例中已演示）。