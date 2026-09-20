# DeflectNormalsFilter 使用说明

## 1. 功能说明

DeflectNormalsFilter 用于根据给定的向量场对网格法向进行偏转偏移。它将每个点的**基准法向**（曲面法向或用户指定的常数法向）与**向量场**按公式计算偏转：

```
ND = normalize(base + strength * V)
```

其中：
- `base` = 曲面法向（自动计算）或用户常数法向
- `V` = 指定向量场在每个点的值
- `strength` = 偏转强度（越大偏转越强）

结果作为新的 3 分量点向量属性输出，属性名固定为 `DeflectedNormals`。

### 应用场景

- 流场可视化：用流速场偏转法向，模拟风/水流对表面朝向的影响
- 法向扰动：用随机或噪声场扰动法向，产生非均匀光照效果
- 法向重定向：将法向从一个方向偏转到另一个方向

### 支持的网格类型

| 网格类型 | 说明 |
|---------|------|
| SurfaceMesh     | 直接计算面法向 |
| VolumeMesh      | 转换为 SurfaceMesh 后计算 |
| StructuredMesh  | 转换为 SurfaceMesh 后计算 |
| UnstructuredMesh| 转换为 SurfaceMesh 后计算 |

> 注意：所有网格类型在内部都会统一转换为可用于法向计算的 SurfaceMesh。

### 向量场挂载位置

| 模式 | 说明 |
|------|------|
| IG_POINT（Auto 模式优先） | 向量场定义在点上，直接使用 |
| IG_CELL  | 向量场定义在单元上，先做 CELL→POINT 平均转换再使用 |

## 2. 调用方式

### 头文件

```cpp
#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
```

### 核心 API

```cpp
// 创建过滤器实例
auto filter = iGame::DeflectNormalsFilter::New();

// 设置输入网格模型
filter->SetInput(mesh);

// 指定向量场属性（按名或按索引）
filter->SetAttributeByName("Velocity");      // 按属性名指定
filter->SetAttributeByIndex(0);               // 按属性索引指定

// 设置偏转强度（默认 1.0）
filter->SetDeflectStrength(1.0f);

// 是否使用用户常数法向（默认 false = 用曲面法向）
filter->SetUseUserNormal(false);

// 设置用户法向（仅当 useUserNormal=true 时生效）
filter->SetUserNormal(0.0, 0.0, 1.0);        // (nx, ny, nz)

// 设置向量场挂载位置（可选，Auto 模式自动判断）
filter->SetVectorFieldAttachment(IG_POINT);   // IG_POINT / IG_CELL / IG_AUTO

// 执行
filter->Execute();
```

### 输出属性

| 属性名 | 类型 | 挂载位置 | 维度 |
|--------|------|---------|------|
| DeflectedNormals | VECTOR | IG_POINT | 3 |

## 3. 使用示例

### 示例 1：球面 + Velocity 场 + 曲面法向

```cpp
#include "FeatureExtraction/iGameDeflectNormalsFilter.h"
#include "iGameFileIO.h"

// 读取球面模型（自带 Velocity 向量场）
auto mesh = iGame::FileIO::ReadFile("./Models/DeflectNormals_SphereSurface.vtk");

// 执行法向偏转
auto filter = iGame::DeflectNormalsFilter::New();
filter->SetInput(mesh);
filter->SetAttributeByName("Velocity");    // 切向流速场
filter->SetDeflectStrength(1.0f);          // 偏转强度
filter->SetUseUserNormal(false);           // 使用曲面法向
filter->Execute();
// 结果：新增属性 "DeflectedNormals"，840 个 3 分量向量
```

### 示例 2：波面 + Position 场 + 用户法向

```cpp
auto mesh = iGame::FileIO::ReadFile("./Models/DeflectNormals_WaveSurface.vtk");

auto filter = iGame::DeflectNormalsFilter::New();
filter->SetInput(mesh);
filter->SetAttributeByName("Position");     // 用点坐标作为向量场
filter->SetDeflectStrength(0.5f);           // 较弱偏转
filter->SetUseUserNormal(true);             // 使用用户法向
filter->SetUserNormal(0.0, 0.0, 1.0);       // 基准法向 = +Z
filter->Execute();
// 结果：每个点的法向从 (0,0,1) 朝 Position 方向偏转
```

### 示例 3：指定向量场挂载位置

```cpp
// 当 Auto 模式不确定时，可手动指定
filter->SetVectorFieldAttachment(IG_POINT);  // 强制从 PointData 读取
filter->SetAttributeByName("Velocity");
filter->Execute();
```

## 4. 注意事项

1. **向量场必须是 3 分量**：向量场属性的维度必须为 3，否则 `Execute()` 返回 false。模型中必须已存在该向量场属性。

2. **属性名匹配**：`SetAttributeByName()` 按名称精确匹配，区分大小写。若找不到指定名称的向量场属性，`Execute()` 会返回 false 并输出错误信息。

3. **CELL→POINT 转换**：当向量场仅存在于 CellData 时，Filter 会自动计算每个点关联单元的平均向量值，转换为 PointData 后再使用。若指定 IG_CELL 但模型中只有 PointData，会报错 "vector field 'xxx' not found on CELL data"。

4. **Auto 模式**：默认情况下 `SetVectorFieldAttachment` 未设置时使用 Auto 模式，优先在 PointData 中查找向量场，找不到再在 CellData 中查找。

5. **strength 范围**：
   - `strength = 0`：结果法向 = 基准法向（无偏转）
   - `strength = 1`：向量场与基准法向等权混合
   - `strength > 1`：向量场主导偏转方向
   - `strength < 0`：反向偏转

6. **用户法向归一化**：用户指定的法向向量无需归一化，Filter 内部会自动归一化。

7. **输出为独立对象**：`GetOutput()` 返回的 DataObject 与输入是同一个对象（原地修改），调用方如需独立副本应自行深拷贝。

8. **测试模型**：本 Filter 提供两个 AI 生成的测试模型：
   - `DeflectNormals_SphereSurface.vtk`：POLYDATA，840 点 1600 三角形，自带 Velocity 向量场
   - `DeflectNormals_WaveSurface.vtk`：POLYDATA，625 点 1152 三角形，自带 Position 向量场

9. **自动测试**：运行 `testDeflectNormals.exe`（无需参数），自动执行 2 组测试（球面曲面法向 + 波面用户法向），打印验证结果并可视化显示。

## 5. 测试模型说明

| 模型文件 | 网格类型 | 点数 | 面数 | 向量场 | 向量场类型 |
|---------|---------|------|------|--------|-----------|
| DeflectNormals_SphereSurface.vtk | POLYDATA | 840 | 1600 | Velocity | 切向流速场 |
| DeflectNormals_WaveSurface.vtk | POLYDATA | 625 | 1152 | Position | 点坐标向量场 |

## 6. 偏转公式说明

```
输入:
  base = 曲面法向 或 用户常数法向 (3D 向量)
  V    = 向量场在当前点的值 (3D 向量)
  s    = 偏转强度 (标量)

计算:
  ND = base + s * V
  ND_normalized = ND / |ND|

输出:
  DeflectedNormals[i] = ND_normalized  (挂载到点, 3 分量)
```

当 `base` 和 `V` 方向相近时，偏转效果较弱；当两者方向垂直时，偏转效果最明显。
