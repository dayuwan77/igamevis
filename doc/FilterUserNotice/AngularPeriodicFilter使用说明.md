# AngularPeriodic 使用说明

角度周期复制（Angular Periodic Copy）过滤器：把一个网格绕指定轴，以给定总角度按份数均分，旋转复制 N 份并合并成一个 UnstructuredMesh 输出。第 0 份为原始网格（不旋转）。

## 一、功能

- 输入：单个网格（SurfaceMesh / UnstructuredMesh / VolumeMesh / StructuredMesh / 点云 PointSet 均可）。
- 输出：合并后的 UnstructuredMesh，包含原始网格 + 各旋转副本。
- PointData / CellData 属性按份逐份复制到输出（标量/向量均保留，值保持不变；向量/张量不随几何旋转）。
- 支持任意大小单元：>16 点多边形、多面体（按核心面编码处理）都不会被丢弃或溢出。

## 二、界面入口

主界面 →【算法处理】菜单 → **角度周期复制 (Angular Periodic)**，弹出参数对话框：

| 参数 | 含义 | 默认 |
| --- | --- | --- |
| origin_x/y/z | 旋转轴经过的点 | 0 / 0 / 0 |
| axis_x/y/z | 旋转轴方向（自动归一化） | 0 / 0 / 1 |
| copies | 总份数（含原始网格） | 2 |
| angle(deg) | 旋转总角度（度），按份数均分：第 i 份旋转 angle×i/copies | 360 |

点击【应用】执行；失败会弹窗显示原因（如轴为零向量、输入为空）。

## 三、调用方式（代码）

```cpp
#include "Periodic/iGameAngularPeriodicFilter.h"

auto filter = iGame::AngularPeriodicFilter::New();
filter->SetInput(dataObj);                                   // 输入网格
filter->SetRotationAxis(Point(ox, oy, oz), Vector3d(ax, ay, az)); // 轴过 (ox,oy,oz)、方向 (ax,ay,az)
filter->SetNumberOfCopies(copies);
filter->SetAngle(angleDeg);                                  // 总角度
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << "\n";               // 失败原因
    return 1;
}
auto output = filter->GetOutput();                           // 合并后的 UnstructuredMesh
```

## 四、使用示例（自动测试，无需手动输入）

仓库自带两个程序化/AI 生成模型（`Examples/Models/`）：

- `AngularDemoRingSector.obj`：120° 环形扇区，绕 Z 轴 3 份 360° 可拼满整圈（示例默认用它）。
- `AngularDemoFinBlade.obj`：离轴单叶闭式桨叶，旋转复制后呈扇叶效果。

运行（构建后工作目录为 `Examples`，路径写死、自动读取）：

```powershell
# Windows（Visual Studio 生成器）
cd cmake-build-sync\Examples
.\Release\testAngularPeriodic.exe

# CTest
ctest -R testAngularPeriodic
```

无 GUI 自动回归（程序内构造网格，断言全部 PASS 返回 0）：

```powershell
.\Release\testAngularPeriodicSelfCheck.exe   # 49 项检查 PASS
ctest -R testAngularPeriodicSelfCheck
```

## 五、注意事项

- `angle` 是**总角度**（按份数均分），不是"每份角度"；`copies` 含原始网格，份数过小或角度为 0 时输出与输入重合。
- 旋转轴为零向量、输入为空、份数 < 1 会执行失败，通过 `GetMessage()` 获取原因。
- 属性值按份**原样复制**：若需要向量/张量随几何同步旋转，请自行对属性数组做旋转（当前过滤器不做）。
- 多面体/StructuredMesh 输入在过滤器内部会先物化为规范单元，顶点号偏移由过滤器处理，使用者无需关心。
- 运行时工作目录必须含 `Models`（构建阶段 `iGameCopyExampleAssets` 会把 `Examples/Models` 拷到构建目录）；直接双击 `Release` 下的 exe 而工作目录不对会读不到模型。

## 六、相关文件

| 文件 | 说明 |
| --- | --- |
| `iGameCore/Filters/Periodic/iGameAngularPeriodicFilter.h/.cpp` | 过滤器实现 |
| `Examples/Filter/Periodic/TestAngularPeriodic.cpp` | GUI 示例（默认读 `AngularDemoRingSector.obj`，自动运行） |
| `Examples/Filter/Periodic/TestAngularPeriodicSelfCheck.cpp` | 无 GUI 自动回归（49 项断言） |
| `Examples/Models/AngularDemoRingSector.obj` | 120° 环形扇区测试模型（程序化/AI 生成） |
| `Examples/Models/AngularDemoFinBlade.obj` | 桨叶测试模型（程序化/AI 生成） |
