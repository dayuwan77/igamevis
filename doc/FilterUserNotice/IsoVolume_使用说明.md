# IsoVolume 等值面体提取 使用说明

## 1. 功能

`IsoVolumeFilter` 提取**标量值落在 `[lower, upper]` 区间内**的体网格,即**两层等值面之间的区域**,等价于 ParaView / VTK 的 `vtkIsoVolume`。

- 对每个单元按标量做三分类:**完全在内**(保留)、**完全在外**(丢弃)、**与等值面相交**(裁剪细分,保留区间内部分)。
- 输入单元与等值面相交时会被细分成更小的单元,因此**输出单元数可能大于输入**(属正常裁剪行为)。
- 支持四面体 / 六面体 / 棱柱 / 金字塔 / 多边形面 / **多面体**等非结构化体网格。

## 2. 调用方式

### 2.1 代码接口(C++)

```cpp
#include <IsoVolume/iGameIsoVolumeFilter.h>

auto filter = iGame::IsoVolumeFilter::New();
filter->SetInput(mesh);                                   // 体网格(UnstructuredMesh / VolumeMesh)
filter->SetIsoScalarData(array, lower, upper, dim);        // array: 点标量数组; lower/upper: 下/上阈值; dim: 取该数组的第几分量
filter->Execute();
auto result = filter->GetOutput();                        // 提取结果(非结构化网格)
```

参数说明：
| 参数 | 含义 |
|---|---|
| `SetInput(obj)` | 输入网格(`UnstructuredMesh`/`VolumeMesh`),需带**点标量** |
| `SetIsoScalarData(array, lower, upper, dim)` | `array`=点标量数组;`lower`/`upper`=下/上阈值;`dim`=选择该数组的第几分量(多分量数组用) |

### 2.2 GUI 调用

菜单 **算法处理 → 等值面体提取 (IsoVolume)**：
- **点属性数组**：选择要使用的点属性(支持多属性);
- **标量分量**：选择该数组的分量(多分量数组可选);
- **lower / upper**：区间上下限(切换数组/分量时自动填 1/3~2/3,可改)。

## 3. 使用示例

完整示例见 `Examples/Filter/TestIsoVolume.cpp`(自动读取 `Examples/Models` 下的测试模型)。

```cpp
// 读取模型(相对路径,无需手动输入)
auto obj = iGame::FileIO::ReadFile("./Models/IsoVolumeTest_RadialShell.vtk");

// 取点标量数组
auto attrs = obj->GetAttributeSet()->GetAllPointAttributes();
auto array = attrs->GetElement(0).pointer;   // 或用 GetName() 按名称取

// 提取 Radius ∈ [0.5, 0.9] 的等值体
auto filter = iGame::IsoVolumeFilter::New();
filter->SetInput(obj);
filter->SetIsoScalarData(array, 0.5, 0.9, 0);
filter->Execute();
auto result = filter->GetOutput();
// result 即两层等值面之间的体网格
```

内置测试模型(`Examples/Models`)：
| 模型 | 说明 | 建议区间 |
|---|---|---|
| `IsoVolumeTest_RadialShell.vtk` | 标量=到球心距离,等值体为球壳 | `Radius` [0.5, 0.9] |
| `IsoVolumeTest_DoubleBlob.vtk` | 两个高斯团,等值体为两个分离区域 | `BlobLevel` [0.7, 0.95] |

## 4. 注意事项

1. **需要点标量**：IsoVolume 按点取标量;如果数据只有**单元标量(cell data)**,请先做 **ConvertToPointData(转换为点数据)** 再用(菜单:算法处理 → 数据转换 → 转换为点数据)。
2. **单分量 / 按分量**：IsoVolume 使用单分量标量;多分量数组需指定 `dim`(或在 GUI 里选「标量分量」)。
3. **建议体网格**：`vtkIsoVolume` 为体单元设计;2D 面网格也能计算,但结果不是"体",且与 ParaView 结果不易对齐,不建议用于 IsoVolume 验证。
4. **多面体支持**：多面体单元(如 Star CCM+ 网格)面/顶点数较多,本 Filter 已使用动态缓冲,可正常裁剪、提取。
5. **输出可能大于输入**：相交单元被细分(1→多块)是正常现象;要得到"清晰的等值体",建议把区间取在能聚焦目标区域的范围,避免过宽的区间。
6. **与 `vtkIsoVolume` 一致性**：在线性体网格(四面体/六面体)+ 点标量下,结果与 ParaView 的 `vtkIsoVolume` 一致(已用体网格数据验证)。

## 5. 相关测试

- `Examples/Filter/TestIsoVolume.cpp`:自动读取两个测试模型并打印输出点/单元数,验证提取结果。
- 测试模型:`Examples/Models/IsoVolumeTest_RadialShell.vtk`、`IsoVolumeTest_DoubleBlob.vtk`。
