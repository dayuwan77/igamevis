# PerlinNoise Perlin 噪声过滤器使用说明

## 功能

`PerlinNoiseFilter` 在**输入数据集的每个点上采样 3D Perlin 噪声**，输出一个独立的深拷贝网格，并为其新增一个点标量数组。几何与拓扑完全不变（只多一列属性），原模型不受影响。

这是一个**过滤器**（必须有输入），不是数据源。

核心行为：

- **输入**：任意网格 / 点集（PointSet 派生；SurfaceMesh、UnstructuredMesh、VolumeMesh、StructuredMesh 等）。
- **输出**：`<输入名>_PerlinNoise` 的新模型，点标量数组默认名 `PerlinNoise`（单分量 double）。
- **采样公式**：

  ```
  xd[i]  = x[i] * Frequency[i] - Phase[i] * 2
  value  = PerlinNoise(xd) * Amplitude
  ```

  其中 `PerlinNoise` 是 Greg Ward 在 Graphics Gems II 中的经典实现（整数格点伪随机值 + 三线性插值 + hermite 平滑）。
- **确定性**：同样的参数与坐标一定得到相同的值，没有随机种子。

## 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 振幅 (Amplitude) | `1` | 噪声幅值 |
| 频率 X/Y/Z (Frequency) | `(1,1,1)` | 各方向的空间频率 |
| 相位 X/Y/Z (Phase) | `(0,0,0)` | 各方向的相位偏移 |
| 标量数组名 | `PerlinNoise` | 输出数组名（仅 C++ 接口可改） |

## 数值验证

与参考实现（`out/pv_perlin_*.py` 脚本采样）逐点对拍，**最大误差 0.000000（浮点一致）**：

| 用例 | 参数 | 最大误差 |
|------|------|----------|
| A | Amplitude=1, Frequency=(1,1,1), Phase=(0,0,0) | 0.000000 |
| B | Amplitude=2, Frequency=(2,2,2), Phase=(0.5,0,0) | 0.000000 |
| C | Amplitude=0.5, Frequency=(1,3,0.5), Phase=(0,0.25,0.75) | 0.000000 |
| D/E | 任意坐标（小数、大坐标） | 2.6e-4 / 2.7e-3（受输入点 float 精度限制） |

> 值域注意：Perlin 噪声的 hermite 插值会**轻微越界**（可超出 ±Amplitude）——
> 参考实现扫描 2 万个点（Amplitude=2.5）得到 `min≈-2.7467 / max≈2.8338`。
> 另外在**整数格点**上插值权重为 0，取值正好等于格点随机值，此时不会越界。

## 调用方式

### C++ 接口

```cpp
#include <AttributeManipulation/iGamePerlinNoiseFilter.h>

auto filter = iGame::PerlinNoiseFilter::New();
filter->SetAmplitude(1.0);                       // 默认 1
filter->SetFrequency(2.0, 2.0, 2.0);             // 默认 (1,1,1)
filter->SetPhase(0.5, 0.0, 0.0);                 // 默认 (0,0,0)
filter->SetScalarArrayName("PerlinNoise");       // 可选，默认即此名
filter->SetInput(mesh);                          // mesh 为 DataObject::Pointer
if (filter->Execute()) {
    auto out = filter->GetOutput();              // 带 PerlinNoise 点标量的新网格
}
```

也可以直接调用噪声内核（便于复用）：

```cpp
double freq[3] = {1, 1, 1};
double phase[3] = {0, 0, 0};
double xyz[3] = {0.5, 0.5, 0.5};
double v = iGame::EvaluatePerlinNoise(xyz, freq, phase, 1.0);
```

### GUI 使用

菜单：**算法处理 → 新增filter/第二批 → Perlin 噪声 (Perlin Noise)**

1. 先加载并选中一个网格模型；
2. 点击菜单弹出参数面板，填写 振幅 / 频率 X,Y,Z / 相位 X,Y,Z；
3. 点 **执行**：模型树出现新模型 `xxx_PerlinNoise`，属性列表里多出 `PerlinNoise` 点标量，可直接用于云图着色。

## 测试

### 命令行示例

```bat
:: 在 Examples 构建目录下运行
testPerlinNoise.exe
```

共 **24 项检查**，覆盖：

1. 三组参数在 21×21×21 规则网格采样点上逐点对拍（误差 0）；
2. 任意坐标（小数、大坐标）逐点对拍；
3. 测试模型文件 `PerlinNoise_Test.vtk` 上两组参数逐点对拍（误差 0）；
4. 输出保持输入几何（点数、坐标不变）；
5. 原模型不被修改（不新增属性）；
6. 默认数组名为 `PerlinNoise`，自定义名生效；
7. 确定性（同参数两次执行结果完全一致）；
8. Amplitude 线性缩放；Frequency / Phase 改变数值；
9. 非整数坐标上插值生效；整数格点取值与参考值一致（无插值）；
10. 空输入、无输入被拒绝；getter 返回 setter 存入的值；输出命名规则。

运行输出 `Result: PASS` 表示通过。

### GUI 链路测试

`out/fsmtest/gui_perlin_test.cpp`（Qt 驱动真实主窗口）：加载 `Examples/Models/PerlinNoise_Test.vtk` →
触发菜单 → 打开参数面板 → 点击「执行」→ 校验场景中新增了 `<输入名>_PerlinNoise` 模型、
点数不变、带 `PerlinNoise` 点标量、原模型未被修改。

## 测试模型

| 文件 | 说明 |
|------|------|
| `Examples/Models/PerlinNoise_Test.vtk` | 11×11×11 六面体网格（1331 点 / 1000 单元），坐标从 −2.5 起、步长 0.5（含非整数坐标，可覆盖插值路径） |

用它可以在 GUI 里直观验证：加载该模型 → 菜单 → Perlin 噪声 → 执行 → 对 `PerlinNoise` 选云图着色，
应看到噪声纹理；把 频率 调到 2~4 会看到更密集的纹理，调 相位 会让纹理整体平移。

## 注意事项

1. **需要输入**：这是过滤器而不是数据源。
2. **点坐标以 float 存储**：大坐标（>1e4）或高精度小数会有约 1e-4 的表示误差，噪声值随之有同量级偏差。
3. **值域可能轻微越界**：hermite 插值特性（见上文说明）。
4. **性能**：纯逐点计算，21³ 网格（9261 点）瞬时完成；大模型上点数线性增长。
