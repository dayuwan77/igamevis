# SurfaceNormalsFilter 使用说明

## 功能
`SurfaceNormalsFilter` 用于计算表面网格的法向量，并按照特征角进行顶点分裂。

- 计算每个面的单位法向量。
- 相邻面法向量夹角大于 30° 时，共享顶点被拆分。
- 相邻面法向量夹角小于等于 30° 时，顶点共享并平滑。
- 输出网格的每个点、每个面都会带新计算的法向量。

## 调用方式

```cpp
#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <iGameFileIO.h>
#include <iGameSurfaceMesh.h>

auto input = iGame::FileIO::ReadFile("model.vtk");
auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(input);
if (mesh == nullptr) {
    // 输入必须是 SurfaceMesh
    return;
}

auto filter = iGame::SurfaceNormalsFilter::New();
filter->SetInput(mesh);

if (!filter->Execute()) {
    // 执行失败
    return;
}

auto output = iGame::DynamicCast<iGame::SurfaceMesh>(filter->GetOutput(0));
```

## 使用示例

参考示例程序：

```
Examples/Filter/SurfaceNormals/TestSurfaceNormalsFilter.cpp
```

示例程序读取模型后执行 filter，并打印输出网格的点法向量和面法向量。

常用测试模型：

- 原测试模型：`Examples/Models/SurfaceNormalsFilter_test.vtk`
- 立方体测试模型：`Examples/Models/SurfaceNormalsFilter_cube.vtk`

## 输出说明
- 输出为新的 `SurfaceMesh`。
- 面数量不变。
- 点数量可能增加，因为尖锐棱边处的顶点会被拆分。
- 输出会重新添加以下属性：
  - 单元属性 `Normals`，3 分量，表示面法向量。
  - 单元属性 `Normals_Magnitude`，表示面法向量模长。
  - 点属性 `Normals`，3 分量，表示点法向量。
  - 点属性 `Normals_Magnitude`，表示点法向量模长。
- 输入中的原始点属性和单元属性会被复制到输出中，旧的 `Normals` 属性会被替换。

## 注意事项
- 输入只能是 `SurfaceMesh`，其他网格类型会直接返回失败。
- 当前特征角固定为 30°，代码中没有提供修改特征角的接口。
- 当前法向平均是直接相加后归一化，没有按面面积加权。
- 输入面环绕方向不一致时，可能产生错误的合并结果或法向抵消。
- 非流形网格上，当前实现存在固定数组越界的风险，不建议用于复杂非流形模型。
- 与 VTK 相比，恰好等于 30° 的边界判定可能略有差异。