# PointSetToOctreeFilter 使用说明

## 功能

**Point Set To Octree（点集转八叉树）** 过滤器，是 VTK `vtkPointSetToOctreeImageFilter` 的移植实现。
它根据输入点集的包围盒与 `NumberOfPointsPerCell`，把包围盒划分为一个规则的体网格（分箱数
`numBuckets = numPoints / numPointsPerCell`，经等价于 `vtkBoundingBox::ComputeDivisions` /
`ClampDivisions` 的算法得到各轴分割数），输出一个 `StructuredMesh`，每个体素（cell）带一个
`unsigned char` 单元标量 **`octree`**，用 8 位 bitfield 记录该体素内落入的 8 个八分区（子八叉树节点）：

```
bit0 = (x<=cx, y<=cy, z<=cz)   bit1 = (x>cx, y<=cy, z<=cz)
bit2 = (x<=cx, y>cy, z<=cz)    bit3 = (x>cx, y>cy, z<=cz)
bit4 = (x<=cx, y<=cy, z>cz)    bit5 = (x>cx, y<=cy, z>cz)
bit6 = (x<=cx, y>cy, z>cz)     bit7 = (x>cx, y>cy, z>cz)
```

其中 `(cx, cy, cz)` 为该体素中心的坐标；多个点落入同一体素时按位 OR 累加。

若开启 `ProcessInputPointArray`，则同时处理一个输入点属性数组，并把结果作为一个多分量（每分量一个
统计函数，顺序为 LastValue / Min / Max / Count / Sum / Mean）的单元属性数组附加到输出，与 VTK 一致。

> 说明：iGameVis 没有独立的 `vtkImageData` / `vtkPartitionedDataSet` 概念，因此 VTK 的「分片数据集 +
> 单张图像」在此直接映射为一个 `StructuredMesh`；图像的 origin / spacing 通过网格点坐标原样表达。

## 参数

| 参数 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `NumberOfPointsPerCell` | `igIndex64` | `1` | 每个体素期望容纳的平均点数，自动钳制到 >= 1 |
| `ProcessInputPointArray` | `bool` | `false` | 是否处理输入点属性数组并统计 |
| `InputPointArrayName` | `std::string` | 空 | 要处理的点属性名；为空时取第一个 `IG_POINT` 上的标量属性 |
| `ComputeLastValue` | `bool` | `false` | 统计：末值 |
| `ComputeMin` / `ComputeMax` | `bool` | `true` | 统计：最小 / 最大值 |
| `ComputeCount` | `bool` | `true` | 统计：计数 |
| `ComputeSum` | `bool` | `false` | 统计：求和 |
| `ComputeMean` | `bool` | `true` | 统计：均值（开启时自动计算 Count 与 Sum） |

## 调用方式

```cpp
#include <Convert/iGamePointSetToOctreeFilter.h>

auto filter = iGame::PointSetToOctreeFilter::New();
filter->SetInput(input);                          // 输入：PointSet 子类
filter->SetNumberOfPointsPerCell(1);              // 可选，体素含点阈值
// filter->SetProcessInputPointArray(true);       // 可选，统计点属性
// filter->SetInputPointArrayName("field");       // 可选，指定要统计的数组
filter->Execute();

auto out = filter->GetOutput();                   // StructuredMesh，带单元标量 "octree"
// 以点/表面方式显示：SetViewStyle(IG_POINTS) 或 IG_SURFACE
```

## 使用示例

示例程序：`Examples/Filter/Convert/TestPointSetToOctree.cpp`
测试模型：`Examples/Models/OctreePoints.vtk`（球壳 + 内部随机点共 500 点的点云）

示例硬编码相对路径读入点云后自动执行转换，并以点的形式显示八叉树输出。
运行方式（工作目录为 `build-msvc`）：`.\Release\testPointSetToOctree.exe`

## 注意事项

1. **输入类型**：只接受 `PointSet` 及其子类；无输入时返回 `false`。
2. **体素占用编码**：`octree` 数组是 `unsigned char` 的 8 位 bitfield，渲染/查询时按位判断，而不是
   连续的「占用计数」。
3. **统计函数**：`ProcessInputPointArray=true` 时至少需开启一个统计函数，否则 `Execute` 报错；开启
   `ComputeMean` 会自动连带计算 Count 与 Sum。
4. **输出维度**：`dimensions[i] = 各轴分割数 + 1`，与 VTK 一致；原点/spacing 由网格点坐标隐式表达。
5. **输入点数组类型**：统计时按输入数组分量逐分量进行，输出为对应分量数的单元属性数组。