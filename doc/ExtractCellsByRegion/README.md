# Extract Cells By Region

新增 `extract_cells_by_region` filter，支持按 Box 或 Sphere 区域提取非结构网格单元，并支持两种筛选策略：

- 严格模式：单元的所有顶点都在区域内；
- 宽松模式：单元的任一顶点在区域内即可。

本次接入覆盖核心 Filter、Qt 命令分发、MCP 工具、Qt 菜单入口和 Examples 测试用例。

## 验证结果

使用 `Tet_Plane.vtk` 测试：

```text
Box(strict) selected cells: 22256
Sphere(loose) selected cells: 30700
```

完整输出见 [verification-output.txt](verification-output.txt)，界面验证见 [gui-verification.png](gui-verification.png)。
