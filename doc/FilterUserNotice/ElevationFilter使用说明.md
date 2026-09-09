# ElevationFilter 使用说明

## 1. 功能简介

ElevationFilter（高程标量场过滤器）将网格中每个点沿指定方向向量的投影长度，线性映射到输出区间，生成一个点标量属性（默认名 `"Elevation"`），可直接用于标量场着色。

核心数学（仿射映射）：

```
h   = p · d                          每点沿方向 d 的带符号投影长度
out = Low + (h - hMin) / (hMax - hMin) * (High - Low)
```

其中 `[hMin, hMax]` 为全部点的投影范围，`[Low, High]` 为用户设置的输出区间（默认 `[0, 1]`）。

典型应用场景：

- 地形可视化：按海拔着色，一眼读出地形起伏（等高线地图的数字版）；
- CFD 后处理：大气 / 海洋仿真按高度分层查看物理量；
- 网格健康检查：按坐标着色可直观发现坐标异常的坏点（如坐标为 1e38 的点会形成极端 outlier）；
- 渲染前处理：依高度混色 shader 的第一步——将高度归一化到 [0,1]。

**输入 / 输出**

| 项 | 说明 |
|---|---|
| 输入 | 1 个 PointSet 系网格（SurfaceMesh / UnstructuredMesh / VolumeMesh 等均可） |
| 输出 | 输入对象本身（in-place），新增 1 个 `IG_SCALAR / IG_POINT` 属性，数组类型 FloatArray，维度 1 |
| 拓扑 | 不修改点 / 单元结构，仅附加属性数组 |

## 2. 调用方式

头文件：`iGameCore/Filters/Elevation/iGameElevationFilter.h`（已在 `iGameFilterIncludes.h` 中统一注册，直接 include 该总头亦可）。

| API | 说明 |
|---|---|
| `ElevationFilter::New()` | 工厂方法创建过滤器，返回智能指针 |
| `SetInput(DataObject)` | 设置输入网格 |
| `SetDirection(float dx, float dy, float dz)` | 设置投影方向向量；**零向量返回 false 并保持原方向** |
| `SetDirection(const Vector3f&)` | 同上的重载版本 |
| `GetDirection()` | 返回当前方向向量（默认 `(0, 0, 1)` 即 +Z） |
| `SetOutputRange(double low, double high)` | 设置输出区间；**要求 low < high**，非法输入被忽略并保持原值 |
| `GetLowValue() / GetHighValue()` | 读取当前输出区间（默认 0 / 1） |
| `SetArrayName(const std::string&)` | 设置生成的属性数组名（默认 `"Elevation"`） |
| `Execute()` | 执行算法，成功返回 true |

执行失败（返回 false）的情形：输入为空、输入不是 PointSet 系网格、方向为零向量、网格没有点。

## 3. 使用示例

### 3.1 C++ 调用

```cpp
#include <Elevation/iGameElevationFilter.h>
#include "iGameFileIO.h"
#include "iGamePointSet.h"

using namespace iGame;

// 1. 读入模型（任意 PointSet 系网格均可）
auto mesh = DynamicCast<PointSet>(FileIO::ReadFile("Models/ElevationSlopeTerrain.vtk"));

// 2. 创建并配置过滤器
auto filter = ElevationFilter::New();
filter->SetDirection(1.f, 1.f, 0.f);   // 沿 (1,1,0) 方向投影（斜向 45° 量高度）
filter->SetOutputRange(0.0, 100.0);    // 输出映射到 [0, 100]

// 3. 执行
filter->SetInput(mesh);
if (!filter->Execute()) {
    // 失败处理（输入非法 / 零向量等）
}

// 4. 结果：mesh 上新增点标量属性 "Elevation"，输出即输入
auto result = DynamicCast<PointSet>(filter->GetOutput());
```

### 3.2 Qt 界面操作

1. 打开模型文件；
2. 菜单栏 **【算法处理】→ 高程场 (Elevation)**（一级菜单项）；
3. 在弹出的参数表单中填写：
   - **方向向量**：三个分量（dx, dy, dz），默认 `(0, 0, 1)` 即沿 Z 轴（常规海拔语义）；
   - **输出范围**：low / high，默认 `[0, 1]`；
4. 点击 OK 执行，右侧模型树出现 `Elevation` 属性，在左侧标量场面板选中即可着色。

## 4. 注意事项

1. **方向向量无需归一化**：所有投影值会先求全局范围再归一化，方向的公共缩放被约去——`(1, 1, 0)` 与 `(2, 2, 0)` 的输出完全一致。
2. **零向量被拒绝**：`(0, 0, 0)` 没有方向语义，`SetDirection` 返回 false 且保持原方向；UI 表单同样拦截。
3. **平面退化不报错**：若网格垂直于投影方向（所有投影值相同，如 XY 平面网格配 +Z 方向），过滤器**降级**为全部输出 Low 值并打警告日志，而不是失败或产生 NaN——"所有点海拔相同"是合法语义。
4. **负向量 = 高低翻转**：`(0, 0, -1)` 与 `(0, 0, 1)` 结果图案相同但颜色高低互换（合法用法，用于反转"谁是最高点"的语义）。
5. **重复执行会追加同名属性**：每次 Execute 都通过 AddAttribute 追加数组。对同一网格反复执行会在属性集中出现多个 `"Elevation"`（UI 中按序号区分）。如需覆盖，可先用 PassArrays 过滤或修改数组名。
6. **输出区间必须 low < high**：`SetOutputRange(1, 0)` 或 `(5, 5)` 会被静默忽略，保持原区间。
7. **投影值类型**：点积按 float 计算、映射按 double 计算，最终存入 FloatArray；对常规工程坐标精度足够。

## 5. 配套测试

- 测试程序：`Examples/Filter/Elevation/TestElevation.cpp`（目标 `testElevation`，已注册进 Examples/CMakeLists.txt 与 CTest）
- 测试模型（本过滤器专用，随源码提交）：
  - `Examples/Models/ElevationSlopeTerrain.vtk` —— 11×11 斜坡地形（z = 0.1·(x+y)），用于任意方向 `(1,1,0)` 投影与端点钉扎断言；
  - `Examples/Models/ElevationTerraces.vtk` —— 11×11 六层梯田（z = floor((x+y)/4)），用于 Z 轴 + 自定义范围 `[10,20]` 的离散层级断言（映射后恰好产生 {10, 12, 14, 16, 18, 20} 六个值）。
- 共 8 组用例：6 组代码内构建网格（映射 / 范围 / 任意方向 / 缩放不变性 / 平面退化 / 非法输入）+ 2 组模型文件用例。模型路径写死为相对路径 `Models/...`（构建时由 CMake 自动复制到构建目录），运行即自动完成全部测试，无需手动输入。
