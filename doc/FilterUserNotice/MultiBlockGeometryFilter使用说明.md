# MultiBlockGeometryFilter

## 1. Overview (功能概述)

`MultiBlockGeometryFilter` 是专为 iGameVis 平台开发的多块装配体（Multi-Block Dataset / `.vtm`）及传统单网格模型的复合表面网格提取 Filter。

### 核心特性
- **递归复合解构 (Recursive Composite Processing)**：采用组合模式（Composite Pattern）设计，使用深度优先搜索（DFS）自动穿透并遍历多层嵌套装配体结构；
- **实体叶子抽面**：自动对装配树各叶子节点网格调度底层表面提取引擎（`ModelGeometryFilter`）；
- **层级与命名保真 (Hierarchy & Metadata Preservation)**：抽取后的表面重新组装为与原始模型完全一致的装配树层级，完整保留各个子零件的原始名称（Name）、零件层级、节点标量/矢量场及单元属性；
- **渲染容器无缝兼容**：装配体复合节点统一使用 `DrawObject` 容器管理，确保在后续 3D 视口渲染管线中能够正确参与材质着色与拾取，避免空指针异常。

---

## 2. Algorithm & Supported Data (算法逻辑与支持网格)

### 2.1 递归遍历算法
1. 若输入为复合节点（`input->HasSubDataObject() == true`）：
   - 实例化 `DrawObject` 作为输出容器；
   - 继承原始输入节点的名称：`outContainer->SetName(input->GetName())`；
   - 对每一个子数据对象递归调用 `ExtractRecursively`，并将抽出的有效子表面挂载至新容器。
2. 若输入为叶子实体网格：
   - 委托给底层 `ModelGeometryFilter` 进行抽面；
   - 成功后将子表面命名为原始输入名称，返回并由上层容器收集。

### 2.2 支持的网格类型 (Supported Meshes)

| 网格类型 (Mesh Type) | 支持状态 (Support) | 说明 |
| :--- | :--- | :--- |
| **MultiBlock / Composite (`.vtm`)** | 完整支持 | 递归展开、各子块并行抽面并按原结构回装 |
| **非结构网格 (`UnstructuredMesh`)** | 完整支持 | 提取 3D 体单元外表面（四边形/三角形） |
| **混合体网格 (`VolumeMesh`)** | 完整支持 | 支持四面体、六面体、三棱柱、金字塔等实体单元 |
| **表面网格 (`SurfaceMesh`)** | 兼容直通 | 纯表面模型直接提取或流转，保持拓扑完整 |
| **结构化网格 (`StructuredMesh`)** | 完整支持 | 提取外边界结构六面体网格表面 |

### 2.3 支持的 3D 体单元类型 (Cell Types)

| 单元类型 (Cell Type) | VTK 类型代码 | 输出表面形式 |
| :--- | :---: | :--- |
| **Hexahedron (六面体)** | 12 | 四边形（Quad）外表面 |
| **Tetrahedron (四面体)** | 10 | 三角形（Triangle）外表面 |
| **Wedge / Prism (三棱柱)** | 13 | 混合四边形与三角形外表面 |
| **Pyramid (金字塔)** | 14 | 混合四边形与三角形外表面 |
| **Polyhedron (多面体)** | 42 | 任意多边形表面（Polygon） |

---

## 3. 调用方式 (API & Calling Convention)

### 3.1 C++ 核心 API
头文件：`#include <ModelSurface/iGameMultiBlockGeometryFilter.h>`

```cpp
// 1. 工厂模式创建 Filter 实例
auto filter = iGame::MultiBlockGeometryFilter::New();

// 2. 设置输入（支持 MultiBlock 装配体或传统单个 DataObject）
filter->SetInput(inputDataObject);

// 3. 执行表面抽取算法
bool success = filter->Execute();

// 4. 获取抽取结果数据集
iGame::DataObject::Pointer output = filter->GetOutput();
```

### 3.2 桌面端交互触发 (iGameVis Qt UI)
在 iGameVis 桌面端界面中已完成动作绑定：
1. 通过【文件】->【打开】载入 `.vtm` 装配体或 `.vtk` / `.vtu` 网格模型；
2. 在左侧模型树（Model Tree）中点击选中待处理的目标模型；
3. 点击顶部菜单栏：
   ```text
   Filter -> 表面提取 (Surface Extraction)
   ```
4. 抽取结果会自动命名为 `<原名称>_MultiBlockSurface` 并安全挂载至模型树，3D 视口自动刷新渲染。

---

## 4. 使用示例 (Examples)

### 4.1 独立测试用例 (Independent Test Case)
源码路径：
```text
Examples/Filter/TestMultiBlockGeometry.cpp
```

构建与运行方式：
```bash
# 运行编译生成的独立可执行文件
./cmake-build-examples/testMultiBlockGeometry.exe
```

