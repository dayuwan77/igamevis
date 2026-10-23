# GroupDatasetsFilter 使用说明

## 1. Overview (功能概述)

`GroupDatasetsFilter` 把用户选中的 **N 个数据对象打包成一个多块复合组（MultiBlock）**，对齐 ParaView 的 **Group Datasets**（`vtkGroupDataSetsFilter`）。

### 核心特性

- **只做容器组合，不合并几何**：每个输入成为输出多块组的一个子块；
- **各子块是输入的深拷贝克隆**：几何（点/单元/面/边）、属性表、显隐样式、配色器、动画帧全部各持一份 —— 新旧对象完全独立，改一边不影响另一边；
- **原始输入零修改**：不挂载、不改名、不删除，父指针保持原样（删组后源对象可正常使用、导出）；
- **允许混合类型**：体网格（`UnstructuredMesh` / `VolumeMesh`）、表面网格（`SurfaceMesh`）、点云可以放进同一个组；
- **多块输入嵌套保留**：输入本身已是多块组时，**嵌套**保留其结构而不展平，因此输出的**顶层块数恒等于输入对象数**；
- **顶层块顺序 = 输入顺序**（与 ParaView 排列行为一致，详见 §3.4）；
- **结果独立成节点**：输出作为新的顶层节点挂到模型树；原始输入保持原状（不自动隐藏）。

---

## 2. 与 Append Datasets 的区别 ⚠️

项目里已有一个多输入 filter `AppendReduceFilter`（菜单「网格合并去重 (Append/Reduce)」），它对应的是 ParaView 的 **Append Datasets**，与本 filter **不是一回事**：

| 对比项 | `AppendReduceFilter`（Append Datasets） | **`GroupDatasetsFilter`（Group Datasets）** |
| :--- | :--- | :--- |
| 输出 | 单个**合并后的网格**（`SurfaceMesh`） | **多块容器**（MultiBlock） |
| 几何 | **真正合并** —— 点/面重新编号 | **不合并** —— 各块保持独立 |
| 与源的关系 | 合并结果是一份新网格 | 各块是输入的**深拷贝克隆**（独立副本） |
| 输入类型 | 必须都是 `SurfaceMesh` | **允许混合类型** |
| 输入范围 | 场景中**全部**模型 | **用户在模型树中选中的**对象 |
| 典型用途 | 把多个零件拼成一个网格 | 把多个模型组织成一个装配体 |

> 一句话区分：**Append 是「粘成一个」，Group 是「装进一个盒子」（盒子里是独立副本）。**

---

## 3. 算法与行为

### 3.1 处理流程

```
① 读取选中的 DataObject（由 UI 层提供）
        ↓
② 确定输出节点名（由 UI 层用唯一命名函数算好）
        ↓
③ 创建多块容器：DrawObject::New()
        ↓
④ 每个输入 → CreateDeepClone：同类型深拷贝克隆
     （几何 / 属性表 / 块映射 / 动画帧 / 显示标量各持一份）
        ↓
⑤ AddSubDataObject(克隆)；随后把克隆的配色器换回
     "源配色器的一份独立拷贝"（不与组共享）
        ↓
⑥ 输出写入 Filter 的输出槽 0
```

> **UI 层额外负责**：挂载到场景与模型树、弹窗汇报。原始输入保持原状。

### 3.2 输入要求与边界

| 情况 | 行为 |
| :--- | :--- |
| 输入个数 = 0 | `Execute()` 返回 `false`，`GetMessage()` 给出提示 |
| 输入个数 = 1 | **允许**，等价于"包一层" |
| 输入含 `nullptr` | 在 `SetInputs()` 阶段被**过滤掉**，不计入个数 |
| 同一对象重复加入 | `SetInputs()` 阶段**按对象指针去重**，只保留 1 块 |
| 输入是多块组 | **嵌套**保留结构，不展平（顶层块数仍按 1 个输入计） |
| 类型暂不支持 / 克隆失败 | 该输入被**跳过**并记入 `GetSkippedNames()`；全部失败时 `Execute()` 返回 `false` |
| 嵌套深度超过 64 | 防御性上限，超出部分跳过（正常装配体远小于此） |

