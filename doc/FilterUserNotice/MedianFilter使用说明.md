# MedianFilter（中值滤波）使用说明

> - 头文件：`iGameCore/Filters/Median/iGameMedianFilter.h`
> - 实现文件：`iGameCore/Filters/Median/iGameMedianFilter.cpp`
> - 界面控件：`Qt/include/IQWidgets/igQtMedianFilterWidget.h`、`Qt/src/IQWidgets/igQtMedianFilterWidget.cpp`、`Qt/Resources/UI/MedianFilter.ui`
> - 界面入口：菜单 **算法处理 → 中值滤波 (Median)**，弹出面板标题 **中值滤波（Median）**，停靠到左侧后页签为 **中值滤波**
> - 自动测试示例：`Examples/Filter/Median/TestMedianFilter.cpp`（CMake 目标 `testMedianFilter`）
> - 配套测试模型：`Examples/Models/3DScalar.vti`（21×21×21 结构化标量场，点标量 `values`）

---

## 一、功能

对**结构化网格（StructuredMesh）的点标量**做核窗口内邻域中值替换：用 `kx × ky × kz` 窗口内所有有效邻点值的中位数，替换窗口中心点的值。语义上对标 ParaView 的 Median 滤波器，用于**去噪、抹平毛刺**，同时尽量保留边界与梯度台阶（不像均值滤波那样把阶跃抹成斜坡）。

- 输入与输出均为结构化网格，**点数与 `DIMENSIONS` 完全一致**（`EXTENT` 受框架缺陷影响，见第七节第 10 条），不改拓扑、不改坐标；
- 只处理**点关联（`IG_POINT`）的单分量标量（`IG_SCALAR`）**，多分量请先用 ExtractComponent 拆分量；
- 核大小各维取**奇数**（默认 `3 × 3 × 3`），2D 数据第三维固定为 `1`；
- 窗口越界时**直接跳过**（不补零、不复制边界值），因此边界点的有效邻点少于内部点；
- 排序与写回都用**原数据类型**（`float` 进 `float` 出，不经 `double` 中转）；
- **输出数组与输入同名、同属性类型、同数组类型**，属性面板中显示的类型与范围口径与原模型一致（不会出现"未知类型"）；
- 生成独立输出节点 `<输入名称>_Median`，不修改输入对象。

**适用场景**：标量场去噪（压力、温度、速度分量等）、去除单点尖刺、在阈值/等值面前做预平滑、把结构化 CFD/CGNS 结果处理成可视化更干净的场。

---

## 二、参数与接口

### 2.1 接口一览

| 接口 | 说明 |
| --- | --- |
| `static Pointer New()` | 创建滤波器（构造时固定 1 输入 / 1 输出） |
| `SetInput(DataObject::Pointer)` | 设置输入数据（基类接口），需为 `StructuredMesh` |
| `SetAttributeByIndex(int index)` | 按属性下标选择要滤波的标量 |
| `SetAttributeByName(const std::string& name)` | 按属性名选择要滤波的标量 |
| `SetKernelSize(int kx, int ky, int kz)` | 设置核大小，各维必须为奇数且 ≥ 1 |
| `bool Execute()` | 执行，失败返回 `false` |
| `DataObject::Pointer GetOutput()` | 取输出（新的 `StructuredMesh`） |
| `std::string GetMessage()` | 取失败原因，**英文短句**（如 `Kernel size must be an odd positive number in every dimension.`）。文案用英文是因为 `std::string` 是裸字节：中文字面量在不同源文件编码 / 控制台代码页（GBK 与 UTF-8）之间会显示成乱码；**每次 `Execute()` 开头会清空**，成功时为空字符串，因此也可用 `GetMessage().empty()` 判断本次执行没有报错 |
| `SetProgressRange(double shift, double scale)` | 基类接口，把内部 0→1 进度映射到全局进度区间 |

### 2.2 参数与默认值

| 参数 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| 标量选择（下标） | `int` | `-1` | **`≥ 0` 时优先按下标选择**，此时属性名被忽略 |
| 标量选择（名称） | `std::string` | 空 | 下标为 `-1` 时按名称在属性集中查找 |
| 核大小 `kx, ky, kz` | `int` | `3, 3, 3` | 各维必须为奇数且 ≥ 1；2D 数据（第三维 ≤ 1）会强制 `kz = 1` |

界面上的控件与上述参数一一对应：

| 控件 | 取值范围 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `标量数组`（下拉框） | 模型的全部属性 | 第 1 项 | 列出**所有**属性（含矢量、单元属性），不合法选择由滤波器给出具体原因 |
| `核大小` X / Y / Z | `1 ~ 99`，步长 `2` | `3 / 3 / 3` | 步长为 2 保证只能取奇数；2D 数据时 Z 被禁用并固定为 1 |
| 类型标签 | — | — | 单分量标量时显示 `类型: float \| 范围: [min, max]`，用于确认输入标量类型 |
| `执行` 按钮 | — | — | 执行期间在状态栏进度条上显示文案"中值滤波中"，并刷新 0→100% 进度 |

