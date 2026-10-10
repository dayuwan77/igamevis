# CleanToGridFilter 使用说明

## 功能描述

网格清理过滤器（CleanToGridFilter）用于清理网格中的质量问题，功能对标 ParaView 的 CleanToGrid 过滤器。

**核心功能：**
1. **合并重合点**：基于容差合并距离小于容差的点
2. **移除退化单元**：检测并移除退化的三角形/四面体等单元
3. **移除未使用的孤立点**：清理不被任何单元引用的点
4. **输出为非结构网格**：统一输出格式，便于后续处理


## 调用方式

### 菜单路径
**算法处理 → 网格清理 (Clean to Grid)**

### 参数说明
| 参数 | 说明 | 默认值 |
|:---|:---|:---|
| 合并容差 (Tolerance) | 合并点的距离阈值 | 0.001 |
| 容差类型 | 绝对/相对容差 | 绝对 (Absolute) |


## 使用示例

### 示例 ：清理带重合点的网格

1. 加载模型 `Convert_Quad_Bicycle.vtk`
2. 点击 **算法处理 → 网格清理 (Clean to Grid)**
3. 保持默认参数（容差 0.001，绝对容差）
4. 点击 **执行**

**结果：**
- 点数：82,212 → 53,878（减少 34.5%）
- 单元数：96,668 → 69,436（减少 28.2%）


## 测试验证

### 测试数据
使用 `Convert_Quad_Bicycle.vtk` 作为测试数据：

| 指标 | 清理前 | 清理后 | 变化 |
|------|-------|--------|-----|
| 点数 | 82,212 | 53,878 | -34.5% |
| 单元数 | 96,668 | 69,436 | -28.2% |

与 ParaView 6.2.0 CleanToGrid 结果一致。

### 编译并运行测试

```bash
# 编译测试用例
cd D:/igamevis
cmake --build cmake-build-examples --target testCleanToGridFilter --config Release

# 运行测试
cd cmake-build-examples
set PATH=D:/igamevis/cmake-build-release/install/bin;%PATH%
Release\testCleanToGridFilter.exe
```

### 预期输出

========== CleanToGridFilter Test ==========
Input  - Points: 82212, Cells: 96668
Output - Points: 53878, Cells: 69436
Change - Points: -28334 (34.5%)
Change - Cells:  -27232 (28.2%)
PASS: Point count matches ParaView (53,878)
PASS: Cell count matches ParaView (69,436)

========== ALL TESTS PASSED ==========


## 注意事项

1. **容差设置**：容差过大会导致模型严重变形，建议从 0.001 开始测试
2. **适用类型**：支持 UnstructuredMesh、SurfaceMesh、VolumeMesh、StructuredMesh
3. **性能考虑**：当前使用暴力搜索合并点，大网格（> 10 万点）可能需要等待 1-2 秒
4. **属性保留**：点属性和单元属性都会被完整保留
5. **输出类型**：始终输出为非结构网格（UnstructuredMesh）