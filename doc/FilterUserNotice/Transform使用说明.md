# Transform 通用几何变换 使用说明

## 1. 功能

`TransformFilter` 对输入模型做一次仿射变换：

```text
平移缩放旋转 + 轴心：p' = t + o + R·S·(p − o)
   t  平移（SetTranslation）
   R  旋转（SetRotation，绕 Z 再 Y 再 X：Rz·Ry·Rx，单位：度）
   S  缩放（SetScale）
   o  轴心（SetPivot，默认 (0,0,0)）——旋转与缩放都绕该点进行
```

| 参数 | 接口 | 默认值 | 说明 |
| --- | --- | --- | --- |
| 平移 | `SetTranslation(tx,ty,tz)` | 0,0,0 | 整体平移量 |
| 旋转 | `SetRotation(rx,ry,rz)` | 0,0,0 | 单位为度，按 Rz·Ry·Rx 顺序 |
| 缩放 | `SetScale(sx,sy,sz)` | 1,1,1 | 各轴缩放系数，不能为 0 |
| 轴心 | `SetPivot(px,py,pz)` | 0,0,0 | 旋转/缩放的中心；为 (0,0,0) 时与旧行为完全一致 |

输出是与输入**同类型的独立对象**（深拷贝几何与属性），输入模型不被修改。

## 2. 输入要求

支持的类型：`SurfaceMesh`（表面网格）、`UnstructuredMesh`（非结构网格）、
`VolumeMesh`（体网格）、`StructuredMesh`（结构网格）、`PointSet`（点云）。

- 其它类型（例如 `.pvd` / `.igcm` 读出的多帧容器 `IG_DRAW_OBJECT`）会执行失败并给出提示；
  多帧容器的帧是"按需加载"的，只变换当前帧会导致切换时间步后又变回未变换的几何，因此暂不支持。
- 属性处理：`IG_VECTOR` 用线性部分 `L` 变换，`IG_NORMAL` 用 `(L⁻¹)ᵀ` 变换并归一化
  （缩放不可逆时跳过法向量），其余属性原样保留。

## 3. 调用方式

### 3.1 GUI 方式（可拖拽盒子）

1. 在模型树中选中一个网格或点云，菜单 **Transform（变换） → 通用几何变换**；
2. 视图中会出现一个贴合模型包围盒的**可拖拽盒子**，面板上出现 9 个参数输入框：

| 鼠标操作（**中键**） | 效果 |
| --- | --- |
| 拖盒子中心、棱 | 平移（`平移 X/Y/Z` 实时更新） |
| 拖六个面手柄 | 沿该轴缩放（`缩放 X/Y/Z` 实时更新） |
| 拖盒子表面 | 旋转（`旋转 X/Y/Z` 实时更新） |


1. 也可以直接在面板里输入参数，盒子会同步跟随；
2. 点 **应用** 变换模型：结果作为**新节点**（`<原名>_Transform`）加入模型树，

3. 点 **重置** 把参数恢复为单位变换、盒子回到初始包围盒（模型不变，需要再次应用才生效）；
4. 关闭面板会移除盒子，但**已经生成的结果节点都会保留**；从未应用过则模型没有任何变化。

### 3.2 代码方式

```cpp
#include <Transformation/iGameTransformFilter.h>

auto filter = iGame::TransformFilter::New();
filter->SetInput(mesh);
// 绕模型自身中心旋转（轴心取包围盒中心）
const auto& bbox = mesh->GetBoundingBox();
filter->SetPivot((bbox.min[0] + bbox.max[0]) * 0.5f,
                 (bbox.min[1] + bbox.max[1]) * 0.5f,
                 (bbox.min[2] + bbox.max[2]) * 0.5f);
filter->SetRotation(0.0f, 0.0f, 90.0f);
filter->SetTranslation(0.0f, 0.0f, 0.0f);
filter->SetScale(1.0f, 1.0f, 1.0f);
if (!filter->Execute()) { /* 输入类型不支持或参数非法 */ }
auto output = filter->GetOutput();
```

### 3.3 命令行测试

```bash
./testTransform        # 或 ctest -R testTransform
```

用例用代码构造的立方体点云自检四项：点云输入可用、轴心绕自身中心（中心不变）、
负缩放镜像、默认轴心为原点（向后兼容），全部通过时输出 `PASS`。

## 4. 注意事项

1. **轴心决定"绕哪里转"**：不设置轴心时旋转/缩放绕坐标原点，模型离原点越远"甩得越厉害"；
   GUI 面板会自动把轴心设为模型的初始包围盒中心，因此拖拽是"绕模型自身中心"的。
2. **缩放不能为 0**（矩阵会奇异）；交互缩放会被限制在 1e-3 以上。
3. **负缩放（镜像）**：filter 支持负缩放；盒子本身只能显示正尺寸（`DynamicBox` 的长度下限为 0.01），
   所以镜像不会在盒子上"翻面"显示——盒子的位置/大小与镜像后的包围盒一致，镜像方向由参数的符号表达；
   拖动盒子时会保留该符号，不会被改写成正数。
4. **变换盒子与"框选"盒子共用同一个交互门控**：打开本面板会先移除框选盒子，
   两者不能同时使用。
5. **每次应用都会新增一个节点**（便于对比不同参数下的结果），参数不会归零。
6. 右键点击盒子会触发框架框选逻辑的"取消"（把盒子临时关掉），面板会自动把它恢复，
   不影响继续操作。

## 5. 相关文件

| 内容 | 路径 |
| --- | --- |
| 过滤器实现 | `iGameCore/Filters/Transformation/iGameTransformFilter.h` / `.cpp` |
| 交互控制器 | `Qt/include/IQWidgets/igQtTransformBoxController.h` / `Qt/src/IQWidgets/igQtTransformBoxController.cpp` |
| 菜单与面板接入 | `Qt/src/IQCore/igQtMainWindow.cpp`（Transform（变换） → 通用几何变换） |
| 用例 | `Examples/Filter/Transformation/TestTransform.cpp` |
