# ForceStaticMesh 强制静态网格过滤器使用说明

## 功能

`ForceStaticMeshFilter` 对输入网格做**静态缓存管理**：首次执行时把输入网格深拷贝一份作为静态缓存（几何固定），之后只要几何规模（点数 / 单元数）不变，就只更新属性数据，输出始终是同一份几何固定的网格。

**语义（界面中也会明确提示）**：

> 固定第一个时间步的网格几何，后续只更新属性；仅适用于各时间步网格完全相同的数据。

也就是说，模型会停在「建立缓存的那一帧」的几何上，云图 / 属性随当前时间步变化。适合：

- 几何不变、属性随时间变化的数据（如 Abaqus ODB 逐帧结果、各时间步网格完全相同的多文件序列）；
- 变形网格对比：把几何钉在初始帧上，只看属性云图的变化；
- 逐帧比较不同时间步的属性，避免视角随网格变形而漂移。

核心行为：

- **首次执行**：深拷贝输入网格（或其各子块），构建静态网格缓存，并记录「缓存几何来自哪一帧」。
- **时间步变化**：拖动时间轴 / 播放时自动执行，把当前帧的属性同步到缓存，几何保持缓存建立时的那一帧。
- **点数 / 单元数变化**：自动重建几何缓存，并在提示中给出「旧规模 → 新规模」。
- **输入对象改变**（手动在另一个模型上执行）：即使新模型点 / 单元数与旧模型相同，也会强制重建缓存，避免误复用旧模型几何。
- **`ForceCacheComputation`**：置为 `true` 时总是强制重建缓存（界面上的「重新建立几何缓存」）。
- **可关闭**：关闭静态缓存后输出直接跟随输入网格（几何随时间步变化），用于对比变形网格的真实形态。
- **不修改原模型**：输出是独立的新网格，原模型保持不变。

## 支持的数据形态

| 输入 | 说明 |
|------|------|
| 单块网格 | SurfaceMesh、UnstructuredMesh、VolumeMesh、StructuredMesh（及任何 PointSet / 派生网格） |
| 时序容器 | `.pvd`、`.igc` 等多文件时序：输入是容器对象，真正的网格挂在子对象上；缓存按块一一对应，容器属性按同名补齐，供模型树 / 云图寻址 |

> 时序数据的两种时间帧类型都已接入：
> - `MultiSubFiles`（PVD / IGC 等）：每帧读取一个 / 多个网格文件；
> - `SingleFieldAttributes`（如 ODB）：几何来自输入本身，每帧只替换属性集。

## 调用方式

### C++ 接口

```cpp
#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>

auto filter = iGame::ForceStaticMeshFilter::New();
filter->SetInput(mesh);            // mesh 为 DataObject::Pointer
if (filter->Execute()) {           // 交互式执行
    auto out = filter->GetOutput(); // 静态缓存网格
}

// 时序驱动：界面在时间步变化后调用，几何沿用缓存建立时的那一帧
if (filter->ExecuteAtTimeStep(frameIndex, timeValue)) {
    auto out = filter->GetOutput();
}
```

可选配置与状态查询：

```cpp
filter->SetForceCacheComputation(true);   // 强制重建几何缓存（默认 false）
filter->SetStaticCacheEnabled(false);     // 关闭静态缓存：输出跟随输入几何（默认 true）
filter->SetStaticCacheEnabled(true);

bool  has     = filter->HasCache();
int   frame   = filter->GetCacheTimeStepIndex();   // 缓存几何来自第几帧（-1：手动建立）
float t       = filter->GetCacheTimeValue();
IGsize pts    = filter->GetCacheNumberOfPoints();
IGsize cells  = filter->GetCacheNumberOfCells();
bool  rebuilt = filter->WasCacheRebuilt();         // 上次执行是否重建了几何
auto  reason  = filter->GetLastRebuildReason();    // RR_NONE / RR_FIRST_BUILD /
                                                   // RR_INPUT_CHANGED / RR_FORCED / RR_SIZE_CHANGED
std::string desc = filter->GetCacheDescription();
// 例如："缓存几何来自 t=0（第 1 帧）：27 点 / 8 单元"
std::string msg  = filter->GetStatusMessage();     // 上次执行的说明文字
```

