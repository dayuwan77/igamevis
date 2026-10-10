# AppendAttributes（追加属性）使用说明

## 1. 功能

`AppendAttributes` 用于把**多个输入数据对象的属性数组合并到同一个数据对象上**，对应 VTK 的 `vtkAppendAttributes` / ParaView 的「Append Attributes」过滤器。

它**不修改几何**：所有输入的点数（以及单元数）必须逐项对应，输出直接采用**第一个输入**的几何与数据类型，然后把各个输入中出现的点属性 / 单元属性依次复制进来。

**主要特性：**

- 输出几何（点坐标、单元连接关系、网格类型）完全取自输入 #0
- 点属性、单元属性可分别开关，对应 ParaView 的 Field Associations
- 同名 + 同类（名称相同且归属同为点或同为单元）的数组**全部保留**，但一个配对里只有一份能留用原名：靠后的输入保留原名，靠前的输入依次改名为 `<原名>_input_<模型序号>`（序号从 1 开始），与 ParaView 的 Append Attributes 观感一致
- 属性深拷贝进输出，原输入模型不被修改；输出为**独立新节点**
- 输出节点命名：`<输入#0 名称>_appended`

## 2. 输入 / 输出

| 项 | 说明 |
|---|---|
| 输入 | ≥ 1 个数据对象，支持 `PointSet` / `SurfaceMesh` / `VolumeMesh` / `UnstructuredMesh` / `StructuredMesh`；也接受 `DrawObject` 包装与复合（多块）数据对象，内部会自动展平 |
| 校验 | 各输入的点数与单元数必须与输入 #0 **逐项对应**，否则 `Execute()` 返回 `false`；数据类型不同时给出警告，仍按输入 #0 的类型输出 |
| 输出 | 输入 #0 几何的深拷贝 + 合并后的 `AttributeSet`，名称后缀 `_appended` |

## 3. 调用方式

### 头文件

```cpp
#include <AppendAttributes/iGameAppendAttributesFilter.h>
```

### 基本流程

```cpp
// 1. 读取需要合并属性的多个数据对象
iGame::DataObject::Pointer objA = iGame::FileIO::ReadFile("./Models/append_grid_a.vtk");
iGame::DataObject::Pointer objB = iGame::FileIO::ReadFile("./Models/append_grid_b.vtk");

// 2. 创建 Filter，逐个追加输入（第一个输入决定输出几何）
auto filter = iGame::AppendAttributes::New();
filter->AddInput(objA);
filter->AddInput(objB);

// 3. 选择要追加哪些关联（对应 ParaView 的 Field Associations），二者默认均为 true
filter->SetAppendPointData(true);   // 追加点属性
filter->SetAppendCellData(true);    // 追加单元属性

// 4. 执行
if (!filter->Execute()) {
    std::cerr << "Filter 执行失败\n";
    return 1;
}

// 5. 获取输出
auto output = filter->GetOutput();
```

### 主要 API

| 方法 | 说明 |
|------|------|
| `AddInput(DataObject::Pointer data)` | 追加一个输入；首次调用会占用输入槽 0，之后自动扩展输入数量 |
| `SetAppendPointData(bool enable)` | 是否追加点属性（默认 `true`） |
| `GetAppendPointData()` | 获取当前点属性开关 |
| `SetAppendCellData(bool enable)` | 是否追加单元属性（默认 `true`） |
| `GetAppendCellData()` | 获取当前单元属性开关 |
| `GetNumberOfCollectedInputs()` | 最近一次执行收集到的有效输入个数（供界面提示用） |
| `Execute()` | 执行属性合并，成功返回 `true` |
| `GetOutput()` | 获取合并结果数据对象 |

## 4. 使用示例

### 示例 1：C++ 合并两个结构网格的属性

```cpp
#include <AppendAttributes/iGameAppendAttributesFilter.h>
#include <iGameFileIO.h>
#include <iGameScene.h>
#include <iostream>

int main() {
    auto scene = iGame::Scene::New();

    // 两个点数 / 单元数完全一致的 11x11x3 结构网格
    auto objA = iGame::FileIO::ReadFile("./Models/append_grid_a.vtk");
    auto objB = iGame::FileIO::ReadFile("./Models/append_grid_b.vtk");
    if (!objA || !objB) {
        std::cout << "读取文件失败!\n";
        return 1;
    }

    auto filter = iGame::AppendAttributes::New();
    filter->AddInput(objA);
    filter->AddInput(objB);
    filter->SetAppendPointData(true);
    filter->SetAppendCellData(true);

    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        return 1;
    }

    auto result = filter->GetOutput();
    scene->AddModel(result);   // 输出节点名：append_grid_a_appended

    // 查看合并后的属性（同名属性全部保留，靠前的输入带 _input_N 后缀）
    auto attrSet = result->GetAttributeSet();
    auto pointAttrs = attrSet->GetAllPointAttributes();
    for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        auto& attr = pointAttrs->GetElement(i);
        if (attr.IsNone()) { continue; }
        std::cout << attr.pointer->GetName()
                  << " (tuples=" << attr.pointer->GetNumberOfElements()
                  << ", comps=" << attr.pointer->GetDimension() << ")\n";
    }
    return 0;
}
```

