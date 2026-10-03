# TensorPrincipalInvariantsFilter 使用说明

对应任务：中等任务 —— 计算对称张量的主值（特征值）与主方向（特征向量）（`tensor_principal_invariants`）。

## 1. 功能

`TensorPrincipalInvariantsFilter` 对输入网格上的**对称张量数组**逐元做对称特征分解，
把主值与主方向作为新的属性数组追加到一个**独立输出的非结构化网格**上：

| 输出属性名 | 类型 | 挂载位置 | 含义 |
| --- | --- | --- | --- |
| `<原名> - Sigma 1/2/3` | DoubleArray（标量） | 与输入张量相同（点/单元） | 第 1/2/3 主值，**按从大到小排序** |
| `<原名> - Sigma 1/2/3 (Vector)` | DoubleArray（3 分量） | 同上 | 第 1/2/3 主方向（默认单位向量） |

数组命名规则为 `<原名> - Sigma N`（标量）与 `<原名> - Sigma N (Vector)`（主方向），
写入顺序固定为**先 3 个向量、再 3 个标量**，便于下游按固定顺序读取；
`<原名>` 是输入张量数组名，例如 `stress - Sigma 1`。

### 1.1 输入张量的分量约定

| 分量数 | 维数 | 分量顺序 |
| --- | --- | --- |
| 6 | 3D 对称张量 | `XX, YY, ZZ, XY, YZ, XZ` |
| 3 | 2D 对称张量 | `XX, YY, XY` |

装成 3×3 对称矩阵后做对称特征分解，特征值按**降序**输出为 `Sigma 1 ≥ Sigma 2 ≥ Sigma 3`。

### 1.2 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `SetTensorArrayName(name)` | 无（必填） | 待处理的张量数组名 |
| `SetArrayAttachment(IG_POINT / IG_CELL)` | `IG_POINT` | 张量挂在点数据还是单元数据 |
| `SetScaleVectors(bool)` | `false` | 主方向是否乘以对应主值（`false` 时输出单位向量，`true` 时长度为对应主值的绝对值） |

## 2. 支持的数据类型与失败行为

支持的输入类型：`UnstructuredMesh` / `SurfaceMesh` / `VolumeMesh`（内部统一转成 UnstructuredMesh 表示）。

以下情况返回 `false`，原因见 `GetMessage()`（**不静默跳过、不写错误结果**）：

| 情况 | 提示信息（节选） |
| --- | --- |
| 没有输入 / 输入为空 | `no input data` / `input data is null` |
| 未指定数组名 | `tensor array name is empty` |
| 数组不存在 | `tensor array 'xxx' not found on point data / cell data` |
| 分量数不是 3 / 6 | `array 'xxx' has N components; a symmetric tensor needs 6 (3D: XX,YY,ZZ,XY,YZ,XZ) or 3 with IG_TENSOR type (2D: XX,YY,XY)` |
| 数组长度与点数 / 单元数不一致 | `tensor array tuple count (N) does not match the mesh point count (M)` |
| 0 个点 / 0 个单元 | `input mesh has no points` / `input mesh has no cells` |

**张量分量含 NaN** 时该元组的 6 个输出全部写 NaN（便于在界面上识别），
并把"含 NaN 的元组数"写入 `GetMessage()`，不做静默处理。

## 3. 调用方式

### 3.1 GUI 方式

1. 打开/导入一个带对称张量数组的模型，在模型树中选中它；
2. 主菜单 **算法处理 → 张量主值 (Tensor Principal Invariants)**，打开左侧面板；
3. 下拉框里**只列出可作为张量的数组**（6 分量，或类型为 `IG_TENSOR` 的 3 分量），
   形如 `stress  [点数据, 6 分量]`；
4. 按需勾选 **主方向按主值缩放**，点 **执行**；
5. 结果作为独立节点（`<原名>_PrincipalInvariants`）加入模型树，可按
   `<张量名> - Sigma 1` 着色查看第一主值的分布；面板下方列出本次写出的 6 个数组名。

