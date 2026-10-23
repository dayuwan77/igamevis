# MultiBlockGeometryFilter

## 1. Overview (功能概述)

`MultiBlockGeometryFilter` 是 iGameVis 平台中用于**多块复合装配体（Multi-Block Dataset / `.vtm`）**的表面网格提取 Filter。它递归穿透装配树，对每个实体子零件执行抽面，并**按原结构回装**为一份新的多块装配体结果。

> ⚠️ **本 Filter 仅接受多块装配体输入**。若传入普通单网格模型，`Execute()` 会直接返回 `false` 并通过 `GetMessage()` 给出明确提示（详见第 3 章）。单网格请使用「算法处理 → 数据处理 → 表面提取 (Surface Extraction)」。

### 核心特性
- **递归复合解构 (Recursive Composite Processing)**：采用组合模式（Composite Pattern）设计，使用深度优先搜索（DFS）自动穿透并遍历多层嵌套装配体结构；
- **实体叶子抽面**：自动对装配树各叶子节点网格调度底层表面提取引擎（`ModelGeometryFilter`）；
- **层级与命名保真 (Hierarchy & Metadata Preservation)**：抽取后的表面重新组装为与原始模型完全一致的装配树层级，完整保留各个子零件的原始名称（Name）、零件层级、节点标量/矢量场及单元属性；
- **失败块不丢弃 (No Silent Drop)**：任一子构件抽面失败时，**原零件会被原样保留在输出装配树中**，装配结构不塌陷；同时精确记录**失败块路径**与**失败原因**，供界面弹窗展示；
- **渲染容器无缝兼容**：装配体复合节点统一使用 `DrawObject` 容器管理，确保在后续 3D 视口渲染管线中能够正确参与材质着色与拾取，避免空指针异常。

---

## 2. Algorithm & Supported Data (算法逻辑与支持网格)

### 2.1 递归遍历算法

```
本 Filter 处理的是「装配树」的每一个节点。装配体天然是一棵树：根节点 / 中间装配组 = 分支节点，具体的实体零件 = 叶子节点。
```

1. **分支节点处理**（`input->HasSubDataObject() == true`）：
   - 实例化 `DrawObject` 作为输出容器（**不可用裸 `DataObject`**，原因见第 6.1 节）；
   - 继承原始输入节点的名称：`outContainer->SetName(input->GetName())`；
   - 逐个遍历子数据对象，携带**当前路径**递归调用 `ExtractRecursively`：
     - 子块抽取成功 → 把抽出的表面网格挂载至容器；
     - 子块抽取失败 → **把原始子块挂载至容器**（保留，不丢弃）。
2. **叶子节点处理**（具体实体零件）：
   - **类型前置校验**：必须是 `IG_VOLUME_MESH` / `IG_UNSTRUCTURED_MESH` / `IG_SURFACE_MESH` / `IG_STRUCTURED_MESH` 之一，否则判定失败；
   - 委托给底层 `ModelGeometryFilter` 进行抽面（含 `try-catch` 异常兜底）；
   - **结果后置校验**：由于底层 `ModelGeometryFilter` 在失败时会返回一个**非空但 0 面片的空壳 SurfaceMesh**（其无参 `Execute()` 恒返回 `true`），因此必须额外检查 `DynamicCast<SurfaceMesh>(output)` 成功**且** `GetNumberOfFaces() > 0` 才算真正成功；
   - 成功后将子表面命名为原始输入名称，返回并由上层容器收集。

### 2.2 输入与子块支持范围 (Supported Input & Block Types)

| 层级 | 类型 | 支持状态 | 说明 |
| :--- | :--- | :--- | :--- |
| **根输入** | **MultiBlock / Composite (`.vtm`)** | **必需** | 必须含 ≥1 个子块，否则直接拒绝 |
| 根输入 | 单网格（`.vtk` / `.vtu` 等） | ❌ 拒绝 | 返回 `false`，提示改用普通表面提取 |
| 根输入 | 空指针 / 空装配体（0 子块） | ❌ 拒绝 | 返回 `false` 并记录原因 |
| **子块（叶子）** | 非结构网格 (`UnstructuredMesh`) | ✅ 支持 | 提取 3D 体单元外表面（四边形/三角形） |
| 子块（叶子） | 体网格 (`VolumeMesh`) | ✅ 支持 | 支持四面体、六面体、三棱柱、金字塔等实体单元 |
| 子块（叶子） | 表面网格 (`SurfaceMesh`) | ✅ 支持 | 纯表面模型直接提取或流转，保持拓扑完整 |
| 子块（叶子） | 结构化网格 (`StructuredMesh`) | ✅ 支持 | 提取外边界结构六面体网格表面 |
| 子块（叶子） | 其他类型（`IG_NONE` 等） | ⚠️ 记录失败 | 原样保留 + 记录路径与原因 |
| 子块（叶子） | 空网格（0 单元） | ⚠️ 记录失败 | 原样保留 + 记录 `表面抽取失败或提取面数为0` |