以仓库自带测试模型为例，输入属性如下：

| 数据对象 | 点属性 | 单元属性 |
|---|---|---|
| `append_grid_a.vtk` | `pressureA`、`shared` | `matA` |
| `append_grid_b.vtk` | `temperatureB`、`shared` | `matB` |

合并后输出包含点属性 `pressureA`、`shared_input_1`、`temperatureB` 与单元属性 `matA`、`matB`。

两个输入都带同名点属性 `shared`：按 ParaView 的约定，第二个输入（`append_grid_b.vtk`）保留原名 `shared`，第一个输入（`append_grid_a.vtk`）的那一份改名为 `shared_input_1`（后缀里的 `1` 是模型序号，从 1 开始）。

### 示例 2：只合并点属性

```cpp
auto filter = iGame::AppendAttributes::New();
filter->AddInput(objA);
filter->AddInput(objB);
filter->SetAppendPointData(true);
filter->SetAppendCellData(false);   // 丢弃所有单元属性
filter->Execute();
```

### 示例 3：一次合并多个输入

```cpp
auto filter = iGame::AppendAttributes::New();
for (const auto& obj : objects) {   // objects 中所有对象的点数 / 单元数必须一致
    filter->AddInput(obj);
}
if (!filter->Execute()) { return 1; }
std::cout << "本次收集到 " << filter->GetNumberOfCollectedInputs() << " 个有效输入\n";
```

## 5. GUI 使用方法

1. 在 iGameVis 中加载需要合并属性的多个模型。
2. 打开一级菜单「算法处理」。
3. 点击「追加属性 (Append Attributes)」。
4. 在弹出的对话框中勾选参与合并的数据对象（界面会显示各对象的点数 / 单元数，便于确认是否逐项对应），并勾选「追加点属性 (Point Data)」「追加单元属性 (Cell Data)」。
5. 点击「确定」执行：模型树新增输出节点 `<输入#0 名称>_appended`，可继续在标量面板中按合并后的数组着色。

## 6. 自动测试示例

- 示例源码：`Examples/Filter/AppendAttributes/TestAppendAttributes.cpp`
- 测试模型：`Examples/Models/append_grid_a.vtk`、`Examples/Models/append_grid_b.vtk`
- 运行时相对路径：`./Models/append_grid_a.vtk`、`./Models/append_grid_b.vtk`
- CMake 目标：`testAppendAttributes`

```
cmake --build <build-dir> --target testAppendAttributes
cd <build-dir>
./testAppendAttributes.exe
```

示例内部写死相对路径，运行时无需手动输入文件名或参数；程序会先打印两个输入模型的属性列表，再打印合并后的属性列表（结束后需按回车退出）。

## 7. 注意事项

1. **点数 / 单元数必须逐项对应**：这是本过滤器的核心前提（与 `vtkAppendAttributes` 一致）。任一输入与输入 #0 不一致时执行失败并报错，例如：
   `input #1 has 100 points / 20 cells while input #0 has 363 points / 200 cells; they must match one-to-one.`

2. **几何一律取自输入 #0**：输出的点坐标、单元连接与网格类型都来自第一个输入。当后续输入的数据类型与输入 #0 不同时只给出警告，仍按输入 #0 的类型输出，因此属性的挂接方式可能与来源模型不同。

3. **同名同类属性改名保留**：按「名称 + 归属（点 / 单元）」配对，同名数组不再丢弃——每个配对里**最后出现**的输入留用原名，更靠前的输入依次改名为 `<原名>_input_<模型序号>`（模型序号从 1 开始，所以第一个输入的属性后缀是 `_input_1`）。两个输入时就是「第二个输入保留原名、第一个输入加 `_input_1`」；多个输入以此类推，例如三个输入的 `shared` 依次变成 `shared_input_1`、`shared_input_2`、`shared`。改名只作用于输出的副本，输入模型不受影响；若输入本身就叫 `<原名>_input_<序号>`，为避免输出重名会继续追加 `_1`、`_2` 等后缀，并在日志里给出警告。

4. **两个开关不能同时关闭**：`SetAppendPointData(false)` 与 `SetAppendCellData(false)` 同时成立时执行失败并报错 `both point data and cell data are disabled!`；被关闭的关联上的属性不会出现在输出里。

5. **输入不能为空**：没有任何有效输入时执行失败并报错 `no valid input data object!`；所有输入既无点也无单元时同样失败。

6. **不修改原模型**：属性以深拷贝方式写入输出的新 `AttributeSet`，原始输入对象的属性保持不变。只有 `IG_POINT` / `IG_CELL` 两类关联的数组会被复制，其他归属的属性会被忽略。

7. **数据类型限制**：仅支持 `PointSet` / `SurfaceMesh` / `VolumeMesh` / `UnstructuredMesh` / `StructuredMesh`（含 `DrawObject` 包装与复合数据的展平）。结构网格输出会补齐单元连接关系；其它数据类型会报 `unsupported data object type` 并执行失败。