---

## 三、调用方式

### 3.1 代码调用

```cpp
#include "Median/iGameMedianFilter.h"
#include "iGameFileIO.h"
#include "iGameStructuredMesh.h"

// 输入必须是结构化网格 + 点标量
auto input = iGame::FileIO::ReadFile("./Models/kit.vtk");
if (!input) { return 1; }

auto filter = iGame::MedianFilter::New();
filter->SetInput(input);              // 基类接口：输入为 DataObject
filter->SetAttributeByName("ke");     // 也可用 SetAttributeByIndex(下标)
filter->SetKernelSize(3, 3, 3);       // 各维必须为奇数；2D 数据第三维会被强制为 1
filter->SetProgressRange(0.0, 1.0);   // 可选：把内部进度映射到 [0, 1]

if (!filter->Execute()) {
    std::cerr << "中值滤波失败：" << filter->GetMessage() << std::endl;
    return 1;
}

auto out = iGame::DynamicCast<iGame::StructuredMesh>(filter->GetOutput());
std::cout << "输出模型：" << out->GetName()                 // 形如 kit_Median
          << "，点数 " << out->GetNumberOfPoints() << std::endl;
```

### 3.2 界面调用

1. 加载数据（`.vtk` / `.cgns` 等），在模型树中选中该模型；
2. 菜单 **算法处理 → 中值滤波 (Median)**，打开左侧"中值滤波"面板；
3. 在 `标量数组` 中选择要滤波的点标量（标签会显示其类型与范围，便于确认选对了字段）；
4. 设置 `核大小` X / Y / Z（保持奇数）；
5. 点击 `执行`：结果以 `<输入名称>_Median` 加入模型树，并按滤波后的标量着色；
6. 可调整核大小后**反复执行**——面板基于**原始模型**重新计算，并**更新同一个结果模型**，不会不断产生新模型；
   若把结果模型删除，再次执行会重新创建它。

> 提示：面板的"类型/范围"标签读的是**原模型**的选中属性；滤波结果的属性类型与之完全相同，因此可用它预测结果字段的类型。
>
> 面板在执行前还会做三项预检并直接弹窗（不进入滤波器）：未选标量、输入不是结构化网格、核大小含偶数。其余不合法情况（矢量/单元属性、多分量、属性失效等）由滤波器返回 `GetMessage()` 文案，面板原样弹出。

---

## 四、输出说明

输出为新的 `StructuredMesh`，名称 `<输入名称>_Median`，几何与拓扑与输入一致（共享点坐标、重建结构化连接关系），仅替换被选中的那一个标量数组。

### 4.1 属性字段与原模型逐项对齐

| 字段 | 输出 | 说明 |
| --- | --- | --- |
| 名称 | 与输入同名（如 `ke`） | 不追加后缀，属性面板与下拉框里名字可直接对应 |
| 属性类型 `type` | 与输入相同 | 原样拷贝（本滤波器下恒为 `IG_SCALAR`） |
| 关联 `attachmentType` | 与输入相同 | 原样拷贝（本滤波器下恒为 `IG_POINT`） |
| 分量数 `dimension` | `1` | 单分量标量 |
| 数组类型 `GetArrayType()` | **与输入相同** | 由具体数组类构造，属性面板显示 `float` / `double` / `int` … 而非"未知类型" |
| 数值 | 窗口内中值 | 以原数据类型比较与写回，不经 `double` |
| 数值范围 | 按新数值统计 | 不继承原范围（避免残留），由使用方惰性统计 |

其余属性（其它点标量、点矢量、单元属性等）**原样保留**，类型、关联与范围字段均沿用输入，顺序也与输入一致；被删除的属性槽位会被跳过。

### 4.2 支持的数组类型

`char`、`unsigned char`、`short`、`unsigned short`、`int`、`unsigned int`、`long long`、`unsigned long long`、`float`、`double`。
其它类型（如内部 Id 数组）会被拒绝并提示 `Unsupported scalar array type.`。

---

## 五、算法说明

1. 由 `DIMENSIONS` 取 `nx, ny, nz`（`nz ≤ 1` 时按 1 处理），点索引为 `i + j·nx + k·nx·ny`；点数与 `nx·ny·nz` 不符则判定为 `Median computation failed.`。
2. 以每个点为中心，遍历 `[-k/2, +k/2]`（整数除法）的窗口，收集**落在网格范围内**的邻点值到同类型缓冲区。
3. 对候选值按**原类型升序排序**，取 `count / 2`（0-based）作为结果——即**上中位数**：候选个数为奇数时是严格中位数，边界处候选个数为偶数时取偏大的那个。
4. 结果以原类型写回输出数组，全程不经过 `double`，因此整型标量不会出现精度或截断问题。
5. 进度按点序号每约 1% 回调一次（`onProgress`），最终进度置 1.0。

