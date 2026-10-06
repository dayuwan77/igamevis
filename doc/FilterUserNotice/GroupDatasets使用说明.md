# GroupDatasetsFilter 使用说明

## 1. Overview (功能概述)

`GroupDatasetsFilter` 把用户选中的 **N 个数据对象打包成一个多块复合组（MultiBlock）**，对齐 ParaView 的 **Group Datasets**（`vtkGroupDataSetsFilter`）。

### 核心特性

- **只做容器组合，不合并几何**：每个输入成为输出多块组的一个子块，子块**直接共享**输入的 `DataObject` 指针；
- **允许混合类型**：体网格（`UnstructuredMesh`）、表面网格（`SurfaceMesh`）、点云可以放进同一个组；
- **多块输入嵌套保留**：输入本身已是多块组时，**嵌套**保留其结构而不展平，因此输出的**顶层块数恒等于输入对象数**；
- **原始数据零修改**：不复制几何、不改块名、不删除输入；
- **结果独立成节点**：输出作为新的顶层节点挂到模型树，原始输入自动隐藏（可在模型树中重新打开）。

---

## 2. 与 Append Datasets 的区别 ⚠️

项目里已有一个多输入 filter `AppendReduceFilter`（菜单「网格合并去重 (Append/Reduce)」），它对应的是 ParaView 的 **Append Datasets**，与本 filter **不是一回事**：

| 对比项 | `AppendReduceFilter`（Append Datasets） | **`GroupDatasetsFilter`（Group Datasets）** |
| :--- | :--- | :--- |
| 输出 | 单个**合并后的网格**（`SurfaceMesh`） | **多块容器**（MultiBlock） |
| 几何 | **真正合并** —— 点/面重新编号 | **不合并** —— 各块保持独立 |
| 输入类型 | 必须都是 `SurfaceMesh` | **允许混合类型** |
| 输入范围 | 场景中**全部**模型 | **用户在模型树中选中的**对象 |
| 典型用途 | 把多个零件拼成一个网格 | 把多个模型组织成一个装配体 |

> 一句话区分：**Append 是「粘成一个」，Group 是「装进一个盒子」。**

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
④ 逐个 AddSubDataObject(obj) —— 共享指针，不复制几何、不改名
        ↓
⑤ 输出写入 Filter 的输出槽 0
```

> **UI 层额外负责**：挂载到场景与模型树、隐藏原始输入、弹窗汇报。

### 3.2 输入要求与边界

| 情况 | 行为 |
| :--- | :--- |
| 输入个数 = 0 | `Execute()` 返回 `false`，`GetMessage()` 给出提示 |
| 输入个数 = 1 | **允许**，等价于"包一层" |
| 输入含 `nullptr` | 在 `SetInputs()` 阶段被**过滤掉**，不计入个数 |
| 同一对象重复加入 | 底层 `AddSubDataObject` 自带去重，只保留 **1 块** |
| 输入是多块组 | **嵌套**保留结构，不展平（顶层块数仍按 1 个输入计） |

### 3.3 输出结构

- 输出是一个 **多块组**（`DrawObject` 容器，含若干子对象）；
- **顶层块数 == 有效输入数**；
- 每个块就是输入对象**本身**（指针相同），因此块的点数 / 单元数 / 属性都与输入完全一致。

### 3.4 ⚠️ 关于**块顺序**（重要）

> **输出的子块顺序由对象内部的 `DataObjectId` 决定，而不是用户的选中顺序。**

原因：多块子对象底层用 `std::map<DataObjectId, DataObject::Pointer>` 保存，
迭代顺序即 **ID 升序**（约等于对象的创建时间），**不是** `AddSubDataObject` 的调用顺序。

**示例**：先加载了 A，之后用某个 filter 生成了 B；用户按 `[B, A]` 的顺序多选并合并，
输出的子块迭代顺序仍为 `[A, B]`（A 的 ID 更小）。

**这与 ParaView 的差异**：ParaView 的 Group Datasets 按**输入顺序**排列块。
本实现按 ID 排列 —— 二者**包含的块完全相同**，只是列举顺序可能不同。

**为什么这样处理**：多块子对象的存储容器被 VTM 读取、表面抽取、渲染等多处依赖，
改动其有序性影响面远超本 filter 范围。多块组本身是**无序集合**，顺序不影响语义正确性。

**编写测试或依赖顺序的代码时**：请**按对象指针查找**目标块，不要用下标。

```cpp
// ❌ 不要假设顺序
auto first = blocks[0];

