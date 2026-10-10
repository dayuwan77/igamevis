# ResampleWithDataSet 使用说明

## 功能

`iGame::ResampleWithDataSet` 对标 ParaView 的 **Resample With Dataset**（VTK 侧是 `vtkProbeFilter`）：把一个网格上的属性场采样到另一个网格的**所有点**上，输出是「采样点网格」的同类型全新网格。

两个输入（语义与 `vtkProbeFilter` 一致，**注意 VTK 的命名与直觉相反**）：

- 输入 0「**被采样网格**」= VTK 的 **Source**：提供数据的网格。它的点数据被插值到采样点上，它的单元数据按「采样点命中哪个单元」拷到采样点上。
- 输入 1「**采样点网格**」= VTK 的 **Input**：探针几何。它的所有点就是采样位置，它的几何与拓扑会被原样深拷贝成输出。

输出（`GetOutput(0)`）包含：

- 几何与拓扑：输入 1 的同类型深拷贝，类型与输入 1 保持一致（`PointSet` / `SurfaceMesh` / `UnstructuredMesh` / `VolumeMesh` / `StructuredMesh`）；
- 属性：
  1. 输入 0 的**点数据**插值到输出点上；
  2. 输入 0 的**单元数据**按命中单元拷到输出点上（与 `vtkProbeFilter` 一致；若同名数组同时存在于输入 0 的点数据与单元数据，只取点数据）；
  3. `validpointmask`（`UnsignedCharArray`，`1` = 该采样点插值成功，`0` = 无效；无效点的各插值属性填 `0`）；
  4. 输入 1 自身的属性数组一并保留（对应 VTK 的 `PassPointArrays` / `PassCellArrays`）；若输入 1 的**点**属性与上述 1~3 产出的数组同名，以采样结果为准。

采样位置、容差与吸附：

- 采样位置就是「采样点网格」的所有点（不沿线均匀生成）；
- 容差默认自动：被采样网格包围盒对角线 × `1e-6`；`SetTolerance(t)` 可手动指定，**t 是相对包围盒对角线的比例**，`t <= 0` 恢复自动；
- 吸附半径（对应 VTK 的 `SnappingRadius`）：`SetSnappingRadius(r)`（`r` 同样相对包围盒对角线）打开后，落在所有单元之外、但在半径内存在单元的采样点会被**吸附**到最近单元边界的最近点取值，并记为有效；本实现**不移动采样点坐标**。

单元类型与插值内核（沿用 ResampleToLine 的内核）：

- 线性面单元：重心坐标 / 双线性 / 扇形三角化；
- 体单元：均值坐标（Mean Value Coordinates）；
- 二次 / 高次单元：二次形函数 + 参数坐标 Newton 反解；
- 未覆盖的高次单元：退化为角点线性处理，并在 `GetMessage()` 中提示数量。

## 主要接口

头文件：

```cpp
#include <ResampleWithDataset/iGameResampleWithDataSet.h>
```