### 3.3 输出结构

- 输出是一个 **多块组**（`DrawObject` 容器，含若干子对象）；
- **顶层块数 == 成功克隆的输入数**；
- 每个块是输入的同类型**深拷贝克隆**：名字、类型、点数 / 单元数 / 属性内容与输入一致，但**数据完全独立** —— 修改克隆（顶点、属性值、显隐、配色器范围）不影响源对象，反之亦然。

### 3.4 块顺序

> **输出的顶层块顺序 = 输入顺序**（与 ParaView 的排列行为一致）。

机制：多块子对象底层用 `std::map<DataObjectId, ...>` 保存，迭代按 **ID 升序**；
而克隆是在 `Execute()` 时**按输入顺序依次创建**的，对象 ID 全局递增 ——
因此顶层块的迭代顺序恰好等于输入顺序。

**编写测试或依赖顺序的代码时**：顺序依赖"按输入顺序创建克隆"这一实现细节，推荐**按名字查找**目标块：

```cpp
// ✅ 推荐：按名字查找（不依赖下标语义）
for (const auto& b : blocks) {
    if (b->GetName() == "group_quad") { /* 找到 */ }
}

// ✅ 可用：按下标取第 i 个成功克隆的输入（当前实现下成立）
auto first = blocks[0];
```

> 重名输入会在报告中按序号编号区分（见 §6.2 限制 4）。

---

## 4. 调用方式 (API & Invocations)

### 4.1 C++ 核心 API

头文件：`#include <GroupDatasets/iGameGroupDatasetsFilter.h>`

```cpp
auto filter = iGame::GroupDatasetsFilter::New();

// 1) 一次性设置全部输入（内部会先按对象指针去重，再过滤 nullptr）
//    传入的 nullptr 会被自动过滤
std::vector<iGame::DataObject::Pointer> inputs = { objA, objB, objC };
filter->SetInputs(inputs);

// 2) 设置输出节点名（由上层算好，可省略 —— 省略时不改名）
filter->SetOutputName("GroupDatasets_1");

// 3) 执行
if (!filter->Execute()) {
    // 失败原因（UTF-8，可直接显示）
    std::cerr << filter->GetMessage() << std::endl;
    return;
}

// 4) 取结果多块组
auto group = filter->GetOutput(0);

// 5) 查询统计
int grouped = filter->GetGroupedCount();                  // 实际合并的对象数
const auto& names = filter->GetBlockNames();              // 各子块名字（按输入顺序）
const auto& skipped = filter->GetSkippedNames();          // 被跳过的输入（类型不支持/克隆失败）
```

> 部分成功时 `Execute()` 仍返回 `true`，`GetMessage()` 会列出被跳过的对象。
> 也可以沿用基类的输入接口手动设置：`filter->SetNumberOfInputs(n); filter->SetInput(i, obj);`

### 4.2 桌面端交互触发 (iGameVis Qt UI)

1. 在左侧**模型树**中选中对象：
   - **多选**：按住 `Ctrl` 或 `Shift` 点击
   - 支持选中**多块装配体的子块**；
2. 点击顶部菜单：**`算法处理` → `合并数据集 (Group Datasets)`**；
3. 无需参数，直接执行；
4. **结果**：
   - 模型树新增顶层节点 **`GroupDatasets_N`**（序号复用最小空缺，删除后可复用）；
   - **原始输入模型保持原状**（不隐藏、不修改；组内是它的独立拷贝，改组不影响原模型）；
   - 弹窗汇报：合并对象数 / 输出节点名 / 子块清单。

---

## 5. 使用示例 (Code Example)

