# RandomAttributesFilter 使用说明

## 1. 功能说明

RandomAttributesFilter 用于为网格模型（DataObject）生成随机标量属性。它支持将随机标量挂载到**点（POINT）**或**单元（CELL）**上，可用于：

- 为没有属性数据的模型添加可视化着色属性
- 测试渲染管线对标量属性的显示效果
- 生成随机分布的标量数据用于算法验证

### 支持的网格类型

| 网格类型 | IG_POINT（挂载到点） | IG_CELL（挂载到单元） |
|---------|---------------------|---------------------|
| PointSet        | 支持（点数 = GetNumberOfPoints） | 不支持 |
| SurfaceMesh     | 支持 | 支持（面数 = GetNumberOfFaces） |
| VolumeMesh      | 支持 | 支持（体数 = GetNumberOfCells） |
| StructuredMesh  | 支持 | 支持（cell 数 = (nx-1)*(ny-1)*(nz-1)） |
| UnstructuredMesh| 支持 | 支持（单元数 = GetNumberOfCells） |

### 支持的数据类型

通过 `SetDataType()` 可指定 10 种标量数据类型：

| 类型 | IG 常量 | C++ 类型 |
|------|---------|---------|
| 字符 | IG_CHAR | char |
| 无符号字符 | IG_UNSIGNED_CHAR | unsigned char |
| 短整型 | IG_SHORT | short |
| 无符号短整型 | IG_UNSIGNED_SHORT | unsigned short |
| 整型 | IG_INT | int |
| 无符号整型 | IG_UNSIGNED_INT | unsigned int |
| 长整型 | IG_LONG_LONG | long long |
| 无符号长整型 | IG_UNSIGNED_LONG_LONG | unsigned long long |
| 单精度浮点 | IG_FLOAT | float（默认） |
| 双精度浮点 | IG_DOUBLE | double |

## 2. 调用方式

### 头文件

```cpp
#include "DataProcessing/iGameRandomAttributesFilter.h"
```

### 核心 API

```cpp
// 创建过滤器实例
auto filter = iGame::RandomAttributesFilter::New();

// 设置输入网格模型
filter->SetInput(mesh);

// 设置随机值范围（默认 0.0 ~ 1.0）
filter->SetRange(0.0f, 255.0f);

// 设置随机种子（固定种子可复现结果）
filter->SetSeed(42u);

// 设置挂载位置：IG_POINT（点）或 IG_CELL（单元）
filter->SetAttachmentType(IG_POINT);

// 设置数据类型（默认 IG_FLOAT）
filter->SetDataType(IG_FLOAT);

// 设置属性名（可选，默认自动命名）
filter->SetAttributeName("MyRandomScalar");

// 执行
filter->Execute();
```

### 输出属性名

- 未指定属性名时，自动命名为：
  - `RandomPointScalars`（IG_POINT 模式）
  - `RandomCellScalars`（IG_CELL 模式）
- 指定属性名时，使用自定义名称

## 3. 使用示例

### 示例 1：为四面体网格生成点随机标量

```cpp
#include "DataProcessing/iGameRandomAttributesFilter.h"
#include "iGameFileIO.h"

// 读取模型
auto mesh = iGame::FileIO::ReadFile("./Models/RandomAttributes_TetraMesh.vtk");

// 生成随机标量（挂载到点）
auto filter = iGame::RandomAttributesFilter::New();
filter->SetInput(mesh);
filter->SetRange(0.0f, 255.0f);
filter->SetSeed(42u);
filter->SetAttachmentType(IG_POINT);
filter->Execute();
// 结果：mesh 的 AttributeSet 中新增属性 "RandomPointScalars"
//       元素个数 = 点数 (125)
```

### 示例 2：为结构化网格生成单元随机标量

```cpp
auto mesh = iGame::FileIO::ReadFile("./Models/RandomAttributes_StructuredGrid.vtk");

auto filter = iGame::RandomAttributesFilter::New();
filter->SetInput(mesh);
filter->SetRange(0.0f, 1.0f);
filter->SetSeed(100u);
filter->SetAttachmentType(IG_CELL);
filter->Execute();
// 结果：新增属性 "RandomCellScalars"
//       元素个数 = 单元数 (343)
```

### 示例 3：使用整数类型

```cpp
filter->SetDataType(IG_INT);
filter->SetRange(0.0f, 1000.0f);
filter->SetSeed(999u);
filter->SetAttachmentType(IG_CELL);
filter->Execute();
// 结果：生成 int 型随机标量，值域 [0, 1000]
```

## 4. 注意事项

1. **必须设置 AttachmentType**：`IG_POINT` 和 `IG_CELL` 是全局枚举（定义在 `iGameType.h`），不是 `iGame::` 作用域下的。调用方必须在 `Execute()` 前设置此项，否则使用默认值。

2. **IG_CELL 对 PointSet 不可用**：纯 PointSet 类型没有单元/面/体数据，若对 PointSet 设置 `IG_CELL` 会导致 `Execute()` 返回 false。

3. **固定种子保证可复现**：设置相同 seed 时，每次运行生成的随机序列完全相同，便于测试验证。不设置 seed 则使用默认随机种子。

4. **随机范围**：`SetRange(min, max)` 中 min 和 max 都可以是负数或浮点数。对于整型数据类型，生成的值会自动截断为整数。

5. **属性覆盖**：如果 AttributeSet 中已存在同名属性，新属性会被追加（不会覆盖），建议使用不同属性名或检查已有属性。

6. **测试模型**：本 Filter 提供两个 AI 生成的测试模型：
   - `RandomAttributes_TetraMesh.vtk`：UNSTRUCTURED_GRID，125 点 384 四面体，用于测试点/单元两种模式
   - `RandomAttributes_StructuredGrid.vtk`：STRUCTURED_GRID，512 点，用于测试结构化网格

7. **自动测试**：运行 `testRandomAttributes.exe`（无需参数），自动执行 4 组测试并打印结果，最后可视化显示。

## 5. 测试模型说明

| 模型文件 | 网格类型 | 点数 | 单元数 | 用途 |
|---------|---------|------|--------|------|
| RandomAttributes_TetraMesh.vtk | UNSTRUCTURED_GRID | 125 | 384 | 点/单元随机标量测试 |
| RandomAttributes_StructuredGrid.vtk | STRUCTURED_GRID | 512 | 343 | 结构化网格随机标量测试 |
