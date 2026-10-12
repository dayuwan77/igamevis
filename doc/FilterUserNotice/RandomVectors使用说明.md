# RandomVectors 随机向量过滤器使用说明

## 功能

`RandomVectorsFilter` 为输入网格的每个点生成随机向量，作为 `IG_VECTOR` 点属性输出。向量方向为随机单位向量，模长均匀分布在 [最小速度, 最大速度] 区间。

- 输出属性名：`BrownianVectors`（3 分量 float 向量）
- **不修改原模型**：过滤器深拷贝输入网格，随机向量只加到新网格上，输出独立的新模型
- 输出命名：`原模型名_RandomVectors`

## 支持的网格类型

SurfaceMesh、UnstructuredMesh、VolumeMesh、StructuredMesh（及任何 PointSet）。

> 注：当前仅支持单块网格；多块/时序容器暂不支持，请对单个块执行。

## 调用方式

### C++ 接口

```cpp
#include <AttributeManipulation/iGameRandomVectorsFilter.h>

auto filter = iGame::RandomVectorsFilter::New();
filter->SetMinimumSpeed(0.0);   // 最小模长
filter->SetMaximumSpeed(1.0);   // 最大模长
filter->SetInput(mesh);         // mesh 为 DataObject::Pointer
if (filter->Execute()) {
    auto out = filter->GetOutput(); // 带 BrownianVectors 属性的新网格
}
```

可选配置：

```cpp
filter->SetMinimumSpeed(0.0);
filter->SetMaximumSpeed(2.0);
double minS = filter->GetMinimumSpeed();
double maxS = filter->GetMaximumSpeed();
```

### GUI 使用

菜单：**算法处理 → 数据属性操作 (Attribute Manipulation) → 随机向量 (Random Vectors)**

- 需先加载并选中一个网格模型（非网格模型会提示不支持）。
- 配置：最小速度 / 最大速度。
- 执行后：模型树出现新模型 `xxx_RandomVectors`，展开可见 `BrownianVectors` 属性。
- 打开"矢量场"面板选择 `BrownianVectors` 可查看每个点的随机向量。

## 使用示例

命令行示例（自动运行，无需手动输入参数）：

```bat
# 在 Examples 构建目录下运行（相对路径 ./Models 自动拷贝）
testRandomVectors.exe
```

该示例从 `./Models/RandomVectors_TestA.vtk` 自动读取并验证：

1. 输出为新模型（非原模型）。
2. `BrownianVectors` 属性存在，维度为 3，元素数与点数一致。
3. 每个向量模长落在 [最小速度, 最大速度] 区间。
4. 原模型不被修改（不含 `BrownianVectors`）。

运行输出 `Result: PASS` 表示通过。

示例源码：`Examples/Filter/AttributeManipulation/TestRandomVectors.cpp`

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/RandomVectors_TestA.vtk` | 2×2×2 六面体网格（27 点 / 8 单元），位于原点附近 |
| `Examples/Models/RandomVectors_TestB.vtk` | 与 A 同规模的六面体网格，偏移到 y=10，标量场值不同 |

## 注意事项

1. **仅单块**：多块容器 / 时序（MultiSubFiles）不支持，请在单个块上使用。
2. **不修改原模型**：输出为深拷贝新网格，原模型保持原样。
3. **非确定性**：随机向量由全局随机数流生成，每次执行结果不同（无可复现种子）。
4. **速度范围**：`MinimumSpeed` 应 ≤ `MaximumSpeed`，且建议 ≥ 0。
5. **属性类型**：输出为 3 分量 `FloatArray`，属性深拷贝支持 float/double/整型等全部数组类型。