测试用例源码：`Examples/Filter/TestGroupDatasets.cpp`
测试数据模型：`Examples/Models/` 下的 `group_tet.vtk`、`group_hex.vtk`、`group_quad.vtk`

```cpp
#include <GroupDatasets/iGameGroupDatasetsFilter.h>
#include <iGameFileIO.h>

int main() {
    // 1) 准备三个不同类型的模型（体网格 + 体网格 + 面片）
    auto tet  = iGame::FileIO::ReadFile("./Models/group_tet.vtk");
    auto hex  = iGame::FileIO::ReadFile("./Models/group_hex.vtk");
    auto quad = iGame::FileIO::ReadFile("./Models/group_quad.vtk");

    // 2) 组合
    auto filter = iGame::GroupDatasetsFilter::New();
    filter->SetInputs({tet, hex, quad});
    filter->SetOutputName("GroupDatasets_1");
    if (!filter->Execute()) { return -1; }

    // 3) 结果：一个含 3 块的多块组，各块是输入的独立克隆（改块不影响源）
    auto group = filter->GetOutput(0);
    return 0;
}
```

### 自动化测试

```powershell
cmake --build .\cmake-build-examples\ --target testGroupDatasets
cd cmake-build-examples
.\testGroupDatasets.exe
```

**5 组用例、85 项断言**：

| 用例 | 验证内容 |
| :--- | :--- |
| Test 1 基本组合（3 个不同类型） | 块数、块名、类型保留、**克隆不与源共享**（对象/Points 数组/属性表/配色器）、各块之和 == 各输入之和（未合并几何）、无输入被直接挂载 |
| Test 2 多块输入 | 顶层块数 == 输入数（嵌套未展平）、内部结构保留、内部子块也是克隆 |
| Test 3 单输入 + 实改验证 | 单输入允许；**改克隆的顶点坐标 / 属性值 / 显隐 / 配色器范围 → 源与组均不受影响** |
| Test 4 输入校验 | 空输入报错、`nullptr` 被过滤、输出名正确、重复对象去重 |
| Test 5 P1 回归 | 源父指针零修改（组合前/后、删组后）；**删组后对源导出**：`.igcm` 路径存活、`.vtk` 导出成功且 roundtrip 一致 |

---

## 6. 注意事项与已知限制

### 6.1 核心语义约定

1. **不合并几何**：输出是容器，不是合并后的网格。若需要真正合并，请用「网格合并去重 (Append/Reduce)」；
2. **原始数据零修改**：不挂载、不重命名、不删除输入对象；
3. **块顺序 = 输入顺序**（当前实现，详见 §3.4），依赖顺序的代码建议按名字查找。

### 6.2 已知限制

| # | 限制 | 说明 |
| :--- | :--- | :--- |
| 1 | **不支持 `PartitionedDataSetCollection`** | 输出固定为 MultiBlock；ParaView 的 Group Datasets 另有一个 PDC 输出选项，本实现未提供 |
| 2 | **克隆为全量深拷贝** | 每个输入的数据在合组时复制一份（几何+属性+显示+动画帧）：大模型多选合并会有内存与耗时开销 —— 这是"新旧完全隔离"的代价 |
| 3 | **类型覆盖有限** | `StructuredMesh` / `Lagrange` / `Spline` / 编码类等暂不支持，会被跳过（`GetSkippedNames()` 可查） |
| 4 | **重名块不加后缀** | 块名即克隆名（来自源对象），多块没有独立的"块名"槽位；重名在弹窗报告中按序号编号区分 |

### 6.3 与多块装配体的关系

- 输入可以是多块装配体的**子块**（借助模型树的多选接口 `GetSelectedDataObjects()` 取到子块本身）；
- 若输入本身是多块组，则**嵌套**保留（克隆时递归建壳），不会展平 —— 因此 `GroupDatasets` 可以逐层组合：
  `Group(Group(A,B), C)` 的顶层块数是 **2**，不是 3。
