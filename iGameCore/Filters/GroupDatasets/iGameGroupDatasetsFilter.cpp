#include "iGameGroupDatasetsFilter.h"

IGAME_NAMESPACE_BEGIN

GroupDatasetsFilter::GroupDatasetsFilter() {
    // 输入个数由 SetInputs() 动态决定，这里先置 0；
    // 输出恒为 1 个（多块组本身）。基类 SetOutput 有边界检查，
    // 若不预留输出槽，SetOutput(0, ...) 会静默失败。
    this->SetNumberOfInputs(0);
    this->SetNumberOfOutputs(1);
}

void GroupDatasetsFilter::SetInputs(const std::vector<DataObject::Pointer>& objs) {
    // 先过滤掉空指针，保证输入数组里全是有效对象。
    std::vector<DataObject::Pointer> valid;
    valid.reserve(objs.size());
    for (const auto& obj: objs) {
        if (obj) { valid.push_back(obj); }
    }

    this->SetNumberOfInputs(static_cast<int>(valid.size()));
    for (int i = 0; i < static_cast<int>(valid.size()); ++i) {
        this->SetInput(i, valid[i]);
    }
}

bool GroupDatasetsFilter::Execute() {
    m_Message.clear();
    m_GroupedCount = 0;
    m_BlockNames.clear();

    const int inputCount = this->GetNumberOfInputs();
    if (inputCount <= 0) {
        m_Message = "没有输入对象：请先在模型树中选中至少一个对象（可按住 Ctrl / Shift 多选）。";
        return false;
    }

    // 多块复合容器必须用 DrawObject 创建：
    // 该对象最终会交给 Scene 渲染，Scene 会把它当作 DrawObject 使用，
    // 若用裸 DataObject::New()，后续 DynamicCast<DrawObject> 会失败。
    auto group = DrawObject::New();
    if (!group) {
        m_Message = "创建多块组失败（内存分配失败）。";
        return false;
    }
    if (!m_OutputName.empty()) { group->SetName(m_OutputName); }

    for (int i = 0; i < inputCount; ++i) {
        auto obj = this->GetInput(i);
        if (!obj) { continue; } // 防御：基类已做边界检查，这里再挡一次空指针

        // 只做容器组合：子块直接共享输入的 DataObject 指针，不复制几何、不改名。
        group->AddSubDataObject(obj);

        m_BlockNames.push_back(obj->GetName());
        ++m_GroupedCount;
    }

    if (m_GroupedCount == 0) {
        m_Message = "选中的对象全部不可用，未生成多块组。";
        return false;
    }

    this->SetOutput(0, group);
    return true;
}

IGAME_NAMESPACE_END