**与"补零/复制边界"实现的差异**：本实现越界即跳过，边界点的窗口自动收缩（有效候选变少但都是真实邻点），因此边界不会出现被 0 或边界值拉偏的中值；相应地，边界附近的中值比内部点更接近原始值（平滑更弱）。

---

## 六、示例程序与自动测试

### 6.1 示例程序

`Examples/Filter/Median/TestMedianFilter.cpp`（CMake 目标 `testMedianFilter`）是一个**无渲染窗口**的控制台自检程序，既可当自动测试跑，也可当调用示例读。它共 **76 项 check**（全部以英文打印，避免控制台代码页乱码），分四组场景：

| 场景 | 输入 | 覆盖点 | check 数 |
| --- | --- | --- | --- |
| Test 1 合成 5×1×1 网格（`IntArray`，值 `{1, 2, 100, 4, 5}`） | 代码构造，无外部依赖 | 确定性中值结果 `{2, 2, 4, 5, 5}`；边界窗口收缩且取上中位数；整型数组类型保留；输入未被改写；`3×3×3` 在 `ny=1` 时等价于 `3×1×1`；成功时 `GetMessage()` 为空 | 19 |
| Test 2 合成 3×3×3 网格（`FloatArray`，`P[0]=100`、`P[13]=-100`） | 代码构造 | 3D 索引约定与核窗口；正负尖刺被完全抹平（27 个值全为 0） | 4 |
| Test 3 真实模型 `3DScalar.vti` | `Examples/Models/3DScalar.vti` | 属性字段与原模型逐项一致、采样点与独立参考实现交叉校验、平滑性（总变差下降）、输出可再次滤波、输入未被改写 | 39 |
| Test 4 失败路径 | 合成网格 + 空 `UnstructuredMesh` | 非结构化网格 / 偶数核 / 无效属性名 / 矢量属性 / 多分量标量 / 数组长度不足各自给出对应提示；同一实例"先失败再成功"时 `GetMessage()` 必须被清空（无残留文案） | 14 |

逐条 check 在验证什么、为什么这样断言有效，见 6.5。

### 6.2 配套测试模型

| 模型 | 规模 | 字段 | 用途 |
| --- | --- | --- | --- |
| `Examples/Models/3DScalar.vti` | VTK XML ImageData `WholeExtent="0 20 0 20 0 20"`，21×21×21 点 / 8000 单元，坐标 (0,0,0)~(1,1,1) | 点标量 `values`（Float64 × 1，∈ [-0.99873500687, 0.99218897372]）；点标量 `vtkValidPointMask`（Int8 × 1，全 1） | 真实结构化标量场：默认 3×3×3 与 5×5×5 中值滤波、属性字段一致性、数值平滑性 |

模型由 `.vti` 读取器（`iGameVTIReader`）承载为 `StructuredMesh`，正好命中本滤波器唯一支持的输入类型；`igame_add_example` 会自动把 `Examples/Models` 拷到构建目录的 `Models/`，示例代码用相对路径直接定位，**无需手动输入**。

### 6.3 运行方式

```powershell
# 构建（需 EXAMPLE_COMPILE=ON）
cmake --build build --config Release --target testMedianFilter --parallel
# 运行（工作目录需能看到 ./Models，CTest 会自动设置）
build\Examples\Release\testMedianFilter.exe
```

也可以经 CTest 运行（`Examples/CMakeLists.txt` 里所有 `igame_add_example` 目标都会自动注册）：

```powershell
ctest --test-dir build -R testMedianFilter --output-on-failure
```

需要换模型时可直接传路径：`testMedianFilter.exe D:\data\myField.vti`。

### 6.4 预期输出与实测数据

正常结束时每项检查打印 `[PASS]`，末尾打印检查计数与 `ALL TESTS PASSED`（任一项失败则是 `[FAIL]` 与 `SOME TESTS FAILED`，进程返回非 0，可直接进 CI）：

```text
== Test 3: real model (./Models/3DScalar.vti) ==
  >> read model
  [PASS] FileIO::ReadFile returns a data object
  [PASS] input is a StructuredMesh (ImageData -> StructuredMesh)
      dimensions=21x21x21 points=9261
  ...
  [PASS] array type identical to input (GetArrayType() == IG_DoubleArray)
  [PASS] output array is a concrete class (not IG_ARRAY_OBJECT)
  ...
      input  range=[-0.998735, 0.992189] TV=3096.43
      3x3x3  range=[-0.699384, 0.911234] TV=2192.29 changed=7541/9261
      5x5x5  range=[-0.556457, 0.796612] TV=1525.65 changed=9082/9261
  ...
N/N checks passed
ALL TESTS PASSED
```

