# Transform 使用说明

## 1. 功能

`TransformFilter` 用于对输入模型进行通用几何变换，包括：

- **平移（Translation）**
- **旋转（Rotation）**
- **缩放（Scale）**

用户可以分别设置 X、Y、Z 三个方向的平移量、旋转角度和缩放比例。

TransformFilter 内部使用 4×4 齐次变换矩阵表示几何变换。旋转参数以**角度**为单位，并在计算变换矩阵时转换为弧度。缩放、旋转和平移矩阵按照内部设定的顺序组合成最终变换矩阵。

该 Filter 支持以下类型的输入数据：

- `SurfaceMesh`
- `VolumeMesh`
- `StructuredMesh`
- `UnstructuredMesh`

## 执行时会根据输入数据类型创建对应的输出对象，并对输出对象中的所有点进行变换。

## 2. 调用方式

### 2.1 C++ API 调用

首先创建 `TransformFilter`：

```cpp
auto filter = iGame::TransformFilter::New();
```

设置输入模型：

```cpp
filter->SetInput(input);
```

然后分别设置平移、旋转和缩放参数：

```cpp
filter->SetTranslation(tx, ty, tz);
filter->SetRotation(rx, ry, rz);
filter->SetScale(sx, sy, sz);
```

其中：

| 参数         | 含义                              |
| ------------ | --------------------------------- |
| `tx, ty, tz` | X、Y、Z 方向的平移量              |
| `rx, ry, rz` | 绕 X、Y、Z 轴的旋转角度，单位为度 |
| `sx, sy, sz` | X、Y、Z 方向的缩放比例            |

最后执行 Filter：

```cpp
if (!filter->Execute()) {
    // Transform 执行失败
}
```

执行成功后，通过：

```cpp
auto output = filter->GetOutput();
```

获取变换后的输出对象。

`Execute()` 首先检查输入对象，并根据输入数据类型创建对应的输出对象；随后构造变换矩阵，并逐点计算变换后的坐标。

------

## 3. 使用示例

### 3.1 完整测试示例

项目中的 `TestTransform.cpp` 使用：

```text
./Models/Transform_Complex.vtk
```

作为测试模型。程序读取模型后创建 `TransformFilter`，并设置一组平移、旋转和缩放参数。

示例代码如下：