### 2.3 支持的 3D 体单元类型 (Cell Types)

| 单元类型 (Cell Type) | VTK 类型代码 | 输出表面形式 |
| :--- | :---: | :--- |
| **Hexahedron (六面体)** | 12 | 四边形（Quad）外表面 |
| **Tetrahedron (四面体)** | 10 | 三角形（Triangle）外表面 |
| **Wedge / Prism (三棱柱)** | 13 | 混合四边形与三角形外表面 |
| **Pyramid (金字塔)** | 14 | 混合四边形与三角形外表面 |
| **Polyhedron (多面体)** | 42 | 任意多边形表面（Polygon） |

---

## 3. 输入约束与错误处理 (Input Constraints & Error Handling)

本章是本 Filter 的重点设计。错误分为**两个层级**：根模型错误（致命，直接拒绝）与子构件错误（容错，继续执行并报告）。

### 3.1 根模型错误（致命）—— 直接拒绝 + 弹窗提示

以下情况 `Execute()` 返回 `false`，错误原因写入 `GetMessage()`：

| 情况 | `GetMessage()` 内容 | 日志 |
| :--- | :--- | :--- |
| 输入为空指针 | `输入模型为空` | `igError` |
| 输入非多块装配体 / 子块数为 0 | `当前模型不是多块复合装配体，请使用普通单网格表面提取` | `igError` |

Qt 界面会直接把该消息弹窗给用户，引导其改用正确的功能入口。

### 3.2 子构件错误（容错）—— 保留原块 + 汇总报告

以下情况**不会中断整体流程**，失败子块被原样保留，同时写入 `GetFailedBlocks()`：

| 失败原因 (`reason`) | 触发条件 |
| :--- | :--- |
| `子模型为空` | 子块指针为 `nullptr` |
| `非实体网格类型（数据对象不支持抽面）` | 子块 `GetDataObjectType()` 不属于 4 种网格类型 |
| `表面抽取失败或提取面数为0` | 底层算法返回失败，或返回 0 面片空壳 |
| `底层算法异常: <what()>` | 底层抛出 `std::exception` 派生异常 |
| `底层算法未知异常` | 底层抛出非 `std::exception` 异常 |

### 3.3 失败块路径格式

路径在递归过程中逐层拼接，格式为 `/根节点名/子节点名/...`：

```text
/assembly_with_error/assembly_empty_error
```

- **根节点名**来自模型文件主名（由 `igQtFileLoader::OpenFile` 设置）；
- 若某节点 `GetName()` 为空，则以 `null_name` 占位，避免出现 `//` 双斜杠；
- ⚠️ **已知限制**：路径基于节点名称拼接，若装配体内存在**同名兄弟节点**，其路径会完全相同，需结合 Qt 弹窗中的顺序辅助定位。

### 3.4 错误信息查询 API

```cpp
// 根模型错误（仅在 Execute() 返回 false 时有意义）
const std::string& GetMessage() const;

// 子构件错误列表（路径 + 原因）
const std::vector<BlockErrorInfo>& GetFailedBlocks() const;

// 其中：
struct BlockErrorInfo {
    std::string path;    // 失败块路径，如 "/Assembly/partA"
    std::string reason;  // 失败原因，如 "表面抽取失败或提取面数为0"
};
```

> 状态变量 `m_Message` 与 `m_FailedBlocks` 在**每次 `Execute()` 进入时自动清空**，同一实例重复执行不会累积历史错误。

---

## 4. 调用方式 (API & Calling Convention)

### 4.1 C++ 核心 API

头文件：`#include <ModelSurface/iGameMultiBlockGeometryFilter.h>`

```cpp
// 1. 工厂模式创建 Filter 实例
auto filter = iGame::MultiBlockGeometryFilter::New();

// 2. 设置输入（必须是多块装配体，否则 Execute() 直接返回 false）
filter->SetInput(inputDataObject);

// 3. 执行表面抽取算法
if (!filter->Execute()) {
    // 根模型级错误：直接读取原因
    std::cerr << "提取失败: " << filter->GetMessage() << std::endl;
    return;
}

// 4. 获取抽取结果数据集
iGame::DataObject::Pointer output = filter->GetOutput();

// 5. 检查子构件级错误（失败块已被原样保留在 output 中）
for (const auto& block : filter->GetFailedBlocks()) {
    std::cerr << "子块异常 [" << block.path << "]: " << block.reason << std::endl;
}
```