失败路径的打印（消息为英文，示例）：

```text
== Test 4: failure paths ==
  >> non-structured input
      message: Median filter only supports a structured mesh (StructuredMesh).
  [PASS] Execute() rejects a non-structured input
  [PASS] message points at the structured-mesh requirement
  >> even kernel size
      message: Kernel size must be an odd positive number in every dimension.
  [PASS] Execute() rejects an even kernel size
  [PASS] message points at the odd-kernel requirement
  ...
  [PASS] the same instance then succeeds and GetMessage() is cleared (no stale failure text)
```

`3DScalar.vti` 上的实测参考数据（用于核对"滤波确实变平滑了"）：

| 核大小 | 数值范围 | 总变差 TV | 变化点数 |
| --- | --- | --- | --- |
| 原模型 | [-0.998735, 0.992189] | 3096.43 | — |
| 3×3×3 | [-0.699384, 0.911234] | 2192.29 | 7541 / 9261（81.4%） |
| 5×5×5 | [-0.556457, 0.796612] | 1525.65 | 9082 / 9261（98.1%） |

> 上表是"这个模型"的实测值。测试里的断言分两类：
>
> - **与数据无关的硬性质**（换任何模型都成立）：输出字段与原模型逐项一致、几何/点数不变、输出值全部落在输入值域内、采样点与独立参考实现完全一致、输出可再次作为输入、输入未被改写、失败路径的提示正确；
> - **针对 `3DScalar.vti` 的固定断言**（换模型需同步调整）：尺寸 21×21×21 / 9261 点、`values` 为 `IG_DoubleArray`、`changed > 0`、`TV(5×5×5) < TV(3×3×3)`。

### 6.5 逐条 check 说明（在做什么、为什么有效）

76 项 check 都写成"正确实现必然通过、写错必然失败"的形式。支撑其有效性的三条原则：

1. **手算真值**：Test 1 / Test 2 的期望值是按"窗口收缩 + 排序取 `count/2`"在纸上算出来的（与代码无关），所以它们能同时钉住索引约定、边界规则和中位数取法；
2. **与实现无关的硬不变量**：值域、总变差、点数/维度/坐标、属性字段的语义 —— 这些对任何正确实现都成立，一旦违反必是错的；
3. **独立参考实现交叉校验**：`ReferenceMedian()` 单独书写（走 `GetElementValue` 的 double 通路，而滤波器走类型化 `RawPointer`），与滤波器输出**逐位精确**比对。

#### Test 1（19 项）：5×1×1 线，`IntArray`，手算真值

| # | check | 在验证什么 | 为什么有效 |
| --- | --- | --- | --- |
| 1 | `input scalar 'P' is an IntArray` | 输入确实是整型数组 | 后面"类型保留"的断言只有在输入为 `IntArray` 时才有意义；若输入先被降级，这条会先失败，避免误判成滤波器的问题 |
| 2 | `Execute() succeeded` | 最小可用性 | 输入已满足全部前置条件（结构化网格、点标量、单分量、奇数核），正确实现必须成功 |
| 3 | `GetMessage() is empty after a successful Execute()` | 成功时不留残文案 | 修复前 `Execute()` 开头写死文案、成功不清空，这条必然失败；是"消息清理"的直接哨兵 |
| 4 | `output name == <input name>_Median` | 命名契约 | 模型树与下游按名字定位结果，名字错是接口级回归 |
| 5 | `output dimensions == 5x1x1 (unchanged)` | 维度不变 | 维度错位会让下游按错尺寸解释数据（点数与维度必须自洽） |
| 6 | `output point count == 5 (unchanged)` | 点数不变 | 防止实现顺手重建/复制点，导致几何与属性长度不再对齐 |
| 7 | `output keeps an attribute named 'P'` | 属性名保留 | 名字变了，界面下拉框与按名取数组都会失效 |
| 8 | `output attribute count == input attribute count` | 不丢也不多属性 | 与 #7 互补：#7 只查一个名字，#8 兜住"其它属性被丢掉"或"平白多出一个" |
| 9 | `output array name == input array name` | 数组对象自身的名字 | 属性名与数组名是两个字段，界面与写出用的是数组名，必须一起对 |
| 10 | `output attribute type == input (IG_SCALAR)` | 属性类型语义 | `IG_SCALAR` 决定它出现在标量下拉框、可参与着色；被写成 `IG_VECTOR` 会直接改变语义 |
| 11 | `output attachment type == input (IG_POINT)` | 关联类型语义 | 若被写成 `IG_CELL`，界面与后续滤波会按单元属性处理，数量语义直接错 |
| 12 | `output component count == 1` | 分量数 | 中值只对单分量有定义；分量数变了等于换了字段 |
| 13 | `output array type == IG_IntArray (same as input)` | **具体数组类型（本次 bug 的核心断言）** | 旧实现 `FlatArray<T>::New()` 造出的是模板基类，`GetArrayType()` 返回 `IG_ARRAY_OBJECT(0)`，属性面板因此显示"未知类型"；这条必然失败 |
| 14 | `output GetArrayType() == input GetArrayType()` | 输出类型 = 输入类型的对称形式 | 不写死枚举值，换输入类型（`float`/`double`/…）时依然成立，符合"同类型进出"的契约 |
| 15 | `output has one value per point (5)` | 数组长度 | `Resize`/写入索引写错会少写或越界，长度是最直接的护栏 |
| 16 | `filtered values == {2, 2, 4, 5, 5}` | 数值 + 两条规则 | 手算真值：i=2 的窗口 `{2,100,4}` → 4（尖刺被去掉）；i=0 窗口收缩为 `{1,2}` → **上中位数 2**（不是平均 1.5）；i=4 → 5。任何"补零/镜像边界/下中位数/取平均"的实现都会失败 |
| 17 | `input array keeps its original values (1, 2, 100, 4, 5)` | 纯函数式：不改输入 | 防止将来改成 in-place 实现——那会污染上游模型和其它滤波器的输入 |
| 18 | `Execute() with 3x3x3 succeeded` | 退化维度（`ny=1`）也能跑 | 窗口比网格大时必须自然退化，不能因为"邻域为空/越界"就报错 |
| 19 | `3x3x3 on a 5x1x1 grid gives the same result as 3x1x1` | 越界邻点被跳过（不补零、不复制边界） | 若 y 方向补零或复制边界，`3×3×3` 会引入 0 或边界值，结果必然与 `3×1×1` 不同 |

