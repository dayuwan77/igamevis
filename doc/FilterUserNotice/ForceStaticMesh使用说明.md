# ForceStaticMesh 强制静态网格过滤器使用说明

## 功能

`ForceStaticMeshFilter` 对输入网格做**静态缓存管理**：首次执行时把输入网格深拷贝一份作为静态缓存（几何固定），之后只要输入对象不变，重复执行仅更新属性数据，输出始终是同一份缓存网格。适合"几何不变、属性随时间/状态变化"的场景。

核心行为：

- **首次执行**：深拷贝输入网格，构建静态网格缓存。
- **同一输入重复执行**：复用缓存，仅用当前输入的属性（点/单元/场数据）更新缓存，不重建几何。
- **输入对象改变**：即使新模型点/单元数与旧模型相同，也会强制重建缓存，避免误复用旧模型几何。
- **`ForceCacheComputation`**：置为 `true` 时总是强制重建缓存，不复用。
- **不修改原模型**：输出是独立的新网格，原模型保持不变。

## 支持的网格类型

SurfaceMesh、UnstructuredMesh、VolumeMesh、StructuredMesh（及任何 PointSet）。

> 注：当前仅支持单块网格；多块/时序容器（如多文件一并导入或 `.pvd`）暂不支持，请对单个块执行。

## 调用方式

### C++ 接口

```cpp
#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>

auto filter = iGame::ForceStaticMeshFilter::New();
filter->SetInput(mesh);            // mesh 为 DataObject::Pointer
if (filter->Execute()) {
    auto out = filter->GetOutput(); // 静态缓存网格
}
```

可选配置：

```cpp
filter->SetForceCacheComputation(true);  // 强制重建缓存（默认 false）
bool on = filter->GetForceCacheComputation();
```

### GUI 使用

菜单：**滤镜 → 数据处理 (Data Processing) → 强制静态网格 (Force Static Mesh)**

- 需先加载并选中一个网格模型（非网格模型会提示不支持）。
- 首次执行：在模型树中新增 `xxx_ForceStaticMesh` 模型（缓存几何固定）。
- 同一模型再次执行：复用缓存、仅更新属性，不会重复新增模型。
- 切换到另一个模型执行：各自维护一份缓存与输出，不会互相干扰。

## 使用示例

命令行示例（自动运行，无需手动输入参数）：

```bat
# 在 Examples 构建目录下运行（相对路径 ./Models 自动拷贝）
testForceStaticMesh.exe
```

该示例从以下模型文件自动读取并验证：

- `./Models/ForceStaticMesh_TestA.vtk`
- `./Models/ForceStaticMesh_TestB.vtk`

验证内容：

1. 同一输入对象执行两次 → 缓存复用（输出为同一对象）。
2. 切换到点/单元数相同但几何不同的另一对象 → 强制重建缓存（输出为新对象，几何对应新输入）。

运行输出 `Result: PASS` 表示通过。

示例源码：`Examples/Filter/ForceStaticMesh/TestForceStaticMesh.cpp`

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/ForceStaticMesh_TestA.vtk` | 2×2×2 六面体网格（27 点 / 8 单元），位于原点附近 |
| `Examples/Models/ForceStaticMesh_TestB.vtk` | 与 A 同规模的六面体网格，整体偏移到 x=10，标量场值不同 |

两个模型点/单元数相同、几何不同，用于验证"输入对象改变时强制重建缓存"。

## 注意事项

1. **仅单块**：多块容器 / 时序（MultiSubFiles）不支持，请在单个块上使用。
2. **原模型不受影响**：输出为深拷贝缓存，修改原模型属性不会影响已生成的缓存（除非重新执行）。
3. **缓存生命周期**：每个输入模型对应一份缓存；同一模型重复执行只更新属性，不会无限新增模型。
4. **属性类型**：属性深拷贝支持 float/double/整型等全部数组类型。
5. **StructuredMesh**：单元数由维度隐式决定，缓存有效性按网格类型正确计算。