### 3.2 代码方式

```cpp
#include <TensorPrincipalInvariants/iGameTensorPrincipalInvariantsFilter.h>

auto filter = iGame::TensorPrincipalInvariantsFilter::New();
filter->SetTensorArrayName("stress");      // 6 分量对称张量（XX,YY,ZZ,XY,YZ,XZ）
filter->SetArrayAttachment(IG_POINT);      // 点数据；单元数据用 IG_CELL
filter->SetScaleVectors(false);            // 输出单位主方向
filter->SetInput(mesh);
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << std::endl;
} else {
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // out 上新增 6 个数组：stress - Sigma 1/2/3 与 stress - Sigma N (Vector)
}
```

## 4. 使用示例

模型 `Examples/Models/TensorPrincipalInvariants_symmetric.vtk`（4 点 / 1 四边形，点数据 `stress`）：

| 元组 | 输入（XX, YY, ZZ, XY, YZ, XZ） | 期望主值 |
| --- | --- | --- |
| 0 | `3, 2, 1, 0, 0, 0` | `3, 2, 1` |
| 1 | `2, 2, 5, 1, 0, 0`（即 `[[2,1,0],[1,2,0],[0,0,5]]`） | `5, 3, 1` |
| 2 | `2, 2, 2, 0, 0, 0`（各向同性） | `2, 2, 2` |
| 3 | `10, -2, -8, 4, 0, 0` | 降序且满足 `Σ = trace = 0`、`Π = det` |

交叉校验（测试里逐元断言，容差 1e-9）：

- `Sigma 1 ≥ Sigma 2 ≥ Sigma 3`；
- `Sigma1 + Sigma2 + Sigma3 == trace(T)`；
- `Sigma1 × Sigma2 × Sigma3 == det(T)`；
- `T · v == λ · v`（主方向确实是特征向量，残差 < 1e-9）；
- `ScaleVectors = false` 时 `|v| == 1`，`true` 时 `|v| == |λ|`。

另有 `TensorPrincipalInvariants_field.vtk`（8×8 网格 + 空间变化的张量场）用于可视化演示
（按 `stress - Sigma 1` 伪彩着色）。

## 5. 注意事项

1. **3 分量数组何时按二维张量处理**：iGame 的数组没有"分量名"概念，因此以属性类型
   `IG_TENSOR` 作为判据 —— 6 分量数组一律按 3D 张量处理；3 分量数组只有在类型为
   `IG_TENSOR` 时才按 2D 张量（`XX, YY, XY`）处理，这样不会把普通三维向量数组误当张量。
2. **主方向的符号惯例固定、结果可复现**：特征向量 `v` 与 `-v` 表示同一根轴的两支，
   本实现统一做**符号规范化**（按主值分布重排、令指定分量为正；主值相等、方向不唯一的
   退化情形由叉乘重建），因此同样的输入每次都会给出同样的方向，便于复现与逐点对照。
   主值 `Sigma` 本身与符号无关。
3. 结果数组名里含空格与括号，导出为 `.vtk` 时框架会把非字母数字字符百分号编码
   （`stress - Sigma 1` → `stress%20-%20Sigma%201`），iGameVis 读回时会自动还原。
4. 重复执行不会堆积结果数组（写之前先删同名的 6 个数组）。
5. 输入模型不会被改动（输出是独立的深拷贝节点，含全部原有属性数组）。

## 6. 测试

```bash
cd Examples
./testTensorPrincipalInvariants
```

覆盖：点的 3D 张量（解析值 + 不变量 + 残差）、`ScaleVectors`、单元上的 `IG_TENSOR` 张量、
NaN 元组处理、失败场景（2 分量 / 非 IG_TENSOR 的 3 分量 / 数组不存在 / 空网格）、重复执行。
断言全部通过才输出 `PASS`。
