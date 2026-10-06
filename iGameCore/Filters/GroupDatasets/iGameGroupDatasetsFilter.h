#ifndef iGameGroupDatasetsFilter_h
#define iGameGroupDatasetsFilter_h

#include "iGameFilter.h"
#include "iGameDrawObject.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * GroupDatasetsFilter —— 把 N 个数据对象打包成一个多块复合组（MultiBlock）
 *
 * 对齐 ParaView 的 Group Datasets（vtkGroupDataSetsFilter）语义：
 *
 *   1. 只做【容器组合】，**不合并几何** —— 每个输入成为输出多块组的一个子块，
 *      子块直接共享输入的 DataObject 指针（与 ParaView 的 ShallowCopy 一致）。
 *      这与 AppendReduceFilter（Append Datasets，真正合并点/单元）是两回事。
 *   2. 允许**混合类型**输入：体网格 / 表面网格 / 点云可以放进同一个多块组。
 *   3. 输入本身若已是多块组，则**嵌套**保留其结构，不做展平 ——
 *      因此输出的【顶层块数】恒等于【输入对象数】。
 *   4. 输出的每个子块**不改名**：多块子对象按 `map<id, 指针>` 存储，
 *      没有独立的“块名”槽位，改块名就等于改输入对象本身，
 *      与“原始模型零修改”原则冲突。重名子块在 UI 报告中按序号编号区分。
 *
 * 用法：
 * @code
 *   auto filter = GroupDatasetsFilter::New();
 *   filter->SetInputs(selectedObjects);      // 选中的 DataObject 列表
 *   filter->SetOutputName("GroupDatasets_1");// 由上层算好的唯一名
 *   if (!filter->Execute()) { 用 filter->GetMessage() 提示用户 }
 *   auto group = filter->GetOutput(0);       // 多块组
 * @endcode
 */
class GroupDatasetsFilter : public Filter {
public:
    I_OBJECT(GroupDatasetsFilter);
    static Pointer New() { return new GroupDatasetsFilter; }

    /// 执行：把已设置的输入打包成多块组，写入输出槽 0。
    /// 返回 false 时用 GetMessage() 取失败原因（供 UI 弹窗）。
    bool Execute() override;

    /// 一次性设置全部输入（内部会先 SetNumberOfInputs 再逐个 SetInput）。
    /// 传入的 nullptr 会被跳过，不计入输入数。
    void SetInputs(const std::vector<DataObject::Pointer>& objs);

    /// 结果组节点名（由上层决定，通常是 UI 层算出的唯一名）。
    void SetOutputName(const std::string& name) { m_OutputName = name; }
    const std::string& GetOutputName() const { return m_OutputName; }

    /// 失败/提示信息（UTF-8，供 UI 直接显示）。
    const std::string& GetMessage() const { return m_Message; }

    /// 实际合并进组的数据对象个数。
    int GetGroupedCount() const { return m_GroupedCount; }

    /// 各子块的名字（按加入顺序，取自输入对象的 GetName()，可能重复）。
    const std::vector<std::string>& GetBlockNames() const { return m_BlockNames; }

protected:
    GroupDatasetsFilter();
    ~GroupDatasetsFilter() override = default;

private:
    std::string m_OutputName;
    std::string m_Message;
    int m_GroupedCount{0};
    std::vector<std::string> m_BlockNames;

    // 注意：输入数组沿用基类 Filter::m_Inputs（ElementArray<DataObject::Pointer>），
    //       不要在这里另建同名成员，否则会遮蔽基类成员。
};

IGAME_NAMESPACE_END
#endif
