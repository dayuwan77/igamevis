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
| 张量属性 | 当前模型里所有 6 / 9 分量的张量属性 | 留空则自动取第一个 |
| 屈服准则 | 主应力 / Tresca 准则 / von Mises 准则 | 见下表 |

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

auto filter = iGame::YieldCriteriaFilter::New();
filter->SetTensorArrayName("Stress");                              // 留空则自动选取
filter->SetCriterion(iGame::YieldCriteriaFilter::VON_MISES);       // PRINCIPAL_STRESS / TRESCA
filter->SetInput(mesh);
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

菜单：**算法处理 → 数据属性操作 (Attribute Manipulation) → 屈服准则 (Yield Criteria)**

1. 加载并选中带张量属性的模型（例如 `Examples/Models/YieldCriteria_Test.vtk`）；
2. 点击菜单弹出参数面板：**张量属性**下拉框（列出模型里的 6/9 分量张量属性）、**屈服准则**下拉框（主应力 / Tresca / von Mises）；
3. 点 **执行**：模型树出现新模型 `xxx_YieldCriteria`，其属性列表里多出结果数组，可直接用于云图着色。

## 测试

### 命令行示例

```bat
:: 在 Examples 构建目录下运行
testYieldCriteria.exe
```

共 **31 项检查**，覆盖：

1. 主应力：σ1/σ2/σ3 与参考值逐点对拍（6 个代表性张量）；
2. 主方向：单位长度、满足 `A·v = σ1·v`、特征值降序；
3. Tresca、von Mises 与参考值逐点对拍；
4. 输出数组命名、结果保留原张量属性；
5. 几何/拓扑不变、原模型不被修改、输出命名规则；
6. 9 分量张量与 6 分量结果一致；
7. 指定张量属性名 / 自动选取 / 找不到属性 / 无张量属性 / 无输入的错误处理；
8. **单元数据**上的张量同样支持（结果保持单元附着）；
9. 9 分量测试模型：`σ1+σ2+σ3` 等于张量迹、von Mises 等于分量公式、Tresca 等于 σ1−σ3（均与解析公式核验，误差 0）；
10. 6 分量测试模型（`.vtu`）：von Mises 与解析公式一致（误差 0）。

运行输出 `Result: PASS` 表示通过。

### GUI 链路测试

`out/fsmtest/gui_yield_test.cpp`（Qt 驱动真实主窗口）：加载测试模型 → 触发菜单 → 面板出现两个下拉框 →
点「执行」→ 校验新增 `<输入名>_YieldCriteria` 模型、点数不变、主应力与方向矢量数组存在、原模型未被修改；
再把准则切换为 Tresca 执行一次，校验 `Stress - Tresca Criterion` 生成。

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/YieldCriteria_Test.vtk` | 11×11×11 六面体（1331 点 / 1000 单元），点带 **9 分量**张量属性 `Stress`（legacy VTK 的 `TENSORS`），应力场为 `sxx=10x, syy=5y, szz=-8z, sxy=2xy, syz=3yz, szx=4zx`（解析可验证） |
| `Examples/Models/YieldCriteria_ParaView_Test.vtu` | 同一应力场、**6 分量**对称张量 `Stress`（XML vtu），供在 ParaView 里用 Yield Criteria 对照同一结果 |

用它可以在 GUI 里直观验证：加载该模型 → 屈服准则 → 选 `Stress` 与某个准则 → 执行 →
对结果数组做云图着色，可以看到随空间变化的应力场分布。

## 注意事项

1. **需要 6 或 9 分量的数组**：模型里没有这样的数组时菜单会直接提示。数组不必被标记为张量类型，按分量数识别。
2. **标量 / 矢量（3 分量）数组不参与计算**。
3. **主方向仅对主应力准则输出**；Tresca 与 von Mises 只输出一个标量数组。
4. **性能**：逐点 Jacobi 特征分解（最多 64 次扫描，通常几次即收敛），1331 点瞬时完成，代价随点数线性增长。