#### Test 2（4 项）：3×3×3 网格上的正负尖刺

| # | check | 在验证什么 | 为什么有效 |
| --- | --- | --- | --- |
| 20 | `Execute() succeeded` | 3D 网格可用 | `nz>1` 时走三维窗口分支（`kz` 不再被强制成 1），是 Test 1 之外的代码路径 |
| 21 | `output array type == IG_FloatArray (same as input)` | 浮点路径同样保留类型 | 与 Test 1 的 `IntArray` 互补，证明"类型保留"不是整型特例 |
| 22 | `both spikes are removed: all 27 filtered values == 0` | 3D 索引约定与窗口正确 | 手算真值：尖刺只在 (0,0,0) 与 (1,1,1)，任何包含尖刺的窗口里尖刺都只有 1 个候选，8/12/27 个候选的中位数必为 0。若索引步长写错（例如 x/y/z 步长混用），窗口会取到无关的点，输出不再是全 0 |
| 23 | `input spikes are still there (100 at 0, -100 at 13)` | 输入未被改写 | 与 #17 互补，覆盖浮点数组路径 |

#### Test 3（39 项）：真实模型 `3DScalar.vti`

| # | check | 在验证什么 | 为什么有效 |
| --- | --- | --- | --- |
| 24 | `FileIO::ReadFile returns a data object` | 模型读得进来 | Test 3 的全部结论都建立在这份输入上，读失败必须先报出来 |
| 25 | `input is a StructuredMesh (ImageData -> StructuredMesh)` | 输入类型符合滤波器唯一支持的输入 | 顺带回归 `.vti` 读取器（ImageData → `StructuredMesh`）这条链路 |
| 26 | `input dimensions == 21x21x21` | 与模型文件 `WholeExtent` 一致 | 钉子：保证后面比较的是"预期的这份数据"，而不是读错维度的数据 |
| 27 | `input point count == 9261` | 点数 = 21³ | 与 #26 互为交叉验证（维度与点数必须自洽） |
| 28 | `input has a point scalar named 'values'` | 目标字段存在 | 否则后面的按名比对会静默变成"两边都没有"，失去意义 |
| 29 | `input 'values' attribute type == IG_SCALAR` | 输入字段语义 | "输出与输入一致"的前提是先把输入钉死，否则两边同时错也会"一致" |
| 30 | `input 'values' attachment type == IG_POINT` | 点关联 | 同上 |
| 31 | `input 'values' component count == 1` | 单分量 | 同上 |
| 32 | `input 'values' array type == IG_DoubleArray (Float64)` | `Float64` 落到 `DoubleArray` | 类型映射是读取器与滤波器的接口约定，写错会直接导致类型不一致或精度损失 |
| 33 | `input 'values' has 9261 values` | 数组长度 = 点数 | 属性长度与几何必须对齐，否则滤波器内部的点数校验会误报 |
| 34 | `Execute() succeeded` | 主流程可用 | 真实数据（含压缩 appended 解码出来的值）能跑通 |
| 35 | `output name == <input name>_Median` | 命名契约 | 同 #4，真实模型路径也验一遍 |
| 36 | `output dimensions == input dimensions` | 规模不变 | 同 #5 |
| 37 | `output point count == input point count` | 点数不变 | 同 #6 |
| 38 | `output point coordinates == input coordinates` | 几何不变 | 抽取点号 0 / 13 / 441 / 中点 / 末点逐一比对**坐标值**（不是指针），所以实现若改成深拷贝也照样通过，只锁语义不锁实现 |
| 39 | `output has an attribute named 'values'` | 属性名保留 | 同 #7 |
| 40 | `output attribute count == input attribute count (all attributes kept)` | 所有属性都在 | 该模型有 2 个点标量（`values` + `vtkValidPointMask`），数量比对能抓住"只搬了被滤波的那一个"这类实现缺陷 |
| 41 | `array name identical ('values')` | 数组名 | 同 #9 |
| 42 | `attribute type identical (IG_SCALAR)` | 属性类型 | 同 #10 |
| 43 | `attachment type identical (IG_POINT)` | 关联类型 | 同 #11 |
| 44 | `component count identical (1)` | 分量数 | 同 #12 |
| 45 | `array type identical to input (GetArrayType() == IG_DoubleArray)` | **输出数组类型 = 输入数组类型** | 本次 bug 的正面回归；写成与输入比较的形式，因此换成 `FloatArray` 输入也成立 |
| 46 | `output array is a concrete class (not IG_ARRAY_OBJECT)` | 不是模板基类 | #45 的补强：即便输入本身类型异常，也要求输出是 `IG_FloatArray`/`IG_DoubleArray` 这类具体类，堵住 `FlatArray<T>::New()` 的复发路径 |
| 47 | `output has one value per input point (9261)` | 长度一致 | 同 #15 |
| 48 | `untouched attribute 'vtkValidPointMask' kept in output` | 未滤波属性被完整带上 | 覆盖"其它属性按原 `type`/`attachmentType` 加入输出"这条分支（条件断言：读到该数组才执行） |
| 49 | `untouched attribute values preserved (all ones)` | 未滤波属性数值不变 | 只查存在不够，数值也不能被改写（例如误把范围/数值重算或截断） |
| 50 | `all filtered values stay inside the input value range` | 中值必取自邻点 | 硬不变量：输出值是输入邻点之一，因此必然落在 `[inMin, inMax]` 内。只有实现"造出了输入中不存在的值"（越界补零、非法置零、类型截断）才会失败 |
| 51 | `median filtering actually changed values` | 确实做了滤波 | 排除"退化成恒等映射/什么都没做"的实现（此时 #50、#52 都可能侥幸通过） |
| 52 | `total variation decreased (field is smoother)` | 平滑性 | TV（相邻点一阶差分绝对值之和）下降是"中值滤波起作用"的量化证据，比"数值有变化"更强：能排除只改了极少数点、或把噪声搬了个位置 |
| 53 | `16 sampled points match the independent reference (3x3x3)` | 与独立参考实现逐位一致 | 采样点覆盖 5 个角点、6 个面心、2 个近角点与 3 个一般内部点（含各轴 ±1 偏移的点）。中值只做"选择"不做算术，double 原样搬运，所以精确比较是合法的；参考实现单独书写、走 `GetElementValue` 通路，任何索引/边界/中位数取法不一致都会在这些点上暴露 |
| 54 | `output can be used as input again (array type recognised by the filter)` | 输出可直接串接 | 直接复现"下游识别不了类型"的现场：旧实现第二次会以 `Unsupported scalar array type.` 失败（滤波器按 `GetArrayType()` 分派） |
| 55 | `second pass keeps IG_DoubleArray` | 二次滤波后类型仍正确 | 防止"第一次对、第二次退化"（例如结果里混入基类数组） |
| 56 | `second pass keeps the point count` | 二次滤波规模不变 | 串接后点数仍应与输入一致（`<name>_Median_Median` 也是同一个 21³ 网格） |
| 57 | `Execute() with 5x5x5 succeeded` | 大核可用 | 大核会走到更多越界分支（边界窗口收缩得更厉害），是独立于 3×3×3 的路径 |
| 58 | `5x5x5 output array type == IG_DoubleArray` | 大核路径类型仍正确 | 同 #45，防止不同核大小之间实现分叉 |
| 59 | `larger kernel is smoother (TV(5x5x5) < TV(3x3x3))` | 核大小真的影响平滑度 | 与 #52 一起构成"平滑单调性"：核越大越平滑。若参数被忽略（永远按 3×3×3 算），这条会失败 |
| 60 | `5x5x5 result differs from 3x3x3 result` | 核大小真的参与计算 | #59 的补充：直接逐值比对两组输出必须不同 |
| 61 | `16 sampled points match the independent reference (5x5x5)` | 大核下仍与参考一致 | 覆盖 `5×5×5` 的窗口边界（`kx/2=2` 的偏移），这是 3×3×3 覆盖不到的取整路径 |
| 62 | `input 'values' keeps its original 9261 values` | 多次执行后输入仍未被改写 | 前面跑了 3×3×3、串接、5×5×5 三次，这条检查累计副作用（9261 个值逐一比对） |