### 4.2 桌面端交互触发 (iGameVis Qt UI)

在 iGameVis 桌面端界面中已完成动作绑定：

```text
算法处理 → 多块模型表面提取
```

使用步骤：
1. 通过【文件】→【打开】载入 `.vtm` 多块装配体；
2. 在左侧模型树（Model Tree）中点击选中该装配体；
3. 点击菜单栏 `算法处理` → `多块模型表面提取`；
4. **交互反馈**：
   - 若选中的不是多块装配体 → 弹窗提示「当前模型不是多块复合装配体，请使用普通单网格表面提取」，并中止执行；
   - 若存在子构件异常 → 正常生成结果后，弹窗逐条列出失败块，例如：
     ```text
     多块表面提取已完成，但以下 1 个子构件处理异常（已为您原样保留）：
     • [/assembly_with_error/assembly_empty_error]: 表面抽取失败或提取面数为0
     ```
5. 抽取结果自动命名为 `<原名称>_MultiBlockSurface` 并挂载至模型树，3D 视口自动刷新渲染。

---

## 5. 使用示例 (Examples)

### 5.1 独立测试用例 (Independent Test Case)

源码路径：
```text
Examples/Filter/TestMultiBlockGeometry.cpp
```

构建与运行方式：
```bash
# 直接运行即可：三组用例（正常/异常/输入校验）全部自动执行，
# 打印断言结果后拉起 3D 交互窗口展示抽取效果
./cmake-build-examples/testMultiBlockGeometry.exe

# 若只想看断言结果、不想弹窗（例如在终端或远程环境），可追加 --selfcheck
./cmake-build-examples/testMultiBlockGeometry.exe --selfcheck
```

> - **`--selfcheck` 不是测试开关**：三组用例在任何模式下都会完整执行，该参数仅控制「跑完后是否打开 3D 窗口」；
> - 退出码约定：`0` = 全部断言通过；`1` = 存在未通过断言或程序异常。

#### 自动化覆盖范围（3 组用例，共 28 项断言）

| 用例 | 输入模型 | 断言要点 |
| :--- | :--- | :--- |
| **① 正常路径** | `assembly_primitives.vtm` | 执行成功；`GetFailedBlocks()` 为空；输出保留 2 个子块；六面体 **10 面 / 12 点**；三棱柱 **8 面 / 8 点** |
| **② 异常路径** | `assembly_with_error.vtm` | 部分成功时仍返回 `true`；**输出仍保留 2 个子块（失败块未丢弃）**；失败报告数量 = 1；失败路径含 `assembly_empty_error`；原因非空；同装配体中的正常零件仍抽取成功；失败零件保留为原始对象 |
| **③ 输入校验** | `assembly_cube_hex.vtk`（单网格）<br>空指针 | 单网格输入返回 `false` 且 `GetMessage()` 非空；空指针输入返回 `false` 且给出原因 |

#### 实测输出样例

> 说明：测试用例的控制台输出统一使用**英文**（与项目其他 Example 一致）。原因是 Windows 控制台默认代码页为 GBK，而源码按 UTF-8 编译，直接输出中文会出现乱码；引入 `<windows.h>` 设置 UTF-8 代码页又会造成 `GetMessage` 宏与 Filter 接口名冲突。
>
> 而 `GetMessage()` / `GetFailedBlocks()` 的返回值仍为**中文**（供 Qt 弹窗展示，Qt 使用 `QString::fromStdString`，不受控制台编码影响）。因此在上面的样例输出中，`failed reason` 与 `rejection reason` 两行若在 GBK 控制台运行可能显示为乱码，这属于控制台编码限制，**不影响任何断言判定**（断言仅校验其非空）。

