# DecimatePolylineFilter 使用说明

## 1. 功能说明

`DecimatePolylineFilter` 用于减少折线中的点，同时尽量保持折线原有形状。过滤器逐条处理输入中的线单元，根据局部误差反复删除误差较小的内部点，折线首尾端点始终保留。

主要用途包括：

- 减少高密度采样曲线的点数；
- 降低折线模型的存储量和渲染开销；
- 在指定几何误差或点字段误差范围内简化轨迹、轮廓线和采样线；
- 使用与 ParaView `Decimate Polyline` 相同的参数进行结果对比。

过滤器支持多条折线输入，每条折线独立计算缩减率和保留点。

## 2. 输入与输出

### 2.1 支持的输入

- `SurfaceMesh` 中保存的显式折线；
- `UnstructuredMesh` 中类型为 `IG_LINE` 或 `IG_POLY_LINE` 的单元。

`SurfaceMesh` 由多边形面自动生成的拓扑边不属于显式折线，不会被当作折线简化。`UnstructuredMesh` 中的非线单元会被忽略；没有有效线单元时，`CanProcessInput()` 和 `Execute()` 返回 `false`。

### 2.2 输出

输出为新的 `UnstructuredMesh`，不会修改输入模型：

- 保留两个点时输出 `IG_LINE`；
- 保留三个及以上点时输出 `IG_POLY_LINE`；
- 只写入实际使用的点，并重新建立紧凑点编号；
- 按来源编号复制保留点的 Point Data 和折线的 Cell Data；
- 输出可以再次作为 `DecimatePolylineFilter` 的输入。

ParaView 的对应过滤器输出 `vtkPolyData`。当前 iGameVis 的 `SurfaceMesh` 不能完整表达和渲染一个多点 `IG_POLY_LINE`，因此本过滤器使用能够保存线单元类型的 `UnstructuredMesh`。两者的数据对象类型不同，但简化策略、保留点及主要参数行为向 ParaView 对齐。

## 3. 参数说明

| 参数或接口 | 默认值 | 说明 |
| --- | --- | --- |
| `SetTargetReduction(value)` | `0.9` | 希望删除的点数比例，自动限制到 `[0, 1]`。例如 `0.5` 表示尝试删除约 50% 的点 |
| `SetMaximumError(value)` | `double` 最大值 | 允许删除候选点的最大局部误差。误差超过该值的点不会进入删除队列 |
| `SetDecimationStrategy(value)` | `Distance` | 设置 `Angle`、`CustomField` 或 `Distance` 策略 |
| `SetCustomFieldName(name)` | 空 | `CustomField` 策略使用的点属性数组名称 |
| `CanProcessInput(input)` | - | 检查输入中是否存在可处理的显式折线 |
| `Execute()` | - | 执行简化，成功返回 `true` |

目标缩减率是期望值。折线点数是整数，并且端点不能删除，因此实际删除比例可能略高于或低于目标值。例如 151 个点使用 `TargetReduction = 0.5` 时输出 75 个点。

### 3.1 Distance

计算候选点到前后邻点所成无限直线的距离平方。距离越小，说明该点越接近局部直线，越优先删除。

### 3.2 Angle

计算候选点两侧线段夹角对应的余弦误差。折线越接近直线，误差优先级越高，越早被删除。

### 3.3 CustomField

读取指定点属性，计算候选点及其前后邻点之间所有分量的最大差值。字段变化越小的点越优先删除。

自定义字段必须满足：

- 为有效数值型 Point Data；
- 数组名称非空；
- 元素数量不少于输入点数；
- 数组类型受到 iGame 数值数组支持。

## 4. iGameVis 调用方式

1. 在 iGameVis 中打开带显式折线的 VTK 或 OBJ 模型。
2. 在模型树中选中需要处理的模型。
3. 打开“算法处理 → 数据处理 → 折线简化 (Decimate Polyline)”。
4. 设置目标缩减率、最大误差和简化策略。
5. 使用 `Custom Field` 时，在“点字段”中选择有效的数值型点属性。
6. 点击执行。成功后，输出作为独立模型加入模型树，原模型会被隐藏，可通过模型树重新显示并进行叠加对比。

界面会将输出显示为点和线框，以便同时观察保留点及折线连接关系。当前模型不包含有效折线、参数非法或自定义字段不满足要求时，会弹出提示，不生成输出节点。

## 5. C++ 使用示例

### 5.1 距离策略