#### Test 4（14 项）：失败路径与消息语义

| # | check | 在验证什么 | 为什么有效 |
| --- | --- | --- | --- |
| 63 | `Execute() rejects a non-structured input` | 类型前置校验生效 | 空 `UnstructuredMesh` 连属性都没有，能挡住它说明校验发生在正确的位置（类型判断之前不依赖属性） |
| 64 | `message points at the structured-mesh requirement` | 报的是**对的原因** | 只断言"失败"太弱：把类型问题误报成属性问题同样会让用户白折腾。这里要求命中 `structured mesh` 关键词 |
| 65 | `Execute() rejects an even kernel size` | 核大小校验生效 | 偶数核会让"取上中位数"的语义含混，必须拒绝 |
| 66 | `message points at the odd-kernel requirement` | 报的是核大小原因 | 命中 `odd`，避免与属性类错误混淆 |
| 67 | `Execute() rejects an unknown attribute name` | 属性查找失败被拒 | 按名找不到时必须失败，而不是默默取第 0 个属性 |
| 68 | `message asks for a valid scalar attribute` | 报的是属性选择原因 | 命中 `valid scalar attribute` |
| 69 | `Execute() rejects a vector attribute` | `IG_VECTOR` 被拒 | 中值对多分量矢量没有定义，必须显式拒绝 |
| 70 | `message says only scalars are supported` | 报的是"只支持标量" | 命中 `only processes scalar attributes`，与 #68 的关键词刻意区分，保证走的是不同分支 |
| 71 | `Execute() rejects a 2-component scalar` | 多分量标量（`IG_SCALAR` + dim=2）被拒 | 这是最容易漏的一条：类型是标量、关联是点，只有分量数不对 |
| 72 | `message asks for a single-component scalar` | 报的是分量数原因 | 命中 `single-component scalar`，确认没有把它误判成 #69 的分支 |
| 73 | `even kernel fails and GetMessage() reports a reason` | 失败时一定有原因 | 因为 `Execute()` 开头会清空消息，万一某条失败分支忘了写原因就会暴露成空串 |
| 74 | `the same instance then succeeds and GetMessage() is cleared (no stale failure text)` | **同一实例"先失败再成功"不留残文案** | 消息是可变状态，只有复用同一实例才暴露"没清空"：换成新实例永远看不出问题。修复前这条必然失败 |
| 75 | `Execute() rejects an array shorter than the mesh point count` | 数组长度不足被拒 | 网格 4 点但数组只有 2 个值时，旧实现会越界读取 `in[2]`/`in[3]`；这条保证不越界、直接失败（评审 P1 的回归） |
| 76 | `message reports incomplete data` | 报的是"数据不完整"原因 | 命中 `incomplete`，与 #74 的成功路径、#64/#66/#70/#72 的其它失败原因刻意区分 |

