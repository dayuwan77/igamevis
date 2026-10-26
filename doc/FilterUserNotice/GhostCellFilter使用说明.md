# Ghost Cells：基于已有分区生成幽灵单元

## 范围
单机执行；输入是包含已有分区的多块模型，叶节点须为由线性四面体、六面体组成的
UnstructuredMesh。每个叶块作为同一数据集的一个分区。支持嵌套容器，输出保留非空
层级、分区次序和名称，创建独立网格，不修改输入。单个网格可执行，但不会自动分区。

不实现 MPI、自动分区、结构化网格、高阶单元或多面体。旧的 GhostPoints → GhostCells
布尔属性转换已被替换；旧的自定义 GhostPoints/GhostCells 不参与生成和移除。

## 操作
1. 加载已经分区的多块模型，例如引用多个 VTU 分区的 VTM；选择根节点。
2. 打开“生成幽灵单元 (Ghost Cells)”，设置 Ghost 层数，默认 1。
3. 执行后，每个分区保留自己的单元，并获得相邻分区的单元副本。
4. 对结果根节点执行“移除 Ghost 信息”，递归移除各分区的重复/隐藏单元并清除标准标记。

层数按共享点的单元邻接逐层扩展，不仅限于共享面的邻接。只与原本直接相邻的分区交换；
增加层数会深入这些相邻分区内部，不会沿分区链继续传播到未直接相邻的分区。
0 层不增加单元，但仍处理共享点所有权。
当设置层数超过可达范围时，不会重复添加单元。重复执行会先剥离原有 DUPLICATECELL，
然后重新生成，因此不会不断累积副本。隐藏点及关联单元、隐藏单元不参加交换。

## 共享点与属性
- 所有分区都存在标量整数点属性 GlobalPointIds 时，以该 ID 匹配。C++ 可指定其他数组名，
  或设为空字符串强制使用坐标。全局整数 ID 及属性使用原始存储复制，保留超过 2^53 的整数。
- 没有该数组时，按框架存储的 float 坐标精确匹配，不使用人为距离容差。
- 同一全局 ID 必须有一致坐标。分区内部重复的点键、只有部分分区提供 ID、
  非法单元连接或不匹配的属性结构均报错，不输出部分结果。
- 各分区的普通点/单元数组须具有一致的名称、存储类型、分量数，并与点/单元数匹配。
  支持现有数值数组；不支持的存储或其他附着类型报错。
- 共享点以输入遍历顺序中的首个分区为所有者；其坐标和点属性用于同步其他分区的副本。
- 几何坐标相同但拓扑不相连的网格，应提供不同的全局点 ID，避免被当成共享点。
- 未标记的重叠原生单元视为非法分区；已有重复单元应使用标准 DUPLICATECELL 标记。

## 标准 vtkGhostType
点和单元都写入 UnsignedCharArray：
- 单元副本：DUPLICATECELL = 1。
- 同一共享点只保留一个非 ghost 拷贝，其余为 DUPLICATEPOINT = 1。
- 原有其他位保留；隐藏点位 2、隐藏单元位 32 用于剥离隐藏数据。
- 0 层输出不包含单元 vtkGhostType，但仍包含点标记。

“移除 Ghost 信息”识别标准 vtkGhostType，按 (1 | 32) 掩码移除单元，再重建连接、
移除无引用点、复制普通数值属性，并删除点级/单元级标记。生成结果可直接作为其输入。
只有点标记时，该移除过滤器只删除标记，不删除关联单元。

## C++ 调用
```cpp
auto ghost = iGame::GhostCellFilter::New();
ghost->SetInput(partitionRoot);
ghost->SetNumberOfGhostLayers(1);
// ghost->SetGlobalPointIdsArrayName("GlobalPointIds");
if (!ghost->Execute()) {
    std::cerr << ghost->GetLastError() << std::endl;
    return;
}
auto remove = iGame::RemoveGhostInformationFilter::New();
remove->SetInput(ghost->GetOutput());
if (remove->Execute()) {
    auto cleanPartitions = remove->GetOutput();
}
```

## 验证
使用临时程序验证四面体共享面、六面体三分区的 0/1/2/7 层、嵌套结构、共享点、
64 位属性、全局 ID、原输入不变、重复执行、生成后移除及非法输入。
临时测试文件与构建入口在验证完成后清理，不作为产品文件保留。

参考：ParaView/VTK Ghost Cells Generator：
https://vtk.org/doc/nightly/html/classvtkGhostCellsGenerator.html