| 接口 | 作用 | 默认值 |
| --- | --- | --- |
| `SetSourceData(data)` | 设置「被采样网格」（输入 0，数据来源，对应 VTK 的 Source） | 无 |
| `SetProbeData(data)` | 设置「采样点网格」（输入 1，探针几何，对应 VTK 的 Input） | 无 |
| `GetSourceData()` / `GetProbeData()` | 读取两个输入 | — |
| `SetAutoTolerance(bool)` | 是否使用自动容差 | `true` |
| `SetTolerance(t)` | 手动容差，**相对**被采样网格包围盒对角线；`t <= 0` 恢复自动 | 自动 |
| `GetTolerance()` | 手动容差；自动容差时返回 `-1` | — |
| `GetEffectiveTolerance()` | 上一次 `Execute()` 实际使用的容差（绝对长度） | — |
| `SetSnappingRadius(r)` | 吸附半径，**相对**包围盒对角线；`r <= 0` 关闭 | 关闭（`-1`） |
| `GetSnappingRadius()` / `GetEffectiveSnappingRadius()` | 相对半径 / 实际绝对半径（`< 0` 表示未启用） | — |
| `Execute()` | 执行采样，返回是否成功 | — |
| `GetOutput(0)` / `GetResampledData()` | 输出：采样点网格的同类型深拷贝 + 采样属性 | — |
| `GetSampleCellIds()` | 每个采样点命中的单元 id，`-1` = 无效 | — |
| `GetSampleValidMask()` | 每个采样点是否有效（与 `validpointmask` 属性一致） | — |
| `GetValidPoints()` | 有效采样点 id 列表（等价 `vtkProbeFilter::GetValidPoints()`） | — |
| `SetValidPointMaskArrayName(name)` | 有效点掩膜数组名；传空串恢复默认；设为 `vtkValidPointMask` 与 VTK 默认命名一致 | `validpointmask` |
| `GetValidPointMaskArrayName()` | 读取当前掩膜数组名 | `validpointmask` |
| `GetSnappedPointCount()` | 本次依靠吸附才命中的采样点数量 | `0` |
| `GetUnsupportedQuadraticCellCount()` | 本次退化为线性角点处理的二次 / 高次单元数量 | `0` |
| `GetMessage()` | 最近一次执行的消息（含采样数、有效数、容差、退化提示） | `未执行` |

## 调用方式

1. 创建 Filter：`ResampleWithDataSet::New()`；
2. 设置输入 0（被采样网格，提供数据）与输入 1（采样点网格，提供采样位置）；
3. 按需设置容差（默认自动即可）与吸附半径（默认关闭）；
4. 调用 `Execute()`；
5. 检查返回值；失败时读 `GetMessage()`，成功时用 `GetOutput(0)` 取结果；
6. 按需读取 `GetSampleValidMask()` / `GetSampleCellIds()` 做统计过滤。

## 界面操作

菜单 **滤波器 → 重采样至数据集(ResampleWithDataSet)**，面板在左侧工具 Tab「重采样至数据集」中打开：

1. **输入** 分组中选择两个模型（下拉框都只列出模型树里已有的模型）：
   - 第一个下拉框「被采样网格」——提供数据的网格；
   - 第二个下拉框「采样点网格」——提供采样位置、并决定输出类型；
   - 「刷新模型列表」重新同步模型树；
2. **参数** 分组的「指定容差」留空或填 `0` 表示自动容差；填正数表示相对被采样网格包围盒对角线的比例；
3. 点「执行」，结果显示在面板底部（采样点数 / 有效数 / 实际容差；有退化单元或冲突时一并提示）；
4. 结果以新节点加入模型树，命名为 `<采样点网格名>_resample`（重名时自动加序号）。

删除被选中的输入模型不会关闭面板，只清空对应的下拉框选择。

## 使用示例

下面的示例使用仓库内固定测试模型 `Models/RenameResample_test.vtk`（单位六面体，点数组 `value` = 0..7）：

```cpp
#include <ResampleWithDataset/iGameResampleWithDataSet.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>

#include <iostream>

int main() {
    using namespace iGame;

    // 被采样网格：提供 value 场
    auto source = FileIO::ReadFile("Models/RenameResample_test.vtk");
    if (!source) {
        std::cerr << "读取被采样网格失败。\n";
        return 1;
    }

    // 采样点网格：这里直接复用同一个网格做自采样（体心插值 = 角点均值 3.5）
    auto filter = ResampleWithDataSet::New();
    filter->SetSourceData(source);
    filter->SetProbeData(source);

    if (!filter->Execute()) {
        std::cerr << filter->GetMessage() << '\n';
        return 1;
    }

    auto output = filter->GetOutput(0);
    const auto& mask = filter->GetSampleValidMask();

    int valid = 0;
    for (unsigned char m : mask) { valid += (m != 0) ? 1 : 0; }

    std::cout << "samples = " << mask.size() << ", valid = " << valid << '\n';
    std::cout << filter->GetMessage() << '\n';

    auto* attributes = output->GetAttributeSet();
    const int valueIndex = attributes->GetAttributeIndex("value");
    if (valueIndex >= 0) {
        // GetAttribute(name) 返回的是 Attribute&（未命中返回占位 NONE），这里用下标形式取值；
        // 数组本体在 Attribute::pointer 上。
        auto& attribute = attributes->GetAttribute(valueIndex);
        std::cout << "value[0] = " << attribute.pointer->GetElementValue(0, 0) << '\n';
    }
    return 0;
}
```

