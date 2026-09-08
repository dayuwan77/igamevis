# ExtractComponent 使用说明

适用范围：iGameCore 提取分量 Filter（类名 ExtractComponentFilter）；Qt 界面入口为菜单「算法处理 → 提取分量 (Extract Component)」。

## 功能

从输入网格的点数据或单元数据中取出一个多维数组的某一分量，生成一维标量数组，并返回一个新的数据对象。输出对象与输入共享几何，输入对象本身不会被修改。

数组选择规则：

- 不指定输入数组时，取属性集中第一个向量属性；
- 可按数组名指定；点、单元存在同名数组时，用挂载类型（点 / 单元）进一步区分；
- 分量按 0 起始的索引取值，0/1/2 即 X/Y/Z。索引超出数组维度时执行失败，错误原因通过 GetMessage() 读取。

输出数组默认名 Result，可自定义；若结果中已有同名数组，新结果会覆盖旧数组而不是报错。输出数组的具体类型与输入数组一致（float、int、long long 等各自保持），避免类型转换。目前支持非结构化网格和表面网格。

## 调用方式

### C++ 接口

```cpp
#include <Attribute/iGameExtractComponentFilter.h>
```

```cpp
auto filter = iGame::ExtractComponentFilter::New();
filter->SetInput(mesh);                    // 输入 DataObject
filter->SetInputArrayName("V");            // 数组名；留空 = 取第一个向量
filter->SetInputAttachmentType(IG_POINT);  // 可选：IG_POINT / IG_CELL，默认不限定
filter->SetOutputArrayName("ResultY");     // 输出数组名，默认 Result
filter->SetComponent(1);                   // 分量索引：0/1/2 = X/Y/Z
bool ok = filter->Execute();               // 失败时 GetMessage() 返回错误说明
auto out = filter->GetOutput();            // 结果数据对象
```

### GUI 操作

1. 打开模型；
2. 菜单「算法处理 → 提取分量 (Extract Component)」打开左侧面板；
3. 选择输入数组（下拉框中点/单元数组以 (Point)/(Cell) 标注）、输出数组名和分量；
4. 点击 Apply。首次执行在模型树新增结果节点（命名 原模型名_ExtractComponent_序号）；对同一输入再次执行则更新该节点，不重复新建。结果节点可继续作为其它 Filter 的输入。

## 使用示例

测试用例在 Examples/Filter/TestExtractComponent.cpp（ctest 名 testExtractComponent）。用例通过相对路径读取 AI 生成的测试模型，不需要任何参数即可完整运行：

```bash
# Windows：在 Examples 构建目录下
testExtractComponent.exe
# 或
ctest -R testExtractComponent --output-on-failure
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例覆盖两个 AI 测试模型（直管道螺旋流、90° 弯管）的真实数据校验，以及重名覆盖、维度越界、单元挂载、输出类型保持等断言。

代码片段（读取 Examples/Models/ExtractComponent_FlowPipe.vtk，提取点向量 V 的 Y 分量）：

```cpp
#include <iostream>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <Attribute/iGameExtractComponentFilter.h>

int main() {
    auto data = iGame::FileIO::ReadFile("./Models/ExtractComponent_FlowPipe.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(data);
    if (mesh == nullptr) return 1;

    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetInputArrayName("V");
    filter->SetOutputArrayName("ResultY");
    filter->SetComponent(1);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    auto arr = out->GetAttributeSet()->GetScalar("ResultY").pointer;
    for (IGsize i = 0; i < arr->GetNumberOfElements(); ++i)
        std::cout << arr->GetValue(i) << "\n";
    return 0;
}
```

## 注意事项

1. 分量索引从 0 起；维度为 1 的数组只能取 X。GUI 面板对 1 维数组会禁用分量选择，确有需要时在代码里用 SetComponent(0)。
2. 不填输入数组名 = 取第一个向量属性，顺序由属性集决定（一般点数据在前）；同名点/单元数组请显式指定挂载类型。
3. 输出是新对象，输入对象不被修改；几何（点、单元）为共享引用，不要假定输出持有独立几何。
4. 输出数组名与输入已有数组重名时执行覆盖，结果集中不会残留两个同名属性。
5. 输入仅支持非结构化网格与表面网格。
6. 示例按相对路径 ./Models/... 读取模型，需在 Examples 构建目录下运行；在其它目录运行请改用完整路径。

## 测试模型

两个模型放在 Examples/Models，由脚本程序化生成，几何与流场分布均可复算：

| 文件 | 内容 |
| --- | --- |
| ExtractComponent_FlowPipe.vtk | 直管道（环形流道）：576 点 / 384 六面体。点向量 V（切向涡 + 轴向流）、点标量 Pressure（沿程递减）；单元向量 cellV、单元标量 cellStress |
| ExtractComponent_BendPipe.vtk | 90° 弯管段：210 点 / 720 四面体。点向量 V（随弯转方向旋转）、点标量 Pressure；单元向量 cellV、单元标量 cellStress |

两个模型均以点向量 V（float、3 分量）作为第一个向量属性，并同时带点、单元两类数组，便于验证点/单元两条提取路径与弯道中分量方向的真实变化。