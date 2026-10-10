# RenameArrays 使用说明

## 功能

`iGame::RenameArrays` 对标 ParaView 的 **Rename Arrays**（VTK 侧是 `vtkArrayRename`）：把点 / 单元数组改名。

与 ParaView 一致的核心语义：

- **输出是一个全新的网格**：与输入同类型，几何、拓扑、属性全部独立（由深拷贝产生），**输入对象完全不被修改**；
- 只改数组的**名字**，不碰几何与数据值；
- 支持按「原名 → 新名」或按「（关联, 下标）」两种方式配置；
- 新名为空 → 拒绝（与 `vtkArrayRename` 一致）；
- 新名与**同组**已有数组重名 → 拒绝该条改名，并把冲突写入 `GetMessage()`（`vtkArrayRename` 是警告后覆盖；这里选择不产生重名数组，避免按名字查数组时出现歧义）；
- 未配置的数组保持原名与原值。

处理范围：只有 `IG_POINT`（点数据）与 `IG_CELL`（单元数据）两种关联。框架的 `AttributeSet` 只支持这两种附着，**没有 vertex 关联**；字段数据（`DataObject::Metadata`）不属于 `AttributeSet`，本 Filter **不涉及**，因此**输出不携带字段数据**。

## 主要接口

头文件：

```cpp
#include <RenameArrays/iGameRenameArrays.h>
```

| 接口 | 作用 | 默认值 |
| --- | --- | --- |
| `SetInput(input)` | 设置输入 `DataObject` | 无 |
| `SetArrayName(attachmentType, inputName, newName)` | 按原名配置改名；`attachmentType` 取 `IG_POINT` / `IG_CELL` | 无 |
| `SetArrayName(attachmentType, index, newName)` | 按输入数组下标配置；下标在 `Execute()` 时按输入解析成原名 | 无 |
| `SetPointArrayName(old, new)` | 点数组改名的便捷写法（等价于 `SetArrayName(IG_POINT, ...)`） | 无 |
| `SetCellArrayName(old, new)` | 单元数组改名的便捷写法 | 无 |
| `ClearAll()` | 清空全部映射 | — |
| `ClearMapping(attachmentType)` | 清空某一关联分组的映射 | — |
| `GetNumberOfMappings()` | 已配置映射条数（含按下标待解析的） | `0` |
| `GetNumberOfArrays(attachmentType)` | 输入中该关联分组的数组个数 | — |
| `GetArrayOriginalName(attachmentType, idx)` | 输入中第 `idx` 个数组的原名；越界返回空串 | — |
| `GetArrayNewName(attachmentType, idx)` | 该数组已配置的新名；未配置返回空串 | — |
| `Execute()` | 执行改名；输出为全新网格 | — |
| `GetOutput(0)` | 输出：与输入同类型的新网格（输入不被修改） | — |
| `GetMessage()` | 最近一次执行的消息（含重名冲突列表） | `未执行` |

## 调用方式

1. 创建 Filter：`RenameArrays::New()`；
2. 设置输入对象；
3. 配置改名：按名字（`SetArrayName(type, old, new)` / `SetPointArrayName` / `SetCellArrayName`）或按（关联, 下标）；
4. 调用 `Execute()`；
5. 检查返回值：失败时读 `GetMessage()`，成功时用 `GetOutput(0)` 取新网格（可再 `GetMessage()` 查看是否出现重名冲突被跳过）；
6. 按需给新网格改名（Filter 会沿用输入的名字，界面会自动命名为 `<输入名>_rename`）。

按下标配置时，可以先用 `GetNumberOfArrays(type)` 与 `GetArrayOriginalName(type, idx)` 枚举输入里的数组；这两者与面板左列的顺序一致。

## 界面操作

菜单 **滤波器 → 重命名数组(RenameArrays)**，面板在左侧工具 Tab「重命名数组」中打开：

1. 面板顶部显示当前输入模型名（取模型树中当前选中的模型；每次打开面板都会刷新）；
2. **点数组 (Point)** 与 **单元数组 (Cell)** 两个分组：
   - 左列只读，列出输入中该组的原数组名；
   - 右列可编辑，双击修改新名，默认等于原名；留空或保持原名表示该数组不改名；
   - 左右两列滚动同步，一一对应；
3. 点「执行」，结果显示在面板底部状态区：`RenameArrays: renamed=N`，出现重名冲突时追加 `skipped(conflict)=1 [旧名 -> 新名]`；
4. 结果以新节点加入模型树，命名为 `<输入名>_rename`（重名时自动加序号），随后面板自动切到新结果，左列显示改名后的数组。