```text
============================================================
  [iGameVis] MultiBlockGeometryFilter Self-Test
  usage: testMultiBlockGeometry [--selfcheck]
         --selfcheck  run assertions only, skip the 3D window
============================================================

[Test 1] Normal path: extract every block of a multiblock assembly
  file: ./Models/assembly_primitives.vtm
    [PASS] read multiblock assembly
    [PASS] Execute() returns true
    [PASS] no failed block reported
    [PASS] output data object is not null
    [PASS] output keeps 2 sub-blocks
    [PASS] cube block found and is a SurfaceMesh
      * assembly_cube_hex: Points = 12, Faces = 10
    [PASS] cube face count == 10
    [PASS] cube point count == 12
    [PASS] wedge block found and is a SurfaceMesh
      * assembly_wedge_prism: Points = 8, Faces = 8
    [PASS] wedge face count == 8
    [PASS] wedge point count == 8

[Test 2] Error path: tolerate a broken block and report it
  file: ./Models/assembly_with_error.vtm
    [PASS] read assembly containing a broken block
    [PASS] Execute() still returns true on partial success
    [PASS] output data object is not null
    [PASS] failed block preserved (output still has 2 sub-blocks)
    [PASS] failed block report count == 1
      * failed path  : /assembly_with_error/assembly_empty_error
      * failed reason: 表面抽取失败或提取面数为0
    [PASS] failed block path is not empty
    [PASS] failed block path contains the broken part name
    [PASS] failed block reason is not empty
    [PASS] healthy block in the same assembly still extracted (10 faces)
    [PASS] failed block still present in the output tree
    [PASS] failed block kept as the original object (not a SurfaceMesh)

[Test 3] Input validation: a single mesh must be rejected
  file: ./Models/assembly_cube_hex.vtk (single unstructured grid)
    [PASS] read single mesh model
    [PASS] single mesh has no sub-blocks (not a multiblock)
    [PASS] Execute() returns false for single mesh input
    [PASS] rejection reason is provided
      * rejection reason: 当前模型不是多块复合装配体，请使用普通单网格表面提取
    [PASS] Execute() returns false for null input
    [PASS] null input rejection reason is provided

============================================================
  Result: 28 passed, 0 failed
============================================================
[SUCCESS] all assertions passed.
```

### 5.2 测试模型清单 (Test Models)

| 模型文件 | 用途 | 内容 |
| :--- | :--- | :--- |
| `Models/assembly_primitives.vtm` | **正常路径验证** | 引用 `assembly_cube_hex.vtk`（六面体，含温度/应力场）+ `assembly_wedge_prism.vtk`（三棱柱，含压力/应变场） |
| `Models/assembly_with_error.vtm` | **异常路径验证** | 引用 `assembly_cube_hex.vtk`（正常）+ `assembly_empty_error.vtk`（0 单元空网格，必然失败） |
| `Models/assembly_empty_error.vtk` | 异常 fixture | 仅 4 个点、0 个单元的 `UNSTRUCTURED_GRID` |
| `Models/assembly_cube_hex.vtk` | 输入校验 fixture | 单体非结构网格，用于验证「非多块输入被拒绝」 |

---

## 6. 对比验证 (Validation against ParaView)

为验证表面抽取算法的精度与装配体层级保真度，使用相同模型在 **iGameVis** 与 **ParaView** 中进行对比验证。

### 6.1 跨平台装配体格式差异说明
由于各平台对复合装配体引用格式的支持存在差异：
- **ParaView 限制**：严格遵循 VTK XML 规范，其 `vtkXMLMultiBlockDataReader` 仅支持引用 XML 格式族（如 `.vtu`），**不支持 `.vtm` 引用 Legacy ASCII 的 `.vtk` 格式**（直接打开会报 `Could not create reader` 错误）；
- **iGameVis 现状**：目前 `iGameVTMReader` 支持 `.vtm` 挂载 `.vtk` 零件，但当前版本底层 `iGameVTUReader` 解析 XML 非结构网格存在缺陷，**暂不支持 `.vtm` 稳定引用 `.vtu` 格式**；
- **验证方案选择**：统一采用 **`.vtm` + `.vtk`**（即 `assembly_primitives.vtm` 引用 `assembly_cube_hex.vtk` 与 `assembly_wedge_prism.vtk`）作为标准测试数据集。

### 6.2 ParaView 验证复现步骤
由于 ParaView 无法直接打开该 `.vtm` 文件，在 ParaView 中需按以下流程手动构建多块装配树进行等价对比：
1. **手动载入子零件**：在 ParaView 中同时打开 `assembly_cube_hex.vtk` 与 `assembly_wedge_prism.vtk` 并点击 `Apply`；
2. **组装多块结构**：按住 `Ctrl` 键同时选中这两个数据集，应用 `Filters -> Alphabetical -> Group Datasets`，将其手动合并为一个多块装配体（MultiBlockDataSet）；
3. **执行表面提取**：选中生成的 `GroupDatasets1`，应用 `Filters -> Alphabetical -> Extract Surface`，点击 `Apply` 完成表面抽取。

### 6.3 对比验证结果清单
在相同模型（六面体立方体零件 + 三棱柱楔形零件）下，两者的实际提取结果对比如下：

