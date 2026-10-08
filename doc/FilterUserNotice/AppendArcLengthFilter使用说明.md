# AppendArcLengthFilter 使用说明

## 功能

**Append Arc Length（追加弧长）** 过滤器，是 VTK `vtkAppendArcLength` 的移植实现。
它沿折线（多段线）逐点累加弧长，为输入数据追加一条点属性数组：

- 数组名：`arc_length`（`AppendArcLengthFilter::GetArrayName()`）；
- 挂载与类型：`IG_POINT` 点属性、`IG_SCALAR`，存储为 `FloatArray`（float32，见"与 VTK 的差异"）；
- 长度：等于输入点数，**所有点先置 0**，只有被折线单元覆盖到的点会被写入；
- 每条折线的**第一个点保持原有值**（VTK 不修改首点：首点保持 0；若该点已被其它折线写过，保留前一次的值）；
- 折线之间共享点时**按遍历顺序后者覆盖前者**，与 VTK `vtkAppendArcLength`
  （"cells must not share points" 只是警告）的实际覆盖行为一致；
- 未参与折线的点（只属于三角形/四面体/顶点等单元，或完全孤立）保持 0；
- 输出即输入本身：`GetOutput()` 与输入是同一个 `DataObject`，`arc_length` 存在同名数组时**原地替换**
  （数组对象、挂载位置、类型不变，因此色带/显示属性不会丢），否则新增。

`arc_length[p]` = 沿该点所在的折线，从折线首点走到 `p` 的各段欧氏距离之和（累加用 double，落盘时再截为
float32）。

## 折线来源

| # | 输入形态 | 折线判定 |
| --- | --- | --- |
| 1 | `UnstructuredMesh`（`.vtu` 的 `UNSTRUCTURED_GRID`、`.vtk` 非结构化网格等） | 单元类型为 `IG_LINE` / `IG_POLY_LINE`；其它类型（三角形、四边形、四面体、顶点……）跳过，被它们独占的点保持 0 |
| 2 | `SurfaceMesh` 的**显式边数组** | legacy `.vtk` `POLYDATA` 的 `LINES` 段被读进 `SurfaceMesh::SetEdges()`；要求边**不是面派生边**（`SurfaceMesh::AreEdgesDerivedFromFaces() == false`） |
| 3 | 其它（`PointSet` / `VolumeMesh` / `StructuredMesh` / 无折线的 `SurfaceMesh`） | 没有折线，写出全 0 数组（与 VTK 只处理 polydata `LINES` 一致） |

注意：`BuildEdges()`（渲染线框以及若干过滤器会调用）会用**面派生边**覆盖边数组，文件里读进来的折线
随之丢失、无法还原。此时本过滤器不再把边当作折线源（否则会把面网格的派生边误当成折线），
`arc_length` 全 0，可用 `GetNumberOfProcessedLines() == 0` 区分。

## 参数

无参数（与 VTK 一致）。对外接口：

```cpp
bool Execute() override;                     // 成功返回 true
const std::string& GetMessage();             // 失败原因（中文）
IGsize GetNumberOfProcessedLines();          // 本次实际处理的折线条数
static const char* GetArrayName();           // "arc_length"
```

## 调用方式

```cpp
#include <iGameFilterIncludes.h>   // 或 #include <AppendArcLength/iGameAppendArcLengthFilter.h>

auto filter = iGame::AppendArcLengthFilter::New();
filter->SetInput(input);                          // PointSet 及其子类
if (filter->Execute()) {
    const char* name = iGame::AppendArcLengthFilter::GetArrayName();   // "arc_length"
    IGsize lines = filter->GetNumberOfProcessedLines();
    // input 上已新增/替换 arc_length 点属性
} else {
    const std::string& why = filter->GetMessage();
}
```

Qt 入口：菜单 **Filters（过滤器）→ 追加弧长 (AppendArcLength)**。无参数，直接作用于当前模型：
成功后刷新模型树并选中 `arc_length`，提示 `Arc length appended: N point(s), M polyline(s).`

## 使用示例

### 1. GUI（iGameVis）

