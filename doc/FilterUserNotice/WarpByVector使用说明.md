# WarpByVector 滤波器使用说明

> - 头文件:`iGameCore/Filters/WarpByVector/iGameWarpByVectorFilter.h`
> - 实现文件:`iGameCore/Filters/WarpByVector/iGameWarpByVectorFilter.cpp`
> - 自动测试示例:`Examples/Filter/TestWarpByVector.cpp`
> - 配套测试模型:`Examples/Models/Streamline_tornado_structuredmesh.vtk`
> - 界面入口:`Filters` 菜单 → **向量形变 (WarpByVector)**(与"阈值""生成ID"同级)

---

## 一、功能

按给定的**向量场**对几何做形变(Warp by vector),逐点计算:

```
新坐标 = 原坐标 + 缩放系数 × 向量(该点)
```

- **输出为独立数据对象**:输入模型保持不变(与 GenerateIds 的输出约定一致);
- **按输入类型分派输出**,结构化网格保持结构化(不转非结构化,避免大网格内存爆炸):
  | 输入类型 | 输出类型 | 说明 |
  | --- | --- | --- |
  | 结构化网格 `IG_STRUCTURED_MESH` | 同类型结构化网格 | 仅拷贝点集、维度信息与属性,形变只作用于点坐标 |
  | 非结构化网格 / 面网格 / 体网格 | `UnstructuredMesh` | 拷贝点、单元、单元类型与属性 |
- **缩放系数**可手动指定,也可**自动缩放**(按模型尺度归一化,使最大位移≈模型对角线);
- **单点位移上限**可配置(相对模型对角线的比例),避免个别极端向量把网格拉变形;
- 向量场从输入的 `AttributeSet` 中获取(优先点关联数组),取属性方式与 Deformation 一致;
- 属性集合按**实际数组类型**逐元素拷贝(`LongLongArray` 等大整数属性不会退化为 double)。

**典型用途**:风场/流场形变可视化(把速度场直接作用到网格)、位移场(Mode Shape)展示、包装结构变形预览、向量场强度直观对比等。

---

## 二、调用方式

### 2.1 主要接口

| 接口 | 说明 |
| --- | --- |
| `static Pointer New()` | 创建滤波器实例 |
| `SetInput(DataObject::Pointer)` | 设置输入数据(基类接口) |
| `bool SetVectorArrayName(const std::string&)` | 形变向量数组名,默认 `"Velocity"`;置空则自动取第一个向量数组 |
| `void SetScaleFactor(double)` | 缩放系数,默认 `1.0`;开启自动缩放时被忽略 |
| `void SetAutoScale(bool)` | 是否自动缩放:按 `模型对角线 / 最大向量模长` 归一化,使最大位移≈模型尺度 |
| `void SetMaxDisplacementRatio(double)` | 单点位移上限(相对模型对角线的比例),`0` 表示不限制 |
| `bool Execute()` | 执行形变,失败返回 `false` |
| `DataObject::Pointer GetOutput()` | 获取输出(基类接口) |
| `UnstructuredMesh::Pointer GetWarpMesh()` | 输出为非结构化网格时直接取用(结构化输入时返回空) |
| `double GetAppliedScale() const` | 回读最近一次实际使用的缩放系数(自动缩放时由内部算出) |
| `GetVectorArrayName()` / `GetScaleFactor()` / `GetAutoScale()` / `GetMaxDisplacementRatio()` | 读取当前配置 |

### 2.2 典型调用步骤

1. `WarpByVectorFilter::New()` 创建滤波器
2. `SetInput(dataObject)` 设置输入
3. `SetVectorArrayName("velocity")` 指定向量数组(名字必须存在,否则执行失败)
4. `SetScaleFactor(...)` 或 `SetAutoScale(true)` 控制形变幅度
5. (可选)`SetMaxDisplacementRatio(...)` 限制单点位移
6. `Execute()` 执行,返回 `true` 表示成功
7. `GetOutput()`(或 `GetWarpMesh()`)获取独立输出并加入场景/模型树

---

## 三、使用示例

### 3.1 基本用法(固定缩放系数)

```cpp
#include "WarpByVector/iGameWarpByVectorFilter.h"
#include "iGameFileIO.h"

auto input = iGame::FileIO::ReadFile("./Models/Streamline_tornado_structuredmesh.vtk");

auto warp = iGame::WarpByVectorFilter::New();
warp->SetInput(input);
warp->SetVectorArrayName("velocity");   // 点关联向量数组名
warp->SetScaleFactor(0.5);              // 新坐标 = 原坐标 + 0.5 × 向量
if (!warp->Execute()) { /* 处理失败:名字不存在 / 非点关联 / 向量长度不足 */ }

auto warped = warp->GetOutput();        // 独立对象,原模型不变
```

### 3.2 自动缩放(免去手动试系数,推荐)

```cpp
auto warp = iGame::WarpByVectorFilter::New();
warp->SetInput(input);
warp->SetVectorArrayName("velocity");
warp->SetAutoScale(true);               // 按模型尺度归一化
warp->Execute();

std::cout << "实际缩放系数 = " << warp->GetAppliedScale() << "\n";
```

### 3.3 限制单点位移

```cpp
warp->SetScaleFactor(1.0);
warp->SetMaxDisplacementRatio(0.2);     // 任何点的位移不超过模型对角线的 20%
warp->Execute();
```

### 3.4 与其它滤波器配合(先打 Id,再形变,再筛选)