完整自检示例位于：

```text
Examples/Filter/TestResampleWithDataSet.cpp
```

该示例覆盖：自采样（探针 = 被采样网格本身，有效点插值应等于原值）、点云探针（体心 = 3.5、网格外点无效且填 0）、吸附半径开关、`validpointmask` 与 API 一致性、采样点网格自身数组的保留规则，以及缺少输入时的错误路径。

## 与 ParaView / VTK 的差异

1. 采样点与几何坐标是单精度（框架 `typedef Vector3f Point`），VTK 为双精度；大坐标或高精度场会有末位差异；
2. 体单元使用均值坐标，VTK 使用等参形函数；线性场一致，非线性场在单元内部会有差异；
3. 无效点同样填 `0`；
4. 不支持复合 / 多块数据集（VTK 的 executive 会逐块展开）；
5. 没有 field 数据（框架的 `AttributeSet` 只有 `IG_POINT` / `IG_CELL` 两种附着），也没有 `vtkOriginalPointIds` / `vtkOriginalCellIds` 这类附加数组；
6. 整型 / 字符型点数据数组按**原类型**承载插值结果（由数组自身截断），不再统一转成 `float`，与 `vtkProbeFilter` 的数组类型行为一致；
7. 单元包含判定的参数域容差与 VTK 一致（`pcoords ∈ [-0.001, 1.001]`），贴单元边界的采样点与 VTK 得到相同的有效 / 无效判定。

## 注意事项

1. 两个输入的语义不要弄反：**提供数据的是「被采样网格」（输入 0）**，输出几何来自「采样点网格」（输入 1）。
2. 输出几何是「采样点网格」的**深拷贝**，不是共享；输入 1 后续被修改不会影响已生成的结果。
3. `GetSampleValidMask()` 与输出属性（默认名 `validpointmask`，可用 `SetValidPointMaskArrayName()` 改为 `vtkValidPointMask`）完全一致，`GetValidPoints()` 给出有效采样点 id；`GetSampleCellIds()` 给出每个采样点命中的单元 id（`-1` = 无效）。无效点的插值属性为 `0`，下游统计务必先用 mask 过滤。
4. 手动容差是**相对值**（乘包围盒对角线），要拿实际使用的绝对长度请用 `GetEffectiveTolerance()`；吸附半径同理（`GetEffectiveSnappingRadius()`）。
5. 吸附半径默认关闭，且**没有在面板中暴露**，只提供 API；开启后不会移动采样点坐标。
6. 「被采样网格」若不是 `UnstructuredMesh`（例如 `SurfaceMesh` / `VolumeMesh` / `StructuredMesh`），会先转换成 `UnstructuredMesh` 再参与定位与插值；转换会复制一份数据。
7. 未覆盖的高次单元会退化为角点线性处理：非线性的二次场在这些单元上不再精确，注意 `GetUnsupportedQuadraticCellCount()` 与消息提示。
8. 采样点网格自身的数组会保留；与采样结果同名时以采样结果为准。若需要保留原名，请先用 RenameArrays 改名。
9. `Execute()` 失败时 `GetOutput(0)` 为 `nullptr`，失败原因从 `GetMessage()` 获取（例如「未选择「被采样网格」」「「采样点网格」没有点，无法确定采样位置」）。
10. 点的定位走均匀网格加速（按被采样网格包围盒划分），一次执行的采样点数等于「采样点网格」的点数；点数很多时建议先把被采样网格裁剪到关心的区域再采样。