1. 打开折线模型（例如 `Examples/Models/AppendArcLength_test.vtk`，或任意含 `LINES` / `POLYLINES` 的 `.vtk` / `.vtp`）；
2. 菜单 **Filters → 追加弧长 (AppendArcLength)**（**无参数面板**，点一下直接执行）；
3. 输出模型自动进入模型树，并自动把 `arc_length` 设为当前数组，直接看到沿折线的渐变着色。

### 2. 核心库（C++）

```cpp
auto filter = iGame::AppendArcLengthFilter::New();
filter->SetInput(mesh);                    // 含折线单元的 PointSet / Mesh
// 输出数组名固定为 arc_length（AppendArcLengthFilter::GetArrayName()），没有 setter；
// 非折线输入时该列全为 0，与 VTK 对不含 lines 的 vtkPolyData 一致。
filter->Execute();
iGame::DataObject::Pointer out = filter->GetOutput(0);   // 点属性多出一列 double 的 arc_length
```

### 3. 测试程序

- 可视化（OpenGL 窗口）：`build-msvc\Release\testAppendArcLength.exe` —— 程序内生成 201 点螺旋折线 → 执行过滤器 →
  用 `arc_length` 着色显示；实测 `first=0`、`last=127.225`、解析总长 `127.245`（折线弦长略短于光滑螺旋，符合预期）。
- 无窗口自检：`build-msvc\Release\testAppendArcLengthChecks.exe` —— **15 / 15 通过**（exit 0），双击运行时末尾会暂停等待回车。

### 4. 演示视频

- `doc/FilterUserNotice/videos/追加弧长.mp4` —— iGameVis 与 ParaView 运行同一模型的效果对比；
- `doc/FilterUserNotice/videos/两个测试程序-追加弧长和直方图.mp4` —— 两个测试程序的运行录像。

## 注意事项

- 只处理**折线类单元**（`IG_LINE` / `IG_POLY_LINE`，对应 `vtkPolyData::GetLines()`）；其它单元（顶点/三角/四边形/多边形/体单元）不参与，其顶点保持 0，没有折线单元的输入整列全为 0 —— 与 VTK 一致。
- 结果只挂在**点属性**上：名为 `arc_length`、1 分量、`FloatArray`（iGameVis 的点坐标固定 float32，与 VTK 的 double/float 按输入精度选择等价）；单元属性不变。
- 与 VTK 相同的限制：假设折线之间**不共享点**；共享时后处理的折线会覆盖前一条的结果。
- 来自 legacy VTK `POLYDATA` 的 `LINES` 段会被读成 `SurfaceMesh` 的边数组；一旦对其执行过 `BuildEdges()`（渲染线框等会触发），边数组会被“面派生边”覆盖，折线就找不回来了。
- 段内长度按**折线弦长**累计（不是光滑曲线的弧长），与 VTK `vtkAppendArcLength` 语义一致。
- 修复前加载超过 256 点的折线模型会闪退（`iGameSurfaceMesh` 的折线段处理），本 PR 一并修复，详见「附 2」。

## 数值验证（与 VTK / ParaView 逐点对照）

对照环境：ParaView 6.2.0（VTK 9.7.0）`pvpython` + `vtkPolyDataReader` + `vtkAppendArcLength`；
两侧都导出 `point_id,x,y,z,arc_length`（`%.9g`）后逐点比对。