#### 这套测试抓不到什么（有效性边界）

1. **两处同时写错同一约定**：参考实现与滤波器共用索引约定 `i + j·nx + k·nx·ny`（与 `StructuredMesh::GetPointIndex` 一致），所以"两边一起改错"抓不到。在**立方网格 + 立方核**下，"整轴互换"这类写法在数学上完全等价（邻域集合不变），#53/#61 无法区分；非立方情形由 Test 1 的 5×1×1 用例提供手算真值。
2. **`EXTENT` 不在断言范围内**：受框架 `SetExtent` 缺陷影响（见第七节第 10 条），输出与输入的 `EXTENT` 目前本来就不一致，因此它不能当作回归项。
3. **读取器路径只做了弱覆盖**：#48/#49 是条件断言（读到 `vtkValidPointMask` 才执行），所以"读取器读不出第二个数组"这类问题不会在这里报警。
4. **模型特定常量绑死 `3DScalar.vti`**：21³ / 9261 / `IG_DoubleArray` / `changed > 0` / TV 单调性，换模型需同步调整（6.4 已列明）。

### 6.6 界面手动验证

不跑示例时，也可以在界面上用同一模型快速核对：

1. 加载 `3DScalar.vti`，菜单 **算法处理 → 中值滤波 (Median)**；
2. `标量数组` 选 `values`（标签显示 `类型: double | 范围: [...]`）；
3. 核大小保持 `3 × 3 × 3`，点 `执行`：结果模型（`<输入名>_Median`，形如 `3DScalar_Median`）加入模型树并按滤波后标量着色；
4. 在模型信息面板确认 `values` 一行仍是 `double`（**不是"未知类型"/"unknown"**），`vtkValidPointMask` 一行原样保留；
5. 再把核大小改成 `5 × 5 × 5` 重新执行，颜色分布应更平滑。

