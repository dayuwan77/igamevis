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

    virtual long long GetPointProcessId(IGsize index);
    virtual long long GetCellProcessId(IGsize index);

    LongLongArray::Pointer m_PointProcessIdArray{nullptr};
    LongLongArray::Pointer m_CellProcessIdArray{nullptr};

    int m_ProcessId{0};
    bool m_GeneratePointData{true};
    bool m_GenerateCellData{false};

    std::string m_Message;
};
IGAME_NAMESPACE_END
#endif