| 用例 | 输入 | 折线数 | 逐点一致 | max_abs | 备注 |
| --- | --- | --- | --- | --- | --- |
| A | `POLYDATA` 12 点，`LINES` 4 条（3 / 4 / 2 / 2 点，含共享点 1、11） | 4 | 12 / 12 | 0 | 与 VTK 位级相同；共享点按遍历顺序覆盖 |
| B | `POLYDATA` 4 点 + 1 个三角形，无 `LINES` | 0 | 4 / 4 | 0 | 两侧全 0 |
| C | `UNSTRUCTURED_GRID`：`POLY_LINE`(3 点) + `TRIANGLE` + `LINE`(2 点) + `VERTEX` | 2 | 9 / 9 | 0 | VTK 的该过滤器只接受 polydata，改用独立 Python 参考实现（同一算法、float32 落盘）对照 |
| D | `POLYDATA` 同时含 `LINES`（4 条）与 `POLYGONS`（1 个） | 4 | 12 / 12 | 0 | 面单元不参与，被它独占的点为 0 |
| E | 公开样例 `dec.vtk` / `ge.vtk` / `gm.vtk` / `ibm.vtk`（一条 345~348 点的长折线） | 1 | 347 / 348 / 345 / 347 全中 | 0 | 见下文「真实/公开数据实测」 |
| F | 自建 `helix_2000.vtk`（2000 点螺旋折线，由 `vtkPolyDataWriter` 写出） | 1 | 2000 / 2000 | 0 | 验证不受 `IGAME_CELL_MAX_SIZE`(256) 限制 |
| G | 只要多边形的 `k.vtk` / `polyex.vtk` / `t.vtk` / `v.vtk` | 0 | 166 / 8 / 64 / 100 全中 | 0 | 两侧全 0 |

用例 A 的实测输出（`arc_length` 列）：

```text
0:#000 0   1:(1,0,0) 9.48683262   2:(1,2,0) 3   3:#000 0
4:(1,1,0) 1.41421354   5:(2,1,0) 2.41421366   6:(3,1,0) 3.41421366
7:(5,5,5) 0   8:(-1,0,0) 0   9:(-1,-1,0) 0   10:(10,0,0) 0   11:(10,0,3) 9.48683262
```

- 折线 `(0,1,2)`：点 2 得 `|1-0| + |2-1| = 1 + 2 = 3`；
- 折线 `(3,4,5,6)`：`√2 ≈ 1.41421354`、`+1 ≈ 2.41421366`、`+1 ≈ 3.41421366`；
- 折线 `(10,11)` 先把点 11 写成 `3`，随后折线 `(11,1)` 再把点 11 与点 1 一起覆盖为
  `|(1,0,0)-(10,0,3)| = √90 ≈ 9.48683262`（VTK 实测同样的 `9.48683262`）。

重复执行：同名数组只保留 1 条（实测 `same-name arrays = 1`），值、折线数与首次执行相同。

### 真实/公开数据实测（含超过 256 点的长折线）

公开样例来自 John Burkardt 的 VTK 示例数据目录（`https://people.sc.fsu.edu/~jburkardt/data/vtk/`，
逐点比对时下载到仓库外目录使用）：

| 文件 | 点数 | 折线数 | `arc_length` 范围 | 与 VTK 逐点一致 |
| --- | --- | --- | --- | --- |
| `dec.vtk`（DEC 股票收盘曲线，一条长折线） | 347 | 1 | [0, 659.554] | 347 / 347，max_abs = 0 |
| `ge.vtk` | 348 | 1 | [0, 632.288] | 348 / 348，max_abs = 0 |
| `gm.vtk` | 345 | 1 | [0, 678.422] | 345 / 345，max_abs = 0 |
| `ibm.vtk` | 347 | 1 | [0, 697.393] | 347 / 347，max_abs = 0 |
| `hello.vtk`（15 条短折线） | 22 | 15 | [0, 2] | 22 / 22，max_abs = 0 |
| `vtk.vtk`（5 条短折线） | 12 | 5 | [0, 4.47214] | 12 / 12，max_abs = 0 |
| `helix_2000.vtk`（自建 2000 点螺旋） | 2000 | 1 | [0, 804.36] | 2000 / 2000，max_abs = 0 |
| `k.vtk` / `polyex.vtk` / `t.vtk` / `v.vtk`（只有多边形） | 8 ~ 166 | 0 | [0, 0] | 全部一致（两侧全 0） |

> 这些长折线暴露并修复了一个真实缺陷：最初实现把单元点集拷进 `igIndex[IGAME_CELL_MAX_SIZE]`，
> 而该常量只有 **256**，347 点的折线会写穿栈、在函数返回时触发 /GS 检查（进程退出码 `0xC0000409`，
> 结果 CSV 不落盘）。现在改用返回内部指针的 `CellArray::GetCellIds(cellId, const igIndex*&)` 重载
> （零拷贝、不受单元点数上限限制），2000 点折线实测正常且与 VTK 位级一致。