---

## 七、注意事项

1. **仅支持结构化网格**：输入类型不是 `IG_STRUCTURED_MESH` 时 `Execute()` 返回 `false`（提示 `Median filter only supports a structured mesh (StructuredMesh).`）。非结构网格请先转结构化，或改写为对应的非结构实现。
2. **只处理点标量、且必须单分量**：矢量、法向量、单元属性、多分量标量都会被拒绝；多分量请先按分量提取（见 [ExtractComponent 使用说明](ExtractComponent使用说明.md)）再滤波。
3. **核大小必须是奇数且 ≥ 1**：否则直接失败（提示 `Kernel size must be an odd positive number in every dimension.`）。界面用步长 2 的 SpinBox 从交互上避免偶数；2D 数据（第三维 ≤ 1）的 `kz` 会被强制为 1，界面上 Z 控件同时被禁用。
4. **边界不补零**：越界邻点被跳过，边界点窗口收缩；若业务上必须"补零/镜像边界"，需在算法层另行扩展，当前实现不做。
5. **上中位数**：候选个数为偶数（只可能出现在边界处）时取偏大的那个，边界结果不会出现"两值平均"。
6. **与原模型共享内存**：输出与输入**共享点坐标对象**，且未滤波的属性数组也是**同一份指针**（浅共享）——后续若对输出做会修改坐标或这些属性值的操作，原模型会同步变化。需要完全独立的数据请自行深拷贝后再滤波。
7. **不修改输入**：滤波器只新建输出节点，输入对象的属性和几何都不会被改写（共享仅指内存复用，非就地修改）。
8. **重复执行不会新增模型**（界面）：面板始终基于原模型重算并更新同一个 `_Median` 结果模型；删除结果模型后再执行会重新创建。
9. **可为结果继续串接滤波器**：输出的属性字段与原模型同构（同名、同属性类型、同数组类型），因此可作为阈值、等值面等后续滤波器的输入。
10. **`EXTENT` 目前不可靠（框架既有缺陷，非本滤波器引入）**：`StructuredMesh::SetExtent`（`iGameCore/Core/DataModel/iGameStructuredMesh.h:31`）写成 `std::copy(e, e + 6, this->size)`，把 6 个值写进了只有 3 个元素的 `size`（`GetExtent()` 读的是另一个成员 `extent`），因此本滤波器里"把输入 extent 拷给输出"这一步实际只写进了 `size`（随后被 `SetDimensionSize` 覆盖），输出 `GetExtent()` 与输入并不一致。判断网格规模请用 `GetDimensionSize()` / `GetNumberOfPoints()`；算法本身不依赖 `extent`，故不影响滤波结果。示例程序因此**不对 `EXTENT` 做断言**。

### 失败提示一览（`GetMessage()` 返回值）

> `Execute()` 开头会清空该字段，下表每一条都对应一次 `Execute()` 返回 `false`；返回 `true` 时该字段为空字符串。
> 文案全部为**英文**：`std::string` 不带编码信息，中文在"源码编码 ≠ 控制台/调用方代码页"时会变乱码（典型现场是核心层用 GBK 写死、界面用 `QString::fromStdString` 按 UTF-8 解），英文可跨端安全显示。

| 提示 | 触发条件 |
| --- | --- |
| `Input data object is null.` | `SetInput` 未设置或为空指针 |
| `Median filter only supports a structured mesh (StructuredMesh).` | 输入 `GetDataObjectType()` 不是 `IG_STRUCTURED_MESH` |
| `Failed to cast the input to StructuredMesh.` | 类型枚举相符但 `DynamicCast` 失败（极少见） |
| `The input model has no attribute.` | 输入的属性集为空 |
| `Please select a valid scalar attribute.` | 下标越界，且按名称也找不到 |
| `The selected attribute is no longer valid, please select it again.` | 该属性槽位已删除或数组指针为空 |
| `Median filter only processes scalar attributes.` | 选中的是矢量 / 法向量 / 张量等 |
| `Median filter only processes point scalar attributes.` | 选中的是单元属性 |
| `Please extract or select a single-component scalar before running the median filter.` | 选中标量的分量数不为 1 |
| `Kernel size must be an odd positive number in every dimension.` | 核大小含偶数或非正值 |
| `Unsupported scalar array type.` | 数组类型不在 4.2 节支持的 10 种之内 |
| `The selected scalar array has fewer values than the mesh points; incomplete data is rejected.` | 所选标量的值个数少于网格点数（数据被截断 / 不完整） |
| `Median computation failed.` | 点数与 `DIMENSIONS` 推算值不符，或类型转换失败 |
| `The output array type does not match the input array type.` | 内部不变量校验：输出数组类型枚举与输入不同（正常流程不会出现，属回归保护） |