### GUI 使用

菜单：**算法处理 → 数据处理 (Data Processing) → 强制静态网格 (Force Static Mesh)**

菜单内容：

| 菜单项 | 作用 |
|--------|------|
| 状态：… | 只读状态行，显示「缓存几何来自 t=0（第 1 帧）：27 点 / 8 单元」，未建立时显示「尚未建立静态缓存」 |
| 说明：固定第一个时间步的网格几何… | 只读说明行，明确该过滤器的适用前提 |
| 应用 / 更新静态网格缓存 | 建立或复用缓存：几何规模不变时只更新属性 |
| 重新建立几何缓存 | 强制重建几何缓存（一次性 `ForceCacheComputation`），几何改以当前时间步为准 |
| 关闭静态缓存（输出跟随输入几何） | 关闭静态缓存，缓存模型从模型树移除，直接查看输入网格的真实变形；再次点击会重新建立缓存 |

**模型属性面板（仿 ParaView 的 Properties 面板）**：**只有选中缓存模型 `xxx_ForceStaticMesh` 时**，「对象属性」里才会出现下面的勾选项（选中输入端模型或其它模型时该项不显示）：

- **Force Cache Computation**：勾选 = 每次都重新计算几何缓存（几何跟随当前时间步），勾选时会立即按当前帧重建一次；
  取消勾选 = 恢复复用缓存（几何固定在缓存建立时的那一帧）。

使用流程：

1. 加载网格模型（单块时序可直接加载 `.pvd` / `.igc`）。
2. 执行 **应用 / 更新静态网格缓存**：模型树出现新模型 `xxx_ForceStaticMesh`，提示中给出缓存几何来源的帧号与规模。
3. 拖动时间轴 / 播放：缓存模型的属性自动随帧更新，几何保持建立缓存时的那一帧，无需再次点击菜单。
4. **也可以直接选中缓存模型 `xxx_ForceStaticMesh` 再拖动时间轴**（与 ParaView 里选中管线输出节点一致）：
   缓存模型会复用输入端的时间轴，切帧时由过滤器重新执行 —— 几何固定、属性更新，
   同时输入模型也会同步显示当前帧的真实几何，便于两者对照。
5. 若某帧点数 / 单元数发生变化：自动重建几何缓存，并提示「几何规模变化（27 点 / 8 单元 → 64 点 / 27 单元），已自动重新建立几何缓存」。
6. 想主动改以当前帧几何为准：点 **重新建立几何缓存**。
7. 想看输入网格的真实变形：点 **关闭静态缓存**。

### 与 ParaView 的一致性

用同一份 `deform.pvd` 在 ParaView 6.2.0（内建 `Force Static Mesh` 过滤器）与 iGameVis 上逐帧比对，点数 / 单元数、点坐标、`pressure[0]` 完全一致；两者都在点数从 27 变为 64 时自动重建缓存并给出告警：

| 时间步 | 几何 | 属性 |
|--------|------|------|
| t=0 | 建立缓存（几何来自 t=0） | pressure[0]=0 |
| t=1 / t=2 | 保持 t=0 的几何（点 9 的 z 仍为 1，源数据为 1.5 / 2.0） | 100 / 200 |
| t=3 | 点数 27→64，缓存失效并自动重建 | 300 |

ParaView 侧基准可用脚本复现：`E:\ParaView 6.2.0\bin\pvpython.exe out\pv_fsm_test.py`。

## 使用示例

命令行示例（自动运行，无需手动输入参数）：

```bat
# 在 Examples 构建目录下运行（相对路径 ./Models 自动拷贝）
testForceStaticMesh.exe
```

验证内容（共 27 项检查，全部通过时输出 `Result: PASS`）：