典型调用流程代码：
```cpp
#include <Core/iGameScene.h>
#include <ModelSurface/iGameMultiBlockGeometryFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>

int main() {
    // 读取装配体文件
    const std::string fileName = "./Models/assembly_primitives.vtm";
    iGame::DataObject::Pointer root = iGame::FileIO::ReadFile(fileName);

    // 实例化 Filter 并运行
    auto filter = iGame::MultiBlockGeometryFilter::New();
    filter->SetInput(root);
    if (!filter->Execute()) {
        return -1;
    }

    auto res = filter->GetOutput();

    // 遍历多块部件并添加至渲染场景
    auto scene = iGame::Scene::New();
    if (res->HasSubDataObject()) {
        for (auto it = res->SubDataObjectIteratorBegin(); it != res->SubDataObjectIteratorEnd(); ++it) {
            auto drawObj = iGame::DynamicCast<iGame::DrawObject>(it->second);
            if (drawObj) {
                drawObj->SetViewStyle(IG_SURFACE);
                drawObj->AddViewStyle(IG_WIREFRAME);
                drawObj->ConvertToDrawableData();
                scene->AddModel(it->second);
            }
        }
    }
    return 0;
}
```

---

## 5. 对比验证 (Validation against ParaView)

为验证表面抽取算法的精度与装配体层级保真度，使用相同模型在 **iGameVis** 与 **ParaView** 中进行对比验证。

### 5.1 跨平台装配体格式差异说明
由于各平台对复合装配体引用格式的支持存在差异：
- **ParaView 限制**：严格遵循 VTK XML 规范，其 `vtkXMLMultiBlockDataReader` 仅支持引用 XML 格式族（如 `.vtu`），**不支持 `.vtm` 引用 Legacy ASCII 的 `.vtk` 格式**（直接打开会报 `Could not create reader` 错误）；
- **iGameVis 现状**：目前 `iGameVTMReader` 支持 `.vtm` 挂载 `.vtk` 零件，但当前版本底层 `iGameVTUReader` 解析 XML 非结构网格存在缺陷，**暂不支持 `.vtm` 稳定引用 `.vtu` 格式**；
- **验证方案选择**：统一采用 **`.vtm` + `.vtk`**（即 `assembly_primitives.vtm` 引用 `assembly_cube_hex.vtk` 与 `assembly_wedge_prism.vtk`）作为标准测试数据集。

### 5.2 ParaView 验证复现步骤
由于 ParaView 无法直接打开该 `.vtm` 文件，在 ParaView 中需按以下流程手动构建多块装配树进行等价对比：
1. **手动载入子零件**：在 ParaView 中同时打开 `assembly_cube_hex.vtk` 与 `assembly_wedge_prism.vtk` 并点击 `Apply`；
2. **组装多块结构**：按住 `Ctrl` 键同时选中这两个数据集，应用 `Filters -> Alphabetical -> Group Datasets`，将其手动合并为一个多块装配体（MultiBlockDataSet）；
3. **执行表面提取**：选中生成的 `GroupDatasets1`，应用 `Filters -> Alphabetical -> Extract Surface`，点击 `Apply` 完成表面抽取。

### 5.3 对比验证结果清单
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

## 6. 注意事项与已知限制 (Precautions & Known Issues)

### 6.1 核心设计与使用约定
1. **容器类型规范（渲染生命周期）**：
   - 装配体的中间复合节点在重组时**必须**使用 `iGame::DrawObject::New()`，切忌使用裸 `iGame::DataObject::New()`；
   - *原因*：iGameVis 场景树（`iGame::Scene`）在管理模型与应用着色属性时依赖向下转型 `DynamicCast<DrawObject>`，裸 DataObject 会导致转型失败引发空指针崩溃。
2. **相对路径寻址约定**：
   - `.vtm` 装配体文件内部的 `<DataSet file="..."/>` 使用相对路径时，`iGameVTMReader` 会自动基于 `.vtm` 所在物理路径补齐。请保证装配体文件与实体零件文件相对位置不变。
3. **运行时日志 (Runtime Logging)**：
   - 过滤器的执行耗时、各部件表面抽取面片统计与可能发生的异常均记录在项目标准日志中：
   ```text
   logs/iGame-core-log.txt
   ```

### 6.2 项目已知限制 (Known ModelTree Limitations)
受当前前端模型树（ModelTree）管理架构影响，使用多块装配体时存在以下已知限制：
1. **2D 色标条（ColorBar）范围同步与即时刷新异常**：
   - 多块装配体下各个子部件物理场标量范围可能存在差异，当前界面的 2D 色标条显示可能不准确或未能即时跟随子部件激活而刷新；
   - 追踪详情见 GitHub 仓库 Issue：`[Bug] 多块装配体（MultiBlock）下 2D 色标条（ColorBar）数据范围同步与即时刷新异常 #39`。
2. **子模型暂不支持独立 Filter 操作**：
   - 当前模型树选定机制限制，多块装配体无法单独选择某个子模型执行局部的 Filter 操作；
   - 即使在模型树控件（ModelTree）中单选了某一个子零件，所有 Filter 操作依然会对该装配体的根模型（Root Model）全局生效。