## 回归测试用例（`Examples/Filter/AppendArcLength/TestAppendArcLengthChecks.cpp`）

- 测试模型（每个 filter 只用一个）：`Examples/Models/AppendArcLength_test.vtk`（8 个点）
  - 折线 A：(0,0,0)-(3,0,0)-(3,4,0) → 弧长 0 / 3 / 7（3-4-5 直角，数值可手算核对）
  - 折线 B：(10,0,0)-(13,0,0) → 弧长 0 / 3
  - 三角形：(1,5,0)-(2,6,0)-(3,7,0) → 不是线单元，顶点保持 0
- 期望值取自 VTK `vtkAppendArcLength` 实测：`arc_length = [0, 3, 7, 0, 3, 0, 0, 0]`（1 分量、float、点属性）。
- 断言覆盖：输出点数、数组名 / 维数 / 附着位置、逐点数值、非线单元保持 0、
  重复执行按覆盖语义处理（不堆积同名数组）。
- 运行方式（**直接可运行的 exe**，不依赖 `Examples` / HDF5）：

  ```powershell
  cmake -S . -B build -DIGAME_BUILD_FILTER_CHECKS=ON     # 开关默认 OFF，不影响常规构建
  cmake --build build --config Release --target testAppendArcLengthChecks
  .\build\Release\testAppendArcLengthChecks.exe          # 失败返回非 0，可直接双击
  ```

  测试模型随 exe 拷到 `<exe 目录>/Models/`，exe 自动在多个候选路径里定位模型，任意工作目录都能运行。
  具备 HDF5 的环境也可以走 `Examples` 里的同名 target（`-DEXAMPLE_COMPILE=ON` + 工作目录 `Examples`）。
- 实测结果：在 `build-msvc\Release` 直接运行 → **通过 15 / 15，失败 0**（exit 0）。

**可视化测试（OpenGL 窗口）**：`Examples/Filter/AppendArcLength/TestAppendArcLength.cpp` →
`build-msvc\Release\testAppendArcLength.exe`（写法与 `Examples/Filter/Convert/TestPointSetToOctree.cpp` 一致）。

- 程序内生成 201 点螺旋折线（r=10、高 20、2 圈）→ 执行过滤器 → 输出加入场景并用 `arc_length` 着色显示；
- 控制台同时打印首/末点弧长与解析总长：实测 `first=0`、`last=127.225`、解析值 `127.245`（差 0.020 =
  折线弦长略短于光滑螺旋，符合预期）；
- 构建：`cmake --build build --config Release --target testAppendArcLength`；实测窗口启动后持续运行 15 秒以上无异常退出。
- 本机限制：当前 checkout 缺少 HDF5，`Examples` 目录无法配置，所以本机验证走的是上面的独立 exe 路径。

## 附 2：随之修复的渲染端缺陷（加载折线模型闪退）

加载 `dec.vtk` 这类"**`LINES` 段含 n>2 点多段线**"的 legacy `POLYDATA` 时程序会直接闪退
（实测退出码 `0xC0000409` = STATUS_STACK_BUFFER_OVERRUN，Windows 事件日志记为"应用程序错误"）。
根因不在本过滤器，而在取线框索引的老代码 `SurfaceMesh::GetDrawableArray()`：

- 它声明 `igIndex cell[IGAME_CELL_MAX_SIZE]`（该常量只有 **256**）后调用 `GetEdgePointIds()` /
  `GetFacePointIds()`，而后者内部是 `CellArray::GetCellIds(cellId, buf)`——按单元**真实点数**写入调用方缓冲；
- 347 点的折线写穿 256 的栈缓冲，函数返回时 /GS 检查失败 → 闪退（点数越多越必现）；
- 该函数"裁剪关/裁剪开"两个分支各有一份同样的写法。