```cpp
#include <Transformation/iGameTransformFilter.h>
#include <Core/iGameScene.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>

#include <iostream>

int main()
{
    auto scene = iGame::Scene::New();

    const std::string fileName = "./Models/Transform_Complex.vtk";
    auto input = iGame::FileIO::ReadFile(fileName);

    if (input == nullptr) {
        std::cout << "Read ERROR!\n";
        return 0;
    }

    auto filter = iGame::TransformFilter::New();
    filter->SetInput(input);

    filter->SetTranslation(10.0f, 0.0f, 0.0f);
    filter->SetRotation(0.0f, 0.0f, 45.0f);
    filter->SetScale(1.5f, 1.5f, 1.5f);

    if (!filter->Execute()) {
        std::cout << "Transform ERROR!\n";
        return 0;
    }

    auto output = filter->GetOutput();

    if (output == nullptr) {
        std::cout << "Output ERROR!\n";
        return 0;
    }

    scene->AddModel(output);

    auto window = iGame::RenderWindow::New();
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

该示例设置：

```text
平移：X = 10.0，Y = 0.0，Z = 0.0
旋转：X = 0.0°，Y = 0.0°，Z = 45.0°
缩放：X = 1.5，Y = 1.5，Z = 1.5
```

因此模型会先按照设置进行缩放、旋转和平移，最终将变换后的模型添加到场景中显示。示例程序会自动读取 `./Models/Transform_Complex.vtk`，无需运行时手动输入文件路径。

------

## 4. Qt 界面调用

在 iGameVis Qt 界面中，可以通过：

```text
Filters → 通用几何变换 (Transform)
```

打开 Transform 参数窗口。

参数窗口提供以下参数：

| 参数           | 默认值 | 说明        |
| -------------- | ------ | ----------- |
| 平移 X         | 0.0    | X 方向平移  |
| 平移 Y         | 0.0    | Y 方向平移  |
| 平移 Z         | 0.0    | Z 方向平移  |
| 旋转 X（角度） | 0.0    | 绕 X 轴旋转 |
| 旋转 Y（角度） | 0.0    | 绕 Y 轴旋转 |
| 旋转 Z（角度） | 0.0    | 绕 Z 轴旋转 |
| 缩放 X         | 1.0    | X 方向缩放  |
| 缩放 Y         | 1.0    | Y 方向缩放  |
| 缩放 Z         | 1.0    | Z 方向缩放  |

Qt 界面读取用户输入的参数后，创建 `TransformFilter`，设置输入数据以及三个变换参数，然后调用 `Execute()`。执行成功后将输出对象添加到模型树并刷新渲染窗口。

------

## 5. 变换矩阵

TransformFilter 使用 4×4 齐次矩阵表示变换。

缩放矩阵为：

```text
S =
| sx  0   0   0 |
| 0   sy  0   0 |
| 0   0   sz  0 |
| 0   0   0   1 |
```

平移矩阵为：

```text
T =
| 1   0   0   tx |
| 0   1   0   ty |
| 0   0   1   tz |
| 0   0   0   1  |
```

旋转分别使用 X、Y、Z 轴旋转矩阵。

最终矩阵按照代码中的顺序组合：

```text
M = T · Rz · Ry · Rx · S
```

因此变换结果由缩放、旋转和平移共同决定。

------

## 6. 注意事项

### 6.1 旋转单位为角度

`SetRotation()` 接收的参数单位为**度（°）**，例如：

```cpp
filter->SetRotation(0.0f, 0.0f, 45.0f);
```

表示绕 Z 轴旋转 45°。Filter 内部会自动将角度转换为弧度后进行三角函数计算。

### 6.2 缩放参数

缩放参数为各方向的缩放比例。

例如：

```cpp
filter->SetScale(2.0f, 1.0f, 1.0f);
```

表示 X 方向扩大 2 倍，而 Y、Z 方向保持不变。

默认缩放参数为：

```text
1.0, 1.0, 1.0
```

### 6.3 输入数据不能为空

执行 Filter 前必须设置有效的输入数据。如果输入为空，`Execute()` 会执行失败并返回 `false`。

### 6.4 输出对象与输入对象分离

TransformFilter 针对支持的数据类型创建新的输出对象，并对点数据进行深拷贝或重新创建，因此变换操作是在输出对象上完成的，而不是直接修改原输入对象。以 `SurfaceMesh` 为例，Filter 通过 `DeepCopy()` 创建输出对象；其他网格类型也会创建新的点数据。

### 6.5 不支持的数据类型

如果输入数据类型不属于当前支持的四种类型：

```text
SurfaceMesh
VolumeMesh
StructuredMesh
UnstructuredMesh
```

则 `Execute()` 会返回 `false`。

### 6.6 参数输入错误

Qt 界面中的平移、旋转和缩放参数需要输入有效的数字。如果输入内容无法转换为数字，界面会提示对应的参数错误，并不会执行 Transform。

------

## 7. 总结

`TransformFilter` 提供统一的三维几何变换功能，可以通过 C++ API 或 iGameVis Qt 界面使用。

基本使用流程为：

```text
读取模型
   ↓
创建 TransformFilter
   ↓
设置输入模型
   ↓
设置平移参数
   ↓
设置旋转参数
   ↓
设置缩放参数
   ↓
Execute()
   ↓
获取输出模型
   ↓
显示或继续处理
```

测试程序采用固定的相对路径：

```text
./Models/Transform_Complex.vtk
```

并自动执行 Transform 和显示结果，可用于验证 Filter 的基本功能。