# Texture Map to Plane 使用说明

## 1. 功能说明

`TextureMapToPlaneFilter` 为输入网格的每个点生成二维纹理坐标，对应 ParaView 的 **Texture Map to Plane**。输出数组名为 `Texture Coordinates`，数据类型为 `float`，包含 S、T 两个分量。

Filter 提供两种映射方式：

- **自动平面生成**：拟合输入点集的最小二乘平面，在平面内建立坐标轴，并将 S、T 分别归一化到 `[0, 1]`。
- **手动平面参数**：由 `Origin`、`Point1` 和 `Point2` 定义两条映射轴，参数含义与 ParaView 一致。两条轴可以不正交，但长度必须大于零且不能共线。

手动模式按下式计算点 `P` 的纹理坐标：

```text
S = dot(P - Origin, Point1 - Origin) / |Point1 - Origin|²
T = dot(P - Origin, Point2 - Origin) / |Point2 - Origin|²
```

Filter 会生成独立输出，不修改输入网格。它只负责生成纹理坐标，不负责加载纹理图片或设置渲染材质。

## 2. iGameVis 调用方式

1. 打开一个包含点数据的网格模型，并在模型树中选中该模型。
2. 点击一级菜单【算法处理】→【生成平面纹理坐标 (Texture Map to Plane)】。
3. 选择映射方式：
   - 自动模式：保持“自动平面生成”勾选；
   - 手动模式：取消勾选，选择 `XY`、`XZ`、`YZ` 预设，或在“自定义”模式下填写三个控制点。
4. 点击【执行并生成独立输出节点】。
5. 在模型树中选择名称以 `_TextureMapToPlane` 结尾的输出节点。
6. 检查点数据中的 `Texture Coordinates`，并在面板底部查看 S、T 范围。

自动模式不使用面板中的 `Origin`、`Point1` 和 `Point2`；这些参数只在手动模式下生效。

## 3. C++ 调用示例

自动模式：

```cpp
#include <DataProcessing/TextureMapToPlane/iGameTextureMapToPlaneFilter.h>

auto filter = iGame::TextureMapToPlaneFilter::New();
filter->SetInput(inputMesh);
filter->SetAutomaticPlaneGeneration(true);
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << std::endl;
    return;
}

auto output = filter->GetOutput();
auto textureCoordinates = filter->GetTextureCoordinates();
```

手动模式：

```cpp
auto filter = iGame::TextureMapToPlaneFilter::New();
filter->SetInput(inputMesh);
filter->SetAutomaticPlaneGeneration(false);
filter->SetOrigin({0.0f, 0.0f, 0.0f});
filter->SetPoint1({4.0f, 0.0f, 0.0f});
filter->SetPoint2({0.0f, 4.0f, 0.0f});
if (!filter->Execute()) {
    std::cerr << filter->GetLastError() << std::endl;
}
```

## 4. 示例模型与自动测试

| 模型 | 用途 | 预期结果 |
| --- | --- | --- |
| `TextureMapToPlaneSlantedPatch.vtk` | 自动平面拟合 | 9 组纹理坐标，S、T 范围均为 `[0, 1]` |
| `TextureMapToPlaneManualPatch.vtk` | 非正交手动轴 | 逐点结果与 ParaView 一致 |
| `TextureMapToPlaneCurvedSaddle.vtk` | 曲面投影 | 手动 XY 映射满足 `S=X/4`、`T=Y/4`；自动模式输出有限的 `[0, 1]` 坐标 |

录屏使用 `TextureMapToPlaneCurvedSaddle.vtk`。手动模式参数如下：

```text
Origin = (0, 0, 0)
Point1 = (4, 0, 0)
Point2 = (0, 4, 0)
```

自动测试文件：

```text
Examples/Filter/DataProcessing/TextureMapToPlane/TestTextureMapToPlane.cpp
```

测试程序从 `Examples/Models` 读取固定相对路径，无需输入文件名。在构建目录的 `Examples` 下运行：

```text
testTextureMapToPlane.exe
```

测试内容包括自动映射、手动映射、曲面投影、独立输出、输入保持不变、支持的数据类型和无效参数处理。测试程序不使用 `Tet_Plane.vtk`。

## 5. 支持范围

支持 `PointSet` 及以下常用派生类型：

- `PointSet`
- `SurfaceMesh`
- `UnstructuredMesh`
- `VolumeMesh`
- `StructuredMesh`

输出保留输入的几何、拓扑和属性。复合数据、空点集以及不属于 `PointSet` 体系的数据对象不受支持。

## 6. 注意事项

1. 自动模式至少需要三个非共线点；重合点或共线点不能确定二维映射平面。
2. 手动模式中，`Point1` 和 `Point2` 不能等于 `Origin`，三个控制点不能共线。
3. 自动模式对曲面执行整体平面投影，不等同于曲面参数化或 UV 展开，结果可能出现拉伸或重叠。
4. 手动模式不执行归一化，映射范围外的点可能得到小于 0 或大于 1 的坐标。
5. 如果输入已有纹理坐标，输出中的原数组会被新的 `Texture Coordinates` 替换；输入对象不受影响。
6. ParaView 和 iGameVis 均输出单精度纹理坐标，末位数值可能因浮点舍入略有差异。
7. ParaView 在自动模式下可能仍显示全零的手动参数；iGameVis 显示包围盒平面预设。两者都不会在自动计算中使用这些参数。
