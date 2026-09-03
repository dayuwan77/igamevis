# AppendReduce Filter — ParaView 对比验证说明

## 测试目的

验证 iGameVis 的 **AppendReduce（网格合并去重）** Filter 与 ParaView 原生 **Append Datasets** Filter 在功能上的一致性，包括：
- 几何拓扑合并（顶点 + 面）
- 点属性合并（标量、向量）
- 单元属性合并（标量）
- 属性交集规则（仅合并所有输入共有的属性）

## 测试数据

测试文件位于 `Examples/Models/` 目录：

| 文件 | 顶点数 | 面数 | 点属性 | 单元属性 |
|------|--------|------|--------|----------|
| `AppendReduce_mesh1.vtk` | 9 | 4 | Temperature（标量）、Pressure（标量） | Stress（标量） |
| `AppendReduce_mesh2.vtk` | 9 | 4 | Temperature（标量）、Velocity（向量） | Stress（标量） |

两个网格在 x=2 处共享一条边（3 个顶点完全重合），用于验证顶点合并功能。

**合并后预期（开启顶点合并）**：
- 顶点数：15（9 + 9 - 3 个重合点）
- 面数：8
- 共有属性：Temperature（点标量）、Stress（单元标量）
- 被丢弃（非交集）：Pressure（仅 mesh1 有）、Velocity（仅 mesh2 有）

## ParaView 操作步骤

### 第一步：加载数据

1. 打开 ParaView
2. 菜单 `File → Open`，选择 `AppendReduce_mesh1.vtk` 和 `AppendReduce_mesh2.vtk`（按住 Ctrl 多选）
3. 点击 `OK`，属性面板中点击 `Apply`
4. 在左侧 Pipeline Browser 中可以看到两个数据对象

### 第二步：使用 Append Datasets

1. 确保在 Pipeline Browser 中同时选中两个文件（按住 Ctrl 点选）
2. 菜单 `Filters → Alphabetical → Append Datasets`
3. 属性面板中点击 `Apply`

### 第三步：验证结果

**几何验证：**
- 查看信息面板（`Information` 标签）
- 确认 Number of Points = 15
- 确认 Number of Cells = 8

**点属性验证（Temperature）：**
1. 在 Pipeline Browser 中选中 AppendDatasets
2. 顶部工具栏找到 `Coloring` 下拉菜单，选择 `Temperature`
3. 确认整个合并后的网格都有颜色渐变（从左到右 100→540 连续过渡）
4. 截图保存为 `paraview_temperature.png`

**单元属性验证（Stress）：**
1. 在 `Coloring` 旁边切换到 `Cell Data` 模式（点/单元切换按钮）
2. 选择 `Stress`
3. 确认 8 个面都有颜色（左半 10-25，右半 30-45）
4. 截图保存为 `paraview_stress.png`

**属性交集验证：**
1. 查看 `Coloring` 下拉列表
2. 确认 **没有** Pressure 属性（mesh1 有但 mesh2 没有）
3. 确认 **没有** Velocity 属性（mesh2 有但 mesh1 没有）
4. 截图保存为 `paraview_attribute_list.png`

## iGameVis 操作步骤

### 第一步：加载数据

1. 打开 iGameVis
2. 菜单 `文件 → 打开`，选择 `AppendReduce_mesh1.vtk`
3. 再次打开 `AppendReduce_mesh2.vtk`
4. 左侧模型树中可以看到两个模型

### 第二步：使用 AppendReduce

1. 菜单 `算法处理 → 网格合并去重 (Append/Reduce)`
2. 弹出参数对话框：
   - **合并重复顶点**：勾选
   - **合并容差**：1e-6
3. 点击「执行」
4. 合并结果自动显示在模型树中，颜色映射自动激活

### 第三步：验证结果

**几何验证：**
- 在模型信息面板中查看输出顶点数和面数
- 预期：15 个顶点（合并去重后），8 个面

**点属性验证（Temperature）：**
1. 选中模型树中的 `append_reduce_result`
2. 右侧属性面板中选择 `Temperature` 标量
3. 确认颜色渐变从左到右连续过渡，接缝处无断层
4. 截图保存为 `igamevis_temperature.png`

**单元属性验证（Stress）：**
1. 切换到单元数据模式
2. 选择 `Stress` 标量
3. 确认 8 个面都有颜色显示
4. 截图保存为 `igamevis_stress.png`

**属性交集验证：**
1. 查看属性下拉列表
2. 确认只有 Temperature 和 Stress
3. 截图保存为 `igamevis_attribute_list.png`

## 对比验证清单

| 验证项 | ParaView 预期 | iGameVis 预期 | 是否一致 |
|--------|--------------|---------------|---------|
| 输出顶点数（合并开） | 15 | 15 | ✅ |
| 输出面数 | 8 | 8 | ✅ |
| Temperature 点标量存在 | 是 | 是 | ✅ |
| Temperature 颜色过渡 | 连续渐变 | 连续渐变 | ✅ |
| Stress 单元标量存在 | 是 | 是 | ✅ |
| Pressure 属性（仅mesh1有） | 不存在 | 不存在 | ✅ |
| Velocity 属性（仅mesh2有） | 不存在 | 不存在 | ✅ |
| 输出顶点数（合并关） | 18 | 18 | ✅ |

## 截图要求（PR 提交用）

建议拼接成 2-3 张对比图：

**图1：Temperature 点标量对比**
- 左：ParaView 中 Temperature 颜色映射
- 右：iGameVis 中 Temperature 颜色映射
- 标题：点标量属性合并对比（Temperature）

**图2：Stress 单元标量对比**
- 左：ParaView 中 Stress 颜色映射（Cell Data 模式）
- 右：iGameVis 中 Stress 颜色映射
- 标题：单元标量属性合并对比（Stress）

**图3：几何与属性列表对比**
- 上：ParaView Information 面板 + 属性列表
- 下：iGameVis 模型信息面板 + 属性列表面板
- 标题：几何统计与属性交集规则对比

## ParaView 对应 Filter 说明

ParaView 中与 AppendReduce 对标的是 **Append Datasets** Filter（`vtkAppendDataSets`）：

- 输入：任意数量的数据集
- 输出：合并后的单个数据集
- 几何：拼接所有点和单元，顶点索引自动偏移
- 属性：**交集合并** —— 只有所有输入都包含的同名同类型属性才会被合并到输出
- 不进行顶点合并（ParaView 如需去重需额外加 `Clean To Grid` 或 `Merge Points` Filter）

iGameVis 的 AppendReduce 在 Append Datasets 基础上额外提供了空间哈希顶点合并功能，相当于 ParaView 中 `Append Datasets + Clean To Grid` 的组合。
