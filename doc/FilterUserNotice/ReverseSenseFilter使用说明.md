# ReverseSense 使用说明

反转面朝向（Reverse Sense）过滤器：翻转曲面朝向。做两件事：

1. **反转面顺序**（默认开）——反转每个面的顶点环序，从而翻转几何法向；
2. **反转法向**（默认开）——把点/单元法向数组（`IG_NORMAL`）逐分量取反。

目前仅支持 `SurfaceMesh` 输入，输出为一份**独立**的 `SurfaceMesh`。

## 一、功能

- 输入：`SurfaceMesh`。输出：新的 `SurfaceMesh`，点数、面数、单元类型均不变。
- 反转面顺序：逐面把顶点索引环序倒置（三角形 `(a,b,c)→(c,b,a)`，四边形 `(a,b,c,d)→(d,c,b,a)`，任意多边形同理）。
- 反转法向：把点属性 / 单元属性中类型为 `IG_NORMAL` 的数组逐分量取负（没有法向数组时自动跳过）。
- 其它属性（标量、向量、整型等）按**原数组类型**原样搬运，不做精度转换。
- 面环序改变后，输出会重建边（Edges）与点→面、边→面邻接关系，保证下游（选择 / 连通性等）拿到的面连接关系与新环序一致。

## 二、界面入口

【算法处理】→「开发中filter/第二批」→「**反转面朝向 (Reverse Sense)**」，对话框两个勾选：**反转面顺序**、**反转法向**。执行后输出作为新节点加入模型树。

## 三、调用方式（代码）

```cpp
#include "ReverseSense/iGameReverseSenseFilter.h"

auto filter = iGame::ReverseSenseFilter::New();
filter->SetInput(surfaceMesh);        // 输入 SurfaceMesh
filter->SetReverseCells(true);        // 是否反转面顶点环序（默认 true）
filter->SetReverseNormals(true);      // 是否反转点/单元法向（默认 true）
if (!filter->Execute()) {
    std::cerr << filter->GetMessage() << "\n";   // 失败原因
    return 1;
}
auto output = filter->GetOutput();    // 反转后的独立 SurfaceMesh
```

## 四、使用示例（自动测试，无需手动输入）

仓库自带程序化/AI 生成模型（`Examples/Models/`）：

- `AIGen_Surface_ReverseSenseDemo.obj`：半圆柱开口曲面，具有明确正反面，GUI 示例默认使用。
- `AIGen_Surface_WaveSheet.obj`：波浪面，法向分量起伏明显，便于先算法向再按法向着色观察翻转。
- `AIGen_Surface_AxisFaces.obj`：三块互相垂直的方片，坐标为整数、法向为 `(0,0,1)/(1,0,0)/(0,1,0)`，数据最直观。

运行（构建后工作目录为 `Examples`，路径写死、自动读取）：

```powershell
# Windows（Visual Studio 生成器）
cd cmake-build-reverse-sense\Examples
.\Release\testReverseSense.exe

# CTest
ctest -R testReverseSense
```

无 GUI 自动回归（程序内构造网格，断言全部 PASS 返回 0）：

```powershell
.\Release\testReverseSenseSelfCheck.exe   # 35 项检查 PASS
ctest -R testReverseSenseSelfCheck
```

## 五、注意事项

- 本过滤器**只接受 `SurfaceMesh`**；传入 `UnstructuredMesh` / 体网格 / 空网格会返回 `false`，原因见 `GetMessage()`。
- 「反转面顺序」只改面环序，不改点坐标；「反转法向」只对 `IG_NORMAL` 数组取反，不重算法向。二者独立开关，可只翻转几何或只翻转法向。
- 输出是独立对象：点集为深拷贝，面为新建 `CellArray`，不会修改输入模型。
- 若要观察法向翻转效果，输入需带有 `IG_NORMAL` 点/单元属性（可先用「曲面/点法向量计算」生成，再按 `Normals` 着色观察）。

## 六、相关文件

| 文件 | 说明 |
| --- | --- |
| `iGameCore/Filters/ReverseSense/iGameReverseSenseFilter.h/.cpp` | 过滤器实现 |
| `Examples/Filter/ReverseSense/TestReverseSense.cpp` | GUI 示例（默认读 `AIGen_Surface_ReverseSenseDemo.obj`，自动运行） |
| `Examples/Filter/ReverseSense/TestReverseSenseSelfCheck.cpp` | 无 GUI 自动回归（35 项断言） |
| `Examples/Models/AIGen_Surface_ReverseSenseDemo.obj`、`..._WaveSheet.obj`、`..._AxisFaces.obj` | 程序化/AI 生成测试模型 |
