# YieldCriteria 屈服准则过滤器使用说明

## 功能

`YieldCriteriaFilter` 在**输入数据集的张量属性上计算屈服准则**，对每个点（或单元）的对称应力张量求主应力，再按所选准则输出结果数组。输出为输入网格的深拷贝，几何与拓扑不变，原模型不受影响。

- **输入**：任意网格 / 点集（PointSet 派生），且带一个 **6 分量**或 **9 分量**的数组（点数据或单元数据均可）
- 张量数组按**分量数**识别：优先取类型被标记为张量的属性，其次接受任意 6 / 9 分量的属性
  （部分读取器不给出张量类型标记，例如 VTU 读取器只区分 3 分量与其它）
- **输出**：`<输入名>_YieldCriteria` 的新模型，追加结果数组，原有属性全部保留
- **确定性**：结果只取决于张量与所选准则

张量分量约定：

- **6 分量对称张量**：`xx, yy, zz, xy, yz, zx`
- **9 分量完整张量**：行主序 3×3（`xx xy xz / yx yy yz / zx zy zz`）

## 参数

| 参数 | 可选值 | 说明 |
|------|--------|------|
| 点张量 (Point Tensors) | 当前模型里所有 6 / 9 分量的**点**属性 | **多选列表**，可一次选多个（Ctrl / Shift 加选） |
| 单元张量 (Cell Tensors) | 当前模型里所有 6 / 9 分量的**单元**属性 | **多选列表**，同上 |
| 屈服准则 | 主应力 / Tresca 准则 / von Mises 准则 | 单选 |

一次执行会对选中的**每个**张量都计算一遍，结果数组分别以各自的张量名为前缀追加到输出上；例如同时选中
`Stress` 与 `Stress2` 时，输出会同时包含 `Stress - Von Mises Criterion` 与 `Stress2 - Von Mises Criterion`。
两个列表可以混合选择（点数据与单元数据各算各的）；同名重复选择会自动去重；只选一个时行为与单选完全一致。

> 只有点张量或只有单元张量时，面板只显示对应的那一个列表。

## 输出数组

| 准则 | 输出数组 | 分量 |
|------|----------|------|
| 主应力 (Principal Stress) | `<张量名> - Sigma 1`、`- Sigma 2`、`- Sigma 3` | 1（主应力，降序 σ1 ≥ σ2 ≥ σ3） |
| 主应力 (Principal Stress) | `<张量名> - Sigma 1 (Vector)`、`- Sigma 2 (Vector)`、`- Sigma 3 (Vector)` | 3（对应主方向的单位矢量） |
| Tresca 准则 | `<张量名> - Tresca Criterion` | 1 |
| von Mises 准则 | `<张量名> - Von Mises Criterion` | 1 |

## 计算公式

主应力由 3×3 对称张量的特征值给出（Jacobi 旋转法，`ComputeSymmetricEigen()`），按降序记为 σ1 ≥ σ2 ≥ σ3：

```
Tresca        = σ1 - σ3
von Mises     = sqrt( 0.5 * ((σ1-σ2)² + (σ2-σ3)² + (σ3-σ1)²) )
```

von Mises 也可直接用张量分量计算（两者数值一致，测试中互为验证）：

```
σ_vm² = 0.5 * [ (sxx-syy)² + (syy-szz)² + (szz-sxx)² + 6*(sxy² + syz² + szx²) ]
```

## 调用方式

### C++ 接口

```cpp
#include <TensorView/iGameYieldCriteriaFilter.h>

// 单个张量
auto filter = iGame::YieldCriteriaFilter::New();
filter->SetTensorArrayName("Stress");                              // 留空则自动选取
filter->SetCriterion(iGame::YieldCriteriaFilter::VON_MISES);       // PRINCIPAL_STRESS / TRESCA
filter->SetInput(mesh);

// 多个张量（点 / 单元可混合）
auto multi = iGame::YieldCriteriaFilter::New();
multi->SetTensorArrayNames({"element_solid_stress", "element_shell_stress_ip"});
multi->SetCriterion(iGame::YieldCriteriaFilter::TRESCA);
multi->SetInput(mesh);
if (multi->Execute()) {
    auto out = multi->GetOutput();   // 每个张量各自一组结果数组
} else {
    // multi->GetStatusMessage() 给出失败原因（中文）
}
```

