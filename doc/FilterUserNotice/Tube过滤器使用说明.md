# Tube（圆管）过滤器使用说明

## 一、功能简介

`TubeFilter` 将一维线段（`IG_LINE` / `IG_POLY_LINE`，例如流线、折线、骨架线）扫掠成具有真实厚度的三维圆管，输出为 `SurfaceMesh`。

**算法原理与 ParaView / VTK 的 `vtkTubeFilter` 一致；输出单元类型与 ParaView 的差异见第四节：**

在每个路径点上，先确定 “垂直于前进方向（切线 T）的平面”，再在该平面内画一个由半径 `Radius` 和边数 `NumberOfSides` 决定的正多边形截面；相邻截面缝合时每个侧面拆成 2 个三角形，可选 `Capping` 在首、尾用三角形扇封闭端面。截面朝向通过 “平行传递” 沿路径延续，保证不翻转、不扭转。

一句话概括：**在每个路径点上，先确定垂直于前进方向的平面，再在该平面内画一个由半径和边数决定的正多边形，最后把这些多边形沿路径对齐、缝合起来，就成了圆管。**

**参数：**

| 参数名               | 含义                                             | 默认值    |
| -------------------- | ------------------------------------------------ | --------- |
| `Radius`           | 圆管半径（固定）                                 | 0.1       |
| `NumberOfSides`    | 截面正多边形边数，≥3，越大越接近圆              | 6         |
| `Capping`          | 是否封闭首、尾端面                               | true      |
| `UseDefaultNormal` | 是否使用用户给定的`DefaultNormal` 作为初始法向 | false     |
| `DefaultNormal`    | 初始法向参考                                     | (0, 0, 1) |

---

## 二、调用方式

### 1. C++ 接口

```
\#include \<Tube/iGameTube.h>

auto filter = iGame::TubeFilter::New();

filter->SetRadius(0.05);

filter->SetNumberOfSides(12);

filter->SetCapping(true);

filter->SetUseDefaultNormal(false);

filter->SetInput(lineData);          // 含线段的 Poly Data

if (!filter->Execute()) {

    // 执行失败（通常是输入不含线段 / 类型不支持）

}

auto outMesh = iGame::DynamicCast\<iGame::SurfaceMesh>(filter->GetOutput(0));
```

主要接口：

| 接口                                                           | 说明                                       |
| -------------------------------------------------------------- | ------------------------------------------ |
| `SetRadius(double)` / `GetRadius()`                        | 设置 / 获取圆管半径。                      |
| `SetNumberOfSides(int)` / `GetNumberOfSides()`             | 设置 / 获取截面边数。                      |
| `SetCapping(bool)` / `IsCapping()`                         | 设置 / 获取是否封端。                      |
| `SetUseDefaultNormal(bool)` / `IsUsingDefaultNormal()`     | 设置 / 获取是否使用用户给定初始法向。      |
| `SetDefaultNormal(const Vector3d&)` / `GetDefaultNormal()` | 设置 / 获取初始法向参考。                  |
| `SetInput(dataObject)`                                       | 设置输入，须为含线段的 Poly Data。         |
| `Execute()`                                                  | 执行，成功返回`true`。                   |
| `GetOutput(0)`                                               | 获取输出，需`DynamicCast<SurfaceMesh>`。 |

### 2. Qt 图形界面

菜单：**过滤器（Filters） → 圆管 (Tube)**

1. 在模型树中选中含线段的模型；
2. 在面板设置半径、边数、是否封端（Capping）、是否使用默认法向等；
3. 点击 “应用（Apply）”，生成圆管。

---

## 三、使用示例

* 测试程序：`Examples/Filter/Tube/TestTubeFilter.cpp`
* 测试模型：`Examples/Models/tube_test.vtk`（一条螺旋线 + 一条波形线，带点标量 `t`、`radius`）

命令行运行（工作目录为 Examples 构建目录）：

```
\# 使用默认相对路径模型

TestTubeFilter.exe

\# 或显式指定模型

TestTubeFilter.exe Models/tube\_test.vtk
```

