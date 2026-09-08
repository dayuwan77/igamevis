# GenerateProcessIds 使用说明

适用范围：iGameCore 生成进程号 Filter（类名 GenerateProcessIdsFilter）；Qt 界面入口为菜单「算法处理 → 生成进程ID (GenerateProcessIds)」。

## 功能

为网格生成进程号数组，用于观察或模拟并行计算的分区：点进程号写入 PointProcessIds，单元进程号写入 CellProcessIds，类型均为 long long。结果直接写回输入对象的属性集，不创建新的数据对象。

进程号取值顺序：

1. 输入数据里带有名为 process_id 的数组（long long，且与点/单元挂载位置一致）时，逐元素沿用该数组的值；
2. 否则使用 SetProcessId() 设置的常数，默认 0；
3. 也可以继承本 Filter，重写受保护的 GetPointProcessId(IGsize) / GetCellProcessId(IGsize) 自行定义分区策略。

支持的输入：非结构化网格、表面网格、体网格、结构化网格、Lagrange 网格等 PointSet 系网格；纯点云（PointSet）不能生成单元数据。重复执行是安全的：同名数组会被复用并覆盖，不会累积重复属性。

## 调用方式

### C++ 接口

```cpp
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>
```

```cpp
auto filter = iGame::GenerateProcessIdsFilter::New();
filter->SetInput(mesh);
filter->SetGeneratePointData(true);   // 生成点进程号，默认开
filter->SetGenerateCellData(true);    // 生成单元进程号，默认关
filter->SetProcessId(7);              // 常数进程号；无 process_id 数组时生效
bool ok = filter->Execute();          // 失败时 GetMessage() 返回错误说明
// 结果直接写在输入 mesh 上：PointProcessIds / CellProcessIds
```

自定义分区时继承并重写两个虚函数：

```cpp
class MyPartitionFilter : public iGame::GenerateProcessIdsFilter {
public:
    I_OBJECT(MyPartitionFilter)
    static Pointer New() { return new MyPartitionFilter; }
protected:
    long long GetPointProcessId(IGsize index) override { return index % 2; }
    long long GetCellProcessId(IGsize index) override { return index % 2; }
};
```

### GUI 操作

1. 打开模型；
2. 菜单「算法处理 → 生成进程ID (GenerateProcessIds)」打开左侧面板；
3. 勾选「生成点数据」「生成单元数据」，点击应用。结果写回当前模型（模型树节点不变），按进程号着色即可查看分区。

注意：面板上没有进程号输入项，直接应用得到的是常数 0；需要其它常数或按外部数组分区时，请改用 C++ 接口或在数据中附加 process_id 数组。

## 使用示例

测试用例在 Examples/Filter/TestGenerateProcessIds.cpp（ctest 名 testGenerateProcessIds）。用例通过相对路径读取 AI 生成的测试模型，不需要任何参数即可完整运行：

```bash
# Windows：在 Examples 构建目录下
testGenerateProcessIds.exe
# 或
ctest -R testGenerateProcessIds --output-on-failure
```

全部通过时逐行打印 PASS 并以退出码 0 结束，任一失败打印 FAIL、退出码为 1。用例分别在两个 AI 测试模型（三节变径管、文丘里缩放喷管）上执行常数、分区、幂等、外部 process_id 四类校验，另覆盖表面网格、体网格与纯点云等程序化网格场景。

代码片段（读取 Examples/Models/GenerateProcessIds_SteppedPipe.vtk，生成常数 7 的点/单元进程号）：

```cpp
#include <iostream>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <ProcessGet/iGameGenerateProcessIdsFilter.h>

int main() {
    auto data = iGame::FileIO::ReadFile("./Models/GenerateProcessIds_SteppedPipe.vtk");
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(data);
    if (mesh == nullptr) return 1;

    auto filter = iGame::GenerateProcessIdsFilter::New();
    filter->SetInput(mesh);
    filter->SetGeneratePointData(true);
    filter->SetGenerateCellData(true);
    filter->SetProcessId(7);
    if (!filter->Execute()) { std::cout << filter->GetMessage() << "\n"; return 1; }

    auto pt = mesh->GetAttributeSet()->GetScalar("PointProcessIds");
    auto cell = mesh->GetAttributeSet()->GetScalar("CellProcessIds");
    std::cout << pt.pointer->GetNumberOfElements() << " points, value "
              << pt.pointer->GetValue(0) << "\n";
    std::cout << cell.pointer->GetNumberOfElements() << " cells, value "
              << cell.pointer->GetValue(0) << "\n";
    return 0;
}
```

## 注意事项

1. 本 Filter 是原地修改：结果数组直接写入输入对象，GetOutput() 返回的就是输入对象本身，与多数会生成新对象的 Filter 语义不同。
2. 结果数组名固定为 PointProcessIds / CellProcessIds，类型 long long；请用 GetScalar(PointProcessIds) 等方式读取。
3. 进程号取值按上文顺序：外部 process_id 数组优先于 SetProcessId() 常数；删除数据中的 process_id 数组会改变后续生成结果。
4. 纯点云（PointSet）生成单元数据会失败并返回错误信息，点数据不受影响。
5. 单元计数：表面网格按面数，体网格按体数，非结构化/结构化网格按单元数。
6. 示例按相对路径 ./Models/... 读取模型，需在 Examples 构建目录下运行；在其它目录运行请改用完整路径。

## 测试模型

两个模型放在 Examples/Models，由脚本程序化生成，几何均可复算：

| 文件 | 内容 |
| --- | --- |
| GenerateProcessIds_SteppedPipe.vtk | 三节变径管道（含两处锥形过渡）：432 点 / 264 六面体。带点标量 layer、单元标量 zone（0/1/2 三段） |
| GenerateProcessIds_VenturiTube.vtk | 文丘里缩放喷管（光滑喉道）：270 点 / 960 四面体。带点标量 layer、单元标量 zone |

两个模型自带的 layer/zone 标量把几何分成三段，直观对应多进程分区的场景；模型本身不要求自带 process_id 数组，直接应用即生成常数进程号。