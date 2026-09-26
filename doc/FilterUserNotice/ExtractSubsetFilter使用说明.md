# ExtractSubsetFilter 使用说明

## 功能

`ExtractSubsetFilter` 用于从**结构化网格 (StructuredMesh)** 中提取一个规则的子区域（Volume of Interest，简称 VOI）。它根据给定的 IJK 索引范围，从原始网格中裁剪出一块连续的数据块，输出仍为结构化网格类型。

**支持的三种输出形态：**

| 条件 | 输出形态 |
|------|----------|
| 三个方向均 > 1 个点 | 3D 六面体结构化网格 (StructuredMesh + Volumes) |
| K 方向仅 1 个点，I、J 方向 > 1 | IJ 平面四边形网格 (StructuredMesh + Faces) |
| J 方向仅 1 个点，I、K 方向 > 1 | IK 平面四边形网格 (StructuredMesh + Faces) |
| I 方向仅 1 个点，J、K 方向 > 1 | JK 平面四边形网格 (StructuredMesh + Faces) |

**特性：**
- 自动按 VOI 索引映射拷贝点属性（Point Attributes）
- 自动按 VOI 索引映射拷贝单元属性（Cell Attributes），保留原始数据类型、维度和数值范围
- 支持负值 `maxI/maxJ/maxK`（自动取对应维度的最大索引）

---

## 调用方式

### 头文件

```cpp
#include <ExtractSubset/iGameExtractSubsetFilter.h>
```

### 基本流程

```cpp
// 1. 读取结构化网格数据
iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile("./Models/Structured_Volume_Test.vtk");

// 2. 创建 Filter 并设置输入
auto filter = iGame::ExtractSubsetFilter::New();
filter->SetInput(obj);

// 3. 设置感兴趣区域 (VOI)
// 格式: minI, maxI, minJ, maxJ, minK, maxK
// 注意: max 为点索引值（非 cell 数）
filter->SetVOI(0, 2, 0, 2, 0, 1);

// 4. 执行
if (!filter->Execute()) {
    std::cerr << "Filter 执行失败: " << filter->GetMessage() << "\n";
    return;
}

// 5. 获取输出
auto result = filter->GetOutput();
```

### 主要 API

| 方法 | 说明 |
|------|------|
| `SetVOI(int minI, int maxI, int minJ, int maxJ, int minK, int maxK)` | 设置六维 VOI 索引范围 |
| `SetVOI(int voi[6])` | 用数组形式批量设置 VOI |
| `GetVOI(int voi[6])` | 获取当前 VOI 设置 |
| `Execute()` | 执行子集提取 |

---

## 使用示例

### 示例 1：提取 3D 子区域

```cpp
#include <ExtractSubset/iGameExtractSubsetFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>

int main() {
    auto scene = iGame::Scene::New();

    // 读取 5×4×3 结构化网格
    const std::string fileName = "./Models/Structured_Volume_Test.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "读取文件失败!\n";
        return 0;
    }

    auto filter = iGame::ExtractSubsetFilter::New();
    filter->SetInput(obj);

    // 提取 I:[0,2] J:[0,2] K:[0,1] 区域，共 3×3×2 = 18 个六面体单元
    filter->SetVOI(0, 2, 0, 2, 0, 1);

    if (!filter->Execute()) {
        std::cout << "过滤器执行失败!\n";
        return 0;
    }

    auto result = filter->GetOutput();
    if (result != nullptr) {
        scene->AddModel(result);
    }

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
```

### 示例 2：提取 2D 平面切片

```cpp
    // 从 5×4×3 网格中提取 J=1 的 IK 平面
    // newSize[1] = 1 且 newSize[0]>1 && newSize[2]>1 时输出为四边形面网格
    filter->SetVOI(0, 4, 1, 1, 0, 2);  // I:0~4, J:1~1, K:0~2
```

---

## 注意事项

1. **输入类型限制**：仅接受 `StructuredMesh` 类型输入。若输入为 `nullptr` 或类型不匹配，`Execute()` 返回 `false`。

2. **VOI 索引合法性**：
   - 各维度的 `min` 值必须 ≥ 0
   - 各维度的 `max` 值必须 < 对应维度的点数（维度大小）
   - `min` 必须 ≤ `max`，否则返回 `false`
   - 可使用负值 `max`（如 `-1`），Filter 会自动替换为该维度的最大有效索引

3. **维度大小**：
   - 模型维度声明为 `DIMENSIONS Ni Nj Nk`，实际点数为 `Ni × Nj × Nk`
   - 有效 cell 数为 `(Ni-1) × (Nj-1) × (Nk-1)`（3D 情况下）
   - VOI 中 `maxI` 的最大合法值为 `Ni-1`

4. **输出属性**：
   - 点属性按 VOI 点索引一一映射拷贝，保留原始类型和 DataRange
   - 单元属性按 VOI 单元索引映射拷贝；当年维度切片无对应单元数据时，该单元属性置零

5. **性能**：当 VOI 范围较大时，属性拷贝可能消耗较多内存

6. **测试模型**：`Examples/Models/Structured_Volume_Test.vtk`（5×4×3 结构化网格，含 24 个六面体单元）

---

## 相关 Filter

- `iGameSliceFilter`：沿任意平面切割网格，适合不规则切片
- `iGameClipFilter`：使用方程或隐式函数裁剪网格
- `iGameConvertToVolumeMeshFilter`：将非结构化网格转换为体网格