```cpp
#include <DataProcessing/iGameDecimatePolylineFilter.h>
#include <iGameFileIO.h>

#include <limits>

auto input = iGame::FileIO::ReadFile(
        "./Models/DecimatePolyline_Curve.vtk");
if (!input || !iGame::DecimatePolylineFilter::CanProcessInput(input)) {
    return 1;
}

auto filter = iGame::DecimatePolylineFilter::New();
filter->SetInput(input);
filter->SetTargetReduction(0.5);
filter->SetMaximumError(std::numeric_limits<double>::max());
filter->SetDecimationStrategy(
        iGame::DecimatePolylineFilter::DecimationStrategy::Distance);

if (!filter->Execute()) {
    return 1;
}

auto output = filter->GetOutput();
```

### 5.2 自定义字段策略

```cpp
auto filter = iGame::DecimatePolylineFilter::New();
filter->SetInput(input);
filter->SetTargetReduction(0.5);
filter->SetMaximumError(1.0);
filter->SetDecimationStrategy(
        iGame::DecimatePolylineFilter::DecimationStrategy::CustomField);
filter->SetCustomFieldName("Temperature");

if (!filter->Execute()) {
    // 输入没有 Temperature 点属性、数组长度不足或类型不支持。
    return 1;
}
```

## 6. 测试模型与示例

- 示例源码：`Examples/Filter/DataProcessing/DecimatePolyline/TestDecimatePolyline.cpp`
- 测试模型：`Examples/Models/DecimatePolyline_Curve.vtk`
- CMake 目标：`testDecimatePolyline`
- 运行时模型路径：`Models/DecimatePolyline_Curve.vtk`

测试模型包含 151 个折线点和 `OriginalPointId` 点属性。测试使用 Distance 策略和 `TargetReduction = 0.5`，校验以下内容：

- 第一次简化结果包含 75 个点和一个 `IG_POLY_LINE`；
- 保留点的 `OriginalPointId` 与 ParaView 参考结果一致；
- 输出能够再次作为输入；
- 第二次使用 `TargetReduction = 0.5` 后保留 37 个点。

在仓库根目录编译：

```powershell
cmake --build build --config Release `
  --target testDecimatePolyline --parallel 8
```

运行自动测试：

```powershell
ctest --test-dir build/Examples `
  -C Release `
  -R "^testDecimatePolyline$" `
  --output-on-failure
```

运行可视化示例：

```powershell
cd build/Examples
.\Release\testDecimatePolyline.exe --visualize
```

可视化模式会打开 1280 × 720 的渲染窗口，以点和线框方式显示第一次简化后的 75 点折线。关闭窗口后程序结束。不带 `--visualize` 参数时只执行自动断言，不打开窗口。

## 7. 注意事项

1. **输入必须包含线单元。** `DATASET POLYDATA` 只是 VTK 数据集声明，不表示其中一定存在 `LINES`；只有 `POLYGONS` 的模型不能作为折线处理。
2. **不要把表面三角形的边界当作折线。** `SurfaceMesh` 从面拓扑自动构建的边会被拒绝，避免将三角形或多边形轮廓错误简化。
3. **混合 `LINES + POLYGONS` 存在 reader 限制。** 当前 legacy VTK reader 可能复用边和面数组。过滤器检测到边、面指向同一数组时会拒绝输入，避免静默处理错误数据；仅修改 Filter 无法恢复 reader 已覆盖的折线。
4. **端点不会删除。** 开放折线至少保留两个点；首尾使用同一点编号表示的闭合折线至少保留闭合所需的点。
5. **最大误差会使简化提前停止。** 当队列中没有误差小于等于 `MaximumError` 的候选点时，即使尚未达到目标缩减率也会结束。
6. **多条折线分别简化。** `TargetReduction` 对每条折线独立计算，不是对整个数据集统一分配删除数量。
7. **属性按来源下标复制。** Point Data 只复制保留点对应的元素，Cell Data 按输出折线的来源单元复制；不支持的数组类型会被跳过。
8. **输出类型与 ParaView 不同。** iGameVis 输出 `UnstructuredMesh`，ParaView 输出 `vtkPolyData`。不要仅用数据对象类型判断是否对齐，应比较保留点、折线连接、属性和参数结果。
9. **可视化运行会阻塞。** 使用 `--visualize` 时，程序会一直运行到渲染窗口被关闭；CTest 不传该参数，因此不会等待窗口。
