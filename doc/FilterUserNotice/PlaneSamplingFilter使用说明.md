# PlaneSamplingFilter 使用说明

## 功能描述
平面采样过滤器（PlaneSamplingFilter）在指定平面上生成规则采样点，读取输入模型在采样点位置的数值，输出一个包含采样点及其属性值的点集。

**核心功能：**
1. **单元定位**：查找采样点所在的四面体/三角形单元
2. **单元内插值**：使用体积坐标/重心坐标进行插值
3. **六面体支持**：自动将六面体拆分为四面体
4. **后备方案**：对边界未定位点采用最近点采样补充
5. **属性传递**：支持标量和矢量属性


## 调用方式

### 菜单路径
**算法处理 → 平面采样 (Plane Sampling)**

### 参数说明
| 参数              | 说明                | 默认值     |
|-------------------|--------------------|------------|
| 平面原点 X/Y/Z    | 采样平面的中心点坐标  | (0, 0, 0) |
| 平面法向 X/Y/Z    | 采样平面的法向向量    | (0, 0, 1) |
| 采样分辨率 (N×N)  | 每个方向的采样点数    | 20        |
| 采样属性          | 选择要采样的属性      | 自动检测   |


## 使用示例

### 示例1：
1. 加载模型 `ContourExtraction_cylinder_UnstructuredGrid.vtk`
2. 点击 **算法处理 → 平面采样 (Plane Sampling)**
3. 保持默认参数，点击 **执行**
4. 输出 20×20 = 400 个采样点

### 示例2：
1. 加载模型 `analysis_vtu_subcase1.vtu`
2. 设置平面原点 (0, 0, 0)，法向 (0, 0, 1)
3. 分辨率设为 20×20
4. 选择属性 `StaticDisplacement`
5. 点击 **执行**


## 测试验证

### 测试数据
| 模型 | 分辨率 | 有效点 | 单元插值 | 后备 |
|------|-------|--------|---------|------|
| `ContourExtraction_cylinder_UnstructedGrid.vtk` | 20×20 | 20/400 | 16 | 4 |
| `analysis_vtu_subcase1.vtu` | 20×20 | 276/400 | 276 | 0 |


### 编译并运行测试

```bash
# 编译测试用例
cd D:/igamevis
cmake --build cmake-build-examples --target testPlaneSamplingFilter --config Release

# 运行测试
cd cmake-build-examples
set PATH=D:/igamevis/cmake-build-release/install/bin;D:/HDF5/bin;%PATH%
Release\testPlaneSamplingFilter.exe
```

### 预期输出

========== PlaneSamplingFilter Test ==========
Input  - Points: 8499, Cells: 7472
Output - Points: 400, Cells: 361
PASS: Point count matches (400)
PASS: Cell count matches (361 quadrilaterals)
PASS: Output has attributes (5 arrays)

========== ALL TESTS PASSED ==========


## 注意事项
1. 模型必须是网格类型（支持 UnstructuredMesh / SurfaceMesh / VolumeMesh / StructuredMesh）
2. 采样分辨率建议 20-100，过高会影响性能
3. 平面法向不能为零向量
4. 模型外采样点标记为无效（vtkValidPointMask = 0）