```cpp
// 1) 先给点/单元打 Id(便于追溯形变前后的对应关系)
auto ids = iGame::iGameGenerateIdsFilter::New(IG_POINT);
ids->SetInput(input);
ids->SetArrayName("PointIds");
ids->Execute();

// 2) 形变(Id 数组会随属性集合一起拷贝到输出)
auto warp = iGame::WarpByVectorFilter::New();
warp->SetInput(ids->GetOutput());
warp->SetVectorArrayName("velocity");
warp->SetScaleFactor(0.5);
warp->Execute();

// 3) 形变后再做阈值筛选
auto threshold = iGame::ThresholdFilter::New();
threshold->SetInput(warp->GetOutput());
threshold->SetScalarData(/* 某个标量数组 */ scalar, iGame::ThresholdFilter::Association::Point, 0);
threshold->SetThreshold(lower, upper);
threshold->Execute();
```

### 3.5 界面用法(与 ParaView 的 WarpByVector 对应)

菜单 `Filters` → **向量形变 (WarpByVector)**,对话框参数:

| 界面项 | 对应 ParaView | 说明 |
| --- | --- | --- |
| **向量 (Vectors)** | Vectors | 下拉列出该模型所有**点关联的三维向量**(显示为 `名称 (dim=N)`);无可用向量时直接提示并中止 |
| **缩放系数 (Scale Factor)** | Scale Factor | 数值输入,默认 `1`;不确定填多少就勾选下一项 |
| **自动缩放(按模型尺度)** | —(ParaView 需自行试数) | 勾选后忽略手填值,按模型尺度归一化 |
| **最大位移比例 (0 = 不限)** | — | 限制单点位移上限,防止个别极端向量拉坏网格 |

点 `Apply` 后,结果作为**新节点** `<模型名>_warp` 加入模型树并自动成为当前模型;原模型节点保持不变。

### 3.6 运行内置自动测试

```powershell
# 构建 Examples 后(需 EXAMPLE_COMPILE=ON),直接运行,无需任何手动输入
cmake --build build --target testWarpByVector --config Debug
cd build/Examples/Debug
./testWarpByVector.exe
```

测试使用仓库内模型 `./Models/Streamline_tornado_structuredmesh.vtk`,依次校验:输出独立性、点数/单元数不变、属性集合完整拷贝、位移严格等于 `scale × 向量`、缩放线性关系、自动缩放、位移上限、错误处理(向量名不存在应失败),全部通过时输出 `[PASS]`。

---

## 四、注意事项

1. **向量必须是点关联数据**:单元关联的向量数组会直接判定失败并打印提示(逐点形变要求每个点有独立向量)。
2. **数组名必须存在**:显式设置的 `SetVectorArrayName` 若在 `AttributeSet` 中找不到,`Execute()` 返回 `false`,**不会静默退回"任意向量数组"**(避免"改了参数却没生效"的误判);只有名字留空时才自动挑选第一个向量数组。
3. **向量元素数必须 ≥ 点数**:不足时执行失败;向量维度小于 3 时只取前若干分量(缺省分量为 0)。
4. **结构化网格保持原类型**:结构化网格不会转成非结构化(否则 128³ 这类网格会产生上千万单元),输出仍是结构化网格;此时 `GetWarpMesh()` 返回空,请使用 `GetOutput()`。
5. **输出是独立对象,输入不被修改**:形变只作用于输出对象的点集;若需要保留形变前的原始数据,直接持有输入对象即可。
6. **包围盒无需手动更新**:`DataObject::GetBoundingBox()` 内部会调用 `ComputeBoundingBox()`,形变后自动反映新范围。
7. **法线无需手动重算**:面法线由 `Face::GetNormal()` 按当前点坐标按需计算,顶点法线在重建可绘制数据时生成;滤波器内部已在形变后调用 `ForceReConvertToDrawableData()` 标记刷新。
8. **属性按实际类型拷贝**:框架的 `AttributeSet::Attribute::DeepCopy()` 只支持 float/double 数组,会把 `LongLongArray` 等属性静默丢成空指针,因此本滤波器自行按数组类型分发拷贝。
9. **缩放系数怎么选**:量级不确定时优先用"自动缩放";手动填时注意向量本身的量级(例如速度场可能上万,直接填 1 会把网格拉出屏幕,建议先试 `0.001` 量级)。
10. **浮点精度**:点坐标通常以 `float` 存储,限幅(位移上限)后存在浮点舍入,校验时请给出小容差(测试中取 `1e-4`)。
11. **大网格开销**:仅点集深拷贝,开销与点数线性相关(如 209 万点约 25MB),不复制单元拓扑的额外结构。

---

## 五、配套测试模型

| 模型 | 规模 | 字段 | 用途 |
| --- | --- | --- | --- |
| `Examples/Models/Streamline_tornado_structuredmesh.vtk` | 312 点 / 288 四边形单元(24 × 12 圆柱漏斗) | `velocity`(向量:切向旋转 + 径向内聚 + 上升)、`temperature`(标量) | 向量形变的自动测试与可视化演示 |

> 模型半径按 `r = 0.15 + 1.35·(z/H)³` 由下向上张开,速度场为典型龙卷风形态,形变效果直观。
> 运行时该模型由 `iGameCopyExampleAssets` 自动拷贝到 Examples 构建目录下的 `Models/`,示例代码直接写死相对路径 `./Models/Streamline_tornado_structuredmesh.vtk`,**无需手动输入**。