## 使用示例

下面的示例使用仓库内固定测试模型 `Models/RenameResample_test.vtk`（点数组 `value`、`alpha`，单元数组 `cid`）：

```cpp
#include <RenameArrays/iGameRenameArrays.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>

#include <iostream>

int main() {
    using namespace iGame;

    auto input = FileIO::ReadFile("Models/RenameResample_test.vtk");
    if (!input) {
        std::cerr << "读取测试模型失败。\n";
        return 1;
    }

    auto filter = RenameArrays::New();
    filter->SetInput(input);

    // 按名字配置
    filter->SetPointArrayName("value", "pressure");
    filter->SetCellArrayName("cid", "cell_id");

    // 也支持按（关联, 下标）配置：这里把第 1 个点数组（alpha）改名为 temperature
    filter->SetArrayName(IG_POINT, 1, "temperature");

    if (!filter->Execute()) {
        std::cerr << filter->GetMessage() << '\n';
        return 1;
    }

    auto output = filter->GetOutput(0);
    std::cout << filter->GetMessage() << '\n';

    // 输入没有被修改：仍然是 value / alpha / cid
    auto* inputAttributes = input->GetAttributeSet();
    std::cout << "input  value index: " << inputAttributes->GetAttributeIndex("value") << '\n';

    // 输出上是新名字：GetAttribute(name) 返回 Attribute&，数组本体在 Attribute::pointer 上，
    // 这里按下标取（GetAttributeIndex 未命中返回 -1）。
    auto* outputAttributes = output->GetAttributeSet();
    const int pressureIndex = outputAttributes->GetAttributeIndex("pressure");
    const int temperatureIndex = outputAttributes->GetAttributeIndex("temperature");
    if (pressureIndex >= 0 && temperatureIndex >= 0) {
        std::cout << "output pressure[0] = "
                  << outputAttributes->GetAttribute(pressureIndex).pointer->GetElementValue(0, 0) << '\n';
        std::cout << "output temperature[0] = "
                  << outputAttributes->GetAttribute(temperatureIndex).pointer->GetElementValue(0, 0) << '\n';
    }
    return 0;
}
```

完整自检示例位于：

```text
Examples/Filter/TestRenameArrays.cpp
```

该示例覆盖：点 / 单元改名生效（新名可查、旧名消失）、数据值逐元素不变、**输入未被修改**、输出的数组与输入**不是同一对象**、按下标配置（且允许先配置后 `SetInput`）、空新名与重名被拒绝并在消息中报告、未配置的数组保持原名原值。示例与 `TestResampleWithDataSet` 共用测试网格 `Examples/Models/RenameResample_test.vtk`。

## 注意事项

1. 本 Filter **不会**修改输入：原模型仍是旧数组名；结果是模型树里的一个**新节点**。需要原地改名时请自行把输出替换回场景。
2. 输出数组与输入数组**不是同一个对象**（深拷贝），改名结果不会互相影响；代价是几何与数组数据都被复制一份（`FlatArray::ShallowCopy` 在本框架中不可用，无法做共享缓冲）。
3. 重命名会影响所有**按名字**查数组的下游：着色 / 过滤参数、脚本、导出文件名、二次开发代码等，改名后请同步更新。
4. 同组重名会被**拒绝**（不是覆盖），这一点与 `vtkArrayRename` 的「警告后覆盖」不同，目的是避免 `GetAttribute(name)` 出现歧义；冲突会出现在 `GetMessage()` 中。
5. 只处理 `IG_POINT` / `IG_CELL`；没有 vertex 关联，字段数据（`Metadata`）不处理，输出不带字段数据。
6. 不支持复合 / 多块数据集：`Execute()` 返回 `false` 并给出消息（`vtkArrayRename` 由 executive 逐块展开）。
7. 按（关联, 下标）配置时，下标顺序等于 `AttributeSet` 的内部顺序，也就是面板左列的顺序；下标在 `Execute()` 时解析，因此「先配置、后 `SetInput`」同样有效。
8. 直接调用 Filter 时输出会沿用输入的名字（`DeepCopyDataObject` 拷贝名字），建议自行 `SetName()`；通过面板执行时会自动命名为 `<输入名>_rename`。
9. 数组的**数据范围**（着色用）会随数组一起深拷贝，改名不会改变取值范围；着色状态按属性下标保存，改名后仍然可用。