| 子块模型名称 (Block Name) | 原始 3D 单元类型 | 表面网格类型 | ParaView 实测结果 (Information 面板) | iGameVis 实测结果 (测试用例终端输出) | 拓扑与数据一致性 |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **`assembly_cube_hex`** | 六面体 (Hex) | Polygonal Mesh | **Points = 12, Cells = 10** | **Points = 12, Faces = 10** | ✅ 完全一致 |
| **`assembly_wedge_prism`** | 三棱柱 (Wedge) | Polygonal Mesh | **Points = 8, Cells = 8** | **Points = 8, Faces = 8** | ✅ 完全一致 |

> **数据分析**：
> - 六面体部件由 2 个体单元组成（共 12 个面），内部贴合接触面被准确剔除，外露表面精确为 $12 - 2 = 10$ 个四边形面；
> - 三棱柱部件由 2 个体单元组成（共 10 个面），内部贴合接触面被准确剔除，外露表面精确为 $10 - 2 = 8$ 个多边形面；
> - iGameVis 提取出的外表面面片数与顶点数与 ParaView 严格保持 100% 吻合，几何拓扑计算精度达到工业级标准。

---

## 7. 注意事项与已知限制 (Precautions & Known Issues)

### 7.1 核心设计与使用约定

1. **输入必须是多块装配体**：
   - 本 Filter 在入口处强校验 `input->HasSubDataObject() && GetNumberOfSubDataObjects() > 0`；
   - 校验只在**顶层入口**执行一次，内部递归函数不再重复校验——否则递归下探到实体叶子零件（本身没有子块）时会被误判为非法而全部失败。
2. **容器类型规范（渲染生命周期）**：
   - 装配体的中间复合节点在重组时**必须**使用 `iGame::DrawObject::New()`，切忌使用裸 `iGame::DataObject::New()`；
   - *原因*：iGameVis 场景树（`iGame::Scene`）在管理模型与应用着色属性时依赖向下转型 `DynamicCast<DrawObject>`，裸 DataObject 会导致转型失败引发空指针崩溃。
3. **成功判定必须检查面片数**：
   - 底层 `ModelGeometryFilter` 的无参 `Execute()` **恒返回 `true`**，失败时会给出一个非空但 0 面片的空壳 `SurfaceMesh`；
   - 因此判定成功必须同时满足 `DynamicCast<SurfaceMesh>` 成功 **且** `GetNumberOfFaces() > 0`，否则会把空壳误判为成功。
4. **相对路径寻址约定**：
   - `.vtm` 装配体文件内部的 `<DataSet file="..."/>` 使用相对路径时，`iGameVTMReader` 会自动基于 `.vtm` 所在物理路径补齐。请保证装配体文件与实体零件文件相对位置不变。

### 7.2 运行时日志 (Runtime Logging)

过滤器的关键事件均写入项目标准日志：

| 级别 | 内容 | 示例 |
| :--- | :--- | :--- |
| `igError` | 根模型错误 | `[MultiBlockGeometryFilter] Input is not a MultiBlock dataset (no sub-data objects).` |
| `IGAME_CORE_WARN` | 子构件失败（含路径与原因） | `[MultiBlockGeometryFilter] Block failed: path='...', reason='no surface extracted (0 faces)'` |

日志文件路径：
```text
logs/iGame-core-log.txt
```

> 💡 **日志语言约定**：日志宏统一使用**英文**文案，因为 Windows 控制台默认代码页为 GBK，中文日志会出现乱码；而面向用户的 `m_Message` / `reason` 保留**中文**，由 Qt 弹窗正确显示。

### 7.3 项目已知限制 (Known ModelTree Limitations)

受当前前端模型树（ModelTree）管理架构影响，使用多块装配体时存在以下已知限制：

1. **2D 色标条（ColorBar）范围同步与即时刷新异常**：
   - 多块装配体下各个子部件物理场标量范围可能存在差异，当前界面的 2D 色标条显示可能不准确或未能即时跟随子部件激活而刷新；
   - 追踪详情见 GitHub 仓库 Issue：`[Bug] 多块装配体（MultiBlock）下 2D 色标条（ColorBar）数据范围同步与即时刷新异常 #39`。
2. **子模型暂不支持独立 Filter 操作**：
   - 当前模型树选定机制限制，多块装配体无法单独选择某个子模型执行局部的 Filter 操作；
   - 即使在模型树控件（ModelTree）中单选了某一个子零件，所有 Filter 操作依然会对该装配体的根模型（Root Model）全局生效。
3. **同名兄弟节点的路径无法区分**：
   - 失败块路径基于节点名称拼接（如 `/Assembly/partA`）。若装配体内存在同名兄弟节点，其路径字符串会完全相同，需结合弹窗中的条目顺序辅助定位。
