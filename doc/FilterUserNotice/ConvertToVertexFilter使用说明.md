# ConvertToVertexFilter 使用说明

> 过滤器名称：`ConvertToVertexFilter`（转换为顶点单元 / Convert To Vertex）
> 头文件：`iGameCore/Filters/Convert/iGameConvertToVertexFilter.h`
> 命名空间：`iGame`

## 1. 功能

将输入数据中的**每个点**转换为一个**顶点单元（`IG_VERTEX`）**，输出为只包含顶点单元的非结构网格（`UnstructuredMesh`）。原网格的体单元 / 面单元均不保留，仅保留点及点属性。

- 输入：`PointSet`（点集；`UnstructuredMesh`、`SurfaceMesh`、`VolumeMesh` 等点基网格均可作为输入）
- 输出：`UnstructuredMesh`，单元类型全部为 `IG_VERTEX`，每个单元引用一个输入点下标

数据处理规则：

| 数据 | 处理方式 |
| ---- | -------- |
| 点（坐标） | 原样拷贝到输出 `Points`；顶点单元只存点下标、不存坐标，对应关系由下标保证 |
| 点属性（`IG_POINT`） | 一一对应拷贝为输出点属性，类型、维度、数值全部保留（支持全部数值数组类型） |
| 单元属性（`IG_CELL`） | 数量与原网格单元无法对应，转换时丢弃 |
| 网格名称 | 继承输入网格名称 |

## 2. 调用方式

### 2.1 代码方式

```cpp
#include <Convert/iGameConvertToVertexFilter.h>

using namespace iGame;

// 1. 创建过滤器
auto filter = ConvertToVertexFilter::New();

// 2. 设置输入（点基网格均可）
filter->SetInput(obj);

// 3. 执行转换
if (!filter->Execute()) {
    // 失败处理
    return;
}

// 4. 取输出：顶点单元非结构网格
auto out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
out->AddViewStyle(IG_POINTS);   // 以点模式显示
```

### 2.2 界面方式

Qt 主窗口「算法处理」菜单 →「转换为顶点单元 (Convert To Vertex)」：对当前选中的模型执行转换，成功后以「算法」节点加入模型树并刷新渲染窗口。无选中模型 / 无数据时不执行。

## 3. 使用示例

测试程序 `testConvertToVertex`：

- 注册：`Examples/CMakeLists.txt` 中 `igame_add_example(testConvertToVertex Filter/Convert/TestConvertToVertex.cpp)`
- 源文件：`Examples/Filter/Convert/TestConvertToVertex.cpp`
- 运行：**直接运行，无需命令行输入**，模型相对路径已写死在代码中

```text
testConvertToVertex.exe
```

示例内容：

1. 读取写死的模型 `./Models/AIGen_Tet_TwistedRod.vtk`（四面体体网格：80 点 / 216 四面体，带 `Temperature`、`Pressure`、`Displacement` 点属性）
2. 执行 `ConvertToVertexFilter`
3. 打印输入点集点数与输出顶点单元网格的点数、单元数
4. 以点模式（`IG_POINTS`）在渲染窗口中显示转换后的顶点单元网格

预期输出（节选）：

```text
input  points: 80
output points: 80, cells: 80
```

输出点数与输入点数相同、单元数与输入点数相同（每个点一个 `IG_VERTEX` 顶点单元）。

## 4. 注意事项

1. **单元属性会被丢弃**：转换后只保留点与点属性，原网格的体 / 面单元及单元属性不保留。
2. **顶点单元不存坐标**：坐标统一存放在输出 `Points` 数组中，单元通过下标引用，不要试图从顶点单元读取坐标。
4. **失败提示**：模型读取失败打印 `Read ERROR!`，执行失败打印 `Execute ERROR!`，输出类型不符打印 `Output ERROR!`。
6. **输入类型**：输入必须是点基网格（`PointSet` 派生类型）；空输入 / 非点集输入会导致执行失败。