修复：两处边/面遍历都改用返回内部指针的 `CellArray::GetCellIds(cellId, const igIndex*&)` 重载，
边按相邻点拆成线段（2 点单元与原来的 `(cell[0], cell[1])` 完全等价，n 点多段线则得到正确的折线线框）；
并把 `SurfaceMesh::GetEdgePointIds()` 的拷贝限定为最多 2 个点（它本来就 `return 2`），
保护其余以 `igIndex e[2]` 调用它的内部站点。

实测（`build-msvc\Release\iGameVis.exe --filepath <model>`，各加载 14 s 判活；括号内为渲染区
高亮像素统计，用空场景/面模型截图对比得出）：

| 模型 | 点数 / 折线数 | 修复前 | 修复后 |
| --- | --- | --- | --- |
| `dec.vtk` | 347 / 1 | 加载即闪退 `0xC0000409` | 存活；视图内可见该曲线（约 2.5k 高亮像素，横向跨 888 px） |
| `helix_2000.vtk` | 2000 / 1 | —— | 存活 |
| `hello.vtk` | 22 / 15 | —— | 存活 |
| `v.vtk`（只有多边形） | 100 / 0 | —— | 存活；面渲染正常（约 20 万高亮像素） |
| `Examples/Models/AppendReduce_mesh1.vtk` | 常规面网格 | —— | 存活（拆分逻辑对 2 点边与原来等价） |

## 与 VTK 的差异

1. **输出存储类型固定为 float32**：VTK 按输入点的精度选择 `vtkFloatArray` / `vtkDoubleArray`；
   本实现固定写 `FloatArray`。float 点输入（legacy `.vtk` 的 `POINTS n float`、`.vtu` float32）两边都是
   float32，逐点位级一致（上表 `max_abs = 0`）；若输入点是 double，VTK 会写 double 数组，本实现仍按
   float32 落盘（相对误差量级 `1e-7`）。本过滤器当前只处理 float 点输入。
2. **折线集合的两处来源差异**：VTK 只看 `vtkPolyData::GetLines()`；本实现除 `SurfaceMesh` 的显式边数组
   之外还支持 `UnstructuredMesh` 的 `IG_LINE`/`IG_POLY_LINE` 单元（VTK 的该过滤器不接受非 polydata
   输入，所以这类输入在 ParaView 里无法直接对照，见表中用例 C）。
3. **被 `BuildEdges()` 覆盖过的折线无法还原**：见"折线来源"一节；VTK 不存在这一步，故只在 iGameVis
   的渲染/过滤器链路中出现。

## 附：随之修复的读取缺陷

legacy `.vtk` `POLYDATA` 的 `LINES` 段由 `VTKAbstractReader::CreateCellArray()` 建 `CellArray`，
原实现有两处问题（与 VTK 无关，属本项目读取端缺陷，本特性需要它才能读到正确的折线）：

1. 复用同一个成员 `m_CellArray`：`LINES` 与 `POLYGONS` 两个 `SetEdges`/`SetFaces` 拿到**同一个数组对象**，
   后写入的段覆盖前一段；
2. `CellArray::Reset()` 不重置 `m_NumberOfCells` / `m_FixedCellSize` / `m_UseOffsets`，也不补回
   偏移数组的起始 `0`，导致**单元点数不一致**的段（如 3 / 4 / 2 / 2 点的折线）偏移整体错位：
   实测第一条折线 `GetCellIds()` 读出 `3 4 5 11`（应为 `0 1 2`），第 2 条起更会读到越界尺寸
   （`cellSize = 3607908491`）并让过滤器崩溃。

修复方式：`CreateCellArray()` 每次新建 `CellArray`。修复前后对比（用例 A）：

| | 读到的折线连通性 | `arc_length` 与 VTK |
| --- | --- | --- |
| 修复前 | `[3 4 5 11]`（错位，后续越界） | 进程崩溃（0xC0000005 / 0xC0000409） |
| 修复后 | `[0 1 2]`、`[3 4 5 6]`、`[10 11]`、`[11 1]` | 12 / 12 位级一致 |

顶点数一致的段（例如全部 2 点线、全部 4 点四边形）本来就正确，所以该缺陷此前未被发现。