（下面这段示例保留单张量写法）
if (filter->Execute()) {
    auto out = filter->GetOutput();   // 带 "Stress - Von Mises Criterion" 等结果数组
} else {
    // filter->GetStatusMessage() 给出失败原因（中文）
}
```

也可以单独调用特征分解（便于复用）：

```cpp
double a[3][3] = {{1, 2, 3}, {2, 4, 5}, {3, 5, 6}};
double values[3];
double vectors[3][3];
iGame::ComputeSymmetricEigen(a, values, vectors);   // 特征值降序，vectors[i] 为对应单位特征向量
```

### GUI 使用

菜单：**算法处理 → 新增filter/第二批 → 屈服准则 (Yield Criteria)**

1. 加载并选中带张量数组的模型（例如 `Examples/Models/YieldCriteria_Test.vtk`）；
2. 点击菜单弹出参数面板：
   - **点张量 (Point Tensors)**：点数据里所有 6/9 分量数组，**多选列表**（Ctrl / Shift 加选，默认选中第一个）
   - **单元张量 (Cell Tensors)**：单元数据里所有 6/9 分量数组，同为多选列表（模型没有单元张量时不显示）
   - **屈服准则**：主应力 / Tresca / von Mises（单选）
3. 点 **执行**：模型树出现新模型 `xxx_YieldCriteria`，选中的每个张量都会得到一组结果数组，可直接用于云图着色。

示例：模型同时带 `Stress` 与 `Stress2` 时，在两个列表里各选一个（或同一列表里多选），一次执行即可得到
两套结果数组；只选一个时与单选行为一致。

## 测试

### 命令行示例

```bat
:: 在 Examples 构建目录下运行
testYieldCriteria.exe
```

共 **39 项检查**，覆盖：

1. 主应力：σ1/σ2/σ3 与参考值逐点对拍（6 个代表性张量）；
2. 主方向：单位长度、满足 `A·v = σ1·v`、特征值降序；
3. Tresca、von Mises 与参考值逐点对拍；
4. 输出数组命名、结果保留原张量属性；
5. 几何/拓扑不变、原模型不被修改、输出命名规则；
6. 9 分量张量与 6 分量结果一致；
7. 指定张量数组名 / 自动选取 / 找不到属性 / 无张量属性 / 无输入的错误处理；
8. **单元数据**上的张量同样支持（结果保持单元附着）；
9. 9 分量测试模型：`σ1+σ2+σ3` 等于张量迹、von Mises 等于分量公式、Tresca 等于 σ1−σ3（均与解析公式核验，误差 0）；
10. 6 分量测试模型（`.vtu`）：von Mises 与解析公式一致（误差 0）；
11. **多选**：一次选两个张量时各自产出结果数组、与逐个单独执行完全一致、几何不变、同名去重、
    名字不存在时整次失败、主应力准则下每个张量都输出主值与方向矢量。

运行输出 `Result: PASS` 表示通过。

### GUI 链路测试

`out/fsmtest/gui_yield_test.cpp`（Qt 驱动真实主窗口）：加载测试模型 → 触发菜单 →
校验面板给出**多选列表**（选择模式为扩展选择、列出两个张量）与一个准则下拉框 →
在列表里同时选中两个张量并点「执行」→ 校验新增 `<输入名>_YieldCriteria` 模型、点数不变、
两个张量各自的结果数组都存在、原模型未被修改；再只选第二个张量与 Tresca 执行一次，
校验只生成该张量的 `Stress2 - Tresca Criterion`。

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/YieldCriteria_Test.vtk` | 11×11×11 六面体（1331 点 / 1000 单元），点带两个 **9 分量**张量属性 `Stress` / `Stress2`（legacy VTK 的 `TENSORS`）；应力场分别为 `sxx=10x, syy=5y, szz=-8z, sxy=2xy, syz=3yz, szx=4zx` 与 `sxx=-4x, syy=7y, szz=2z, sxy=x+z, syz=y-x, szx=z-y`（均可解析验证），`Stress2` 用于验证多选 |
| `Examples/Models/YieldCriteria_ParaView_Test.vtu` | 同一应力场、**6 分量**对称张量 `Stress`（XML vtu），供在 ParaView 里用 Yield Criteria 对照同一结果 |

用它可以在 GUI 里直观验证：加载该模型 → 屈服准则 → 在列表里选一个或两个张量 → 执行 →
对结果数组做云图着色，可以看到随空间变化的应力场分布。

## 实际工程模型验证

用 LS-DYNA d3plot 导出的 `d3plot_0101.vtu`（2126 点 / 1502 单元 / 21 个属性）实测：

- 该模型的张量数组在单元数据上，名为 `element_solid_stress`（6 分量）；它**没有被标记为张量类型**
  （VTU 读取器给出的是标量类型），本过滤器按**分量数**识别，因此可以直接使用；
- 三种准则均正常：von Mises 范围 0 ~ 0.1909（均值 0.0526）、Tresca 0 ~ 0.2203，
  主应力满足 σ1 ≥ σ2 ≥ σ3，结果与几何一一对应（1502 个单元各一个值）；
- 注意：该模型里壳单元的 `element_shell_stress_ip1/2/3` 会被 VTU 读取器**合并**为 3 分量矢量
  `element_shell_stress_ip`，不是 6 分量，因此不出现在候选列表里。

## 注意事项

1. **需要 6 或 9 分量的数组**：模型里没有这样的数组时菜单会直接提示。数组不必被标记为张量类型，按分量数识别。
2. **标量 / 矢量（3 分量）数组不参与计算**。
3. **主方向仅对主应力准则输出**；Tresca 与 von Mises 只输出一个标量数组。
4. **多选**：列表里选中的每个张量都会计算一遍，结果数组以各自的张量名为前缀；同名会自动去重。
5. **性能**：逐点 Jacobi 特征分解（最多 64 次扫描，通常几次即收敛），1331 点瞬时完成，代价随点数与所选张量个数线性增长。