1. 同一输入对象执行两次 → 缓存复用（输出为同一对象），且未重建几何。
2. 切换到点 / 单元数相同但几何不同的另一对象 → 强制重建（原因 `RR_INPUT_CHANGED`），几何对应新输入（原点位于 x=10）。
3. `ForceCacheComputation = true` → 强制重建（原因 `RR_FORCED`）。
4. 关闭静态缓存 → 输出直接等于输入对象。
5. 时序第 1 帧（t=0）建立缓存，记录「缓存几何来自 t=0」，几何为第 1 帧坐标。
6. 时序第 2 帧（几何变形，z 拉伸 1.5 倍）→ 缓存对象身份不变、未重建、几何仍是第 1 帧坐标（点 9 的 z 仍为 1，而不是 1.5）、属性已更新为第 2 帧的值。
7. 每次时间步更新后属性集被标记为已修改（时间戳递增）。
8. 时序第 3 帧 → 未重建、属性继续更新（pressure[0] == 200）。
9. 时序第 4 帧规模变化（27 点 / 8 单元 → 64 点 / 27 单元）→ 自动重建，原因 `RR_SIZE_CHANGED`，缓存来源帧更新为第 4 帧，状态消息包含规模变化提示，输出点数为 64。

运行输出 `Result: PASS` 表示通过。

> 构建提示：`Examples/CMakeLists.txt` 依赖 `find_package(HDF5 REQUIRED)`，本机如无可用的 HDF5，可用任意链接了 `iGameCore` 的最小工程编译同一个 `TestForceStaticMesh.cpp`（编译宏需与 iGameCore 一致：`IGAME_PLATFORM_WINDOWS`、`PLATFORM_WINDOWS`、`IGAME_OPENGL_VERSION_460`、`FFMPEG_ENABLE`），运行目录需能通过 `./Models/...` 访问上述素材。

示例源码：`Examples/Filter/ForceStaticMesh/TestForceStaticMesh.cpp`

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/ForceStaticMesh_TestA.vtk` | 2×2×2 六面体网格（27 点 / 8 单元），位于原点附近 |
| `Examples/Models/ForceStaticMesh_TestB.vtk` | 与 A 同规模的六面体网格，整体偏移到 x=10，标量场值不同 |
| `Examples/Models/ForceStaticMesh_TimeSeries/deform.pvd` | 时序集合：t=0/1/2 为同一拓扑的三种变形（z 拉伸、x 剪切），t=3 为规模变化帧（64 点 / 27 单元） |
| `Examples/Models/ForceStaticMesh_TimeSeries/deform_0.vtu` … `deform_2.vtu` | 同一拓扑（27 点 / 8 单元）的变形帧，`pressure` / `displacement` 逐帧不同 |
| `Examples/Models/ForceStaticMesh_TimeSeries/size_change.vtu` | 规模变化帧（64 点 / 27 单元），用于验证自动重建 |

GUI 验证建议：直接加载 `deform.pvd`，执行一次「应用 / 更新静态网格缓存」，然后逐帧播放：

- t=0/1/2：模型几何停在 t=0，云图随帧变化；
- 切到 t=3：提示出现「几何规模变化」，模型几何更新为 t=3 的形状；
- 点「关闭静态缓存」：缓存模型消失，输入网格按各帧真实几何显示。

## 注意事项

1. **仅适用于各时间步网格完全相同的数据**：几何会被固定在建立缓存（或最近一次重建）的那一帧；如果每帧几何都不同，看到的几何并非当前帧的真实形状。
2. **规模变化会打断静态语义**：点数 / 单元数一旦变化就必须重建几何（旧缓存的连接关系已经不适用），此时提示会明确告知；如果需要每帧都看真实几何，请关闭静态缓存。
3. **多块时序按块缓存**：块数变化视为输入改变，会整体重建；容器属性按同名补齐，属性值域按子块重新汇总。
4. **属性更新会标记缓存对象与渲染数据已修改**：属性集 `Modified()` + `ForceReConvertToDrawableData()`，避免云图停留在上一帧。
5. **缓存生命周期**：每个输入模型对应一份缓存；界面按输入模型分别维护过滤器实例与输出模型，重复执行只更新属性，不会无限新增模型。
6. **性能**：几何规模不变的帧只做属性深拷贝，不重建几何；关闭静态缓存时才每帧同步几何。