程序会打印生成 tube 的点 / 面数量，并弹出一个新的渲染窗口（黑框）显示圆管；关闭窗口后程序退出。

在 ParaView 中对照：打开同一模型 → Filters → Tube，设置相同的 Radius、Number of Sides、Capping，可比对渲染几何与顶点坐标；注意单元计数口径与 ParaView 不同（详见第四节）。

---

## 四、与 ParaView 的差异（单元计数口径，重要）

本过滤器与 ParaView 的 `vtkTubeFilter` **几何形状完全相同**（同样的顶点坐标、同样的圆管外观），但**输出的单元类型和计数方式不同**：

|                                       | 侧壁                                                                                  | 封端 Capping                                                       |
| ------------------------------------- | ------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| 本实现                                | 每段 × 每边把一个侧面**拆成 2 个三角形**（全部输出三角形）                     | 每端**新增 1 个中心点**，用 `NumberOfSides` 个三角形做三角形扇 |
| ParaView`vtkTubeFilter`（多数版本） | 把沿整条折线、同一侧面方向的三角形打包成 **triangle strip**，每条线每个侧面仅 1 个 strip | 每端 1 个 strip，并**复制**环上顶点（坐标相同、法向独立）      |

以本测试（2 条 120 点折线、半径为0.1、 边数为6、Capping 开）为例：

|          | 点数                   | 单元数                                 |
| -------- | ---------------------- | -------------------------------------- |
| 本实现   | 1444                   | 2880        |
| ParaView | 1464（cap 多复制 24 点，首尾中间共4个） | 16（12 个侧壁 strip + 4 个 capstrip） |

计数口径明细（边数 6、2 条 120 点折线、Capping 开）：

* 本实现：环点 2×120×6 = 1440，封端中心点 2 线 ×2 端 = 4，共 **1444 点**；侧壁三角形 2×119×6×2 = 2856，封端三角形 2×2×6 = 24，共 **2880 个三角形**。
* ParaView：环点 1440，封端复制点 2×2×6 = 24，共 **1464 点**；侧壁 strip 2×6 = 12，封端 strip 2×2 = 4，共 **16 个 strip**。
* 两者**渲染结果完全等价**——显卡会把每个 strip 当场展开成同样数量的三角形；区别只在存储 / 计数：strip 让成串三角形共享顶点、压成一个单元，故 ParaView 单元数很少。跨软件比对请比较**几何形状与顶点坐标**，不要直接比较单元数量。

---

## 五、注意事项

1. **输入须为含线段（**`IG_LINE`**&#x20;/&#x20;**`IG_POLY_LINE`**）的 Poly Data**，输出为 `SurfaceMesh`；输入不含线段时 `Execute()` 返回失败。
2. **半径要与模型尺度匹配**：半径过大，相邻 / 弯折处管段会互相重叠、穿模；过小则看不清厚度。可参照模型整体尺寸取约 0.5%\~1%。
3. **边数决定圆滑度与面数**：每个侧面拆成 2 个三角形，每段产生 2×`NumberOfSides` 个三角形；边数越大越接近圆，但面片数量和开销线性增加，一般 6\~20 即可。
4. **Capping（封端）**：在每条折线首、尾各新增 1 个中心点，用三角形扇补一个端面；开放折线建议开启，封闭 / 环形线无需封端。
5. **Default Normal / Use Default Normal**：用于控制第一个截面的初始朝向。

* 不勾选（默认）时，程序自动选择初始参考方向；
* 当管子**一开始就是竖直**走向、与默认参考方向（0,0,1）重合时，程序会自动换一个坐标轴作参考，无需手动处理。

6. **截面朝向沿路径 “平行传递”**：每个截面只顺着拐弯做最小转动以保持垂直于新切线，因此圆管不会突然翻转或扭转。
7. 点属性会随圆管环向复制（与 `vtkTubeFilter` 行为一致）。本实现全部输出三角形（侧壁拆 2 个三角形、封端用中心点三角形扇），ParaView 多数版本输出 triangle strip，二者单元 / 点计数口径不同但几何等价，详见第四节。