// ✅ 按指针查找
for (const auto& b : blocks) {
    if (b.GetPointer() == target.GetPointer()) { /* 找到 */ }
}
```

---

## 4. 调用方式 (API & Invocations)

### 4.1 C++ 核心 API

头文件：`#include <GroupDatasets/iGameGroupDatasetsFilter.h>`

```cpp
auto filter = iGame::GroupDatasetsFilter::New();

// 1) 一次性设置全部输入（内部会先 SetNumberOfInputs 再逐个 SetInput）
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
const auto& names = filter->GetBlockNames();              // 各子块名字（按加入顺序）
```

> 也可以沿用基类的输入接口手动设置：
> `filter->SetNumberOfInputs(n); filter->SetInput(i, obj);`

### 4.2 桌面端交互触发 (iGameVis Qt UI)

1. 在左侧**模型树**中选中对象：
   - **多选**：按住 `Ctrl` 或 `Shift` 点击
   - 支持选中**多块装配体的子块**；
2. 点击顶部菜单：**`算法处理` → `合并数据集 (Group Datasets)`**；
3. 无需参数，直接执行；
4. **结果**：
   - 模型树新增顶层节点 **`GroupDatasets_N`**（序号复用最小空缺，删除后可复用）；
   - **原始输入模型自动隐藏**（对象仍保留在场景与模型树中，可重新点亮眼睛图标）；
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

    // 3) 结果：一个含 3 块的多块组，各块与输入是同一对象
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

**4 组用例、49 项断言**：

| 用例 | 验证内容 |
| :--- | :--- |
| Test 1 基本组合 | 块数、块名、类型保留、共享指针、**各块之和 == 各输入之和**（未合并几何）、属性保留 |
| Test 2 多块输入 | 顶层块数 == 输入数（嵌套未展平）、块内部结构保留 |
| Test 3 单输入 | 允许，块数 == 1 |
| Test 4 输入校验 | 空输入报错、`nullptr` 被过滤、输出名正确、重复对象去重 |

---

## 6. 注意事项与已知限制

### 6.1 核心语义约定

1. **不合并几何**：输出是容器，不是合并后的网格。若需要真正合并，请用「网格合并去重 (Append/Reduce)」；
2. **原始数据零修改**：不复制几何、不重命名、不删除输入对象；
3. **块顺序由 `DataObjectId` 决定**（详见 §3.4），依赖顺序的代码请按指针查找。

### 6.2 已知限制

| # | 限制 | 说明 |
| :--- | :--- | :--- |
| 1 | **不支持 `PartitionedDataSetCollection`** | 输出固定为 MultiBlock；ParaView 的 Group Datasets 另有一个 PDC 输出选项，本实现未提供 |
| 2 | **块顺序不可控** | 见 §3.4；如需与 ParaView 严格对齐顺序，需改动多块底层容器（影响面大） |
| 3 | **块与输入共享数据** | 各块是输入的**同一对象**，属于只读共享。任何就地修改都会同时影响原模型 |
| 4 | **重名块不加后缀** | 多块没有独立的"块名"槽位，改名等于改输入对象本身，因此保留原样；重名在弹窗报告中按序号编号区分 |

### 6.3 与多块装配体的关系

- 输入可以是多块装配体的**子块**（借助模型树的多选接口 `GetSelectedDataObjects()` 取到子块本身）；
- 若输入本身是多块组，则**嵌套**保留，不会展平 —— 因此 `GroupDatasets` 可以逐层组合：
  `Group(Group(A,B), C)` 的顶层块数是 **2**，不是 3。
