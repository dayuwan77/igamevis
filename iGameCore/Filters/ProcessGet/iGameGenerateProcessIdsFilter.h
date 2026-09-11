#pragma once
#ifndef EOIGAME_IGAMECORE_PROCESSGET_IGAMEGENERATEPROCESSIDSFILTER_H
#define EOIGAME_IGAMECORE_PROCESSGET_IGAMEGENERATEPROCESSIDSFILTER_H

#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGameModel.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>

#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @class   GenerateProcessIdsFilter
 * @brief   为网格生成进程号数组（PointProcessIds / CellProcessIds）
 *
 * 输出为新的数据对象：几何（点、单元）与输入共享，属性集为输入的拷贝加上新生成的
 * 进程号数组，输入对象本身不被修改；GUI 会把结果作为独立节点加入模型树。
 * 进程号取值优先沿用输入上同名的 process_id(LongLong) 数组，否则使用常数 SetProcessId()。
 */
class GenerateProcessIdsFilter : public Filter {
public:
    I_OBJECT(GenerateProcessIdsFilter)

    static Pointer New() { return new GenerateProcessIdsFilter; }

    bool Execute() override;

    void SetGeneratePointData(bool b) { m_GeneratePointData = b; }
    bool GetGeneratePointData() const { return m_GeneratePointData; }

    void SetGenerateCellData(bool b) { m_GenerateCellData = b; }
    bool GetGenerateCellData() const { return m_GenerateCellData; }

    void SetProcessId(int pid) { m_ProcessId = pid; }
    int GetProcessId() const { return m_ProcessId; }

    const std::string& GetMessage() const { return m_Message; }

protected:
    GenerateProcessIdsFilter();
    ~GenerateProcessIdsFilter() override = default;

    // 返回输入网格的单元总数；网格类型不支持单元时返回 false（如普通 PointSet）。
    bool GetCellCount(PointSet* mesh, IGsize& cellCount);

    // 进程号取值：优先沿用输入上的 process_id 数组，否则使用常数 m_ProcessId。
    // 派生类可重写这两个方法实现自定义分区策略。
    virtual long long GetPointProcessId(IGsize index);
    virtual long long GetCellProcessId(IGsize index);

    // 输入上已有的 process_id 数组（只读来源，输入不被修改）
    LongLongArray::Pointer m_InputPointProcessIdArray{nullptr};
    LongLongArray::Pointer m_InputCellProcessIdArray{nullptr};

    int m_ProcessId{0};
    bool m_GeneratePointData{true};
    bool m_GenerateCellData{false};

    std::string m_Message;
};

IGAME_NAMESPACE_END
#endif
