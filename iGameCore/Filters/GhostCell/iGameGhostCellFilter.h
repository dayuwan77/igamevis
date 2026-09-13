#ifndef iGameGhostCellFilter_h
#define iGameGhostCellFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN
class GhostCellFilter : public Filter {
public:
    I_OBJECT(GhostCellFilter);
    static Pointer New() { return new GhostCellFilter; }

    // 输入的单元标记数组名（默认标准 vtkGhostType，必须是 Cell Data 上的单分量数组）
    void SetGhostArrayName(const std::string& name) { m_GhostArrayName = name; }
    const std::string& GetGhostArrayName() const { return m_GhostArrayName; }

    // 输出的单元掩码数组名（默认 GhostCellMask，取值 0/1）
    void SetOutputArrayName(const std::string& name) { m_OutputArrayName = name; }
    const std::string& GetOutputArrayName() const { return m_OutputArrayName; }

    // 选择要提取的 ghost 类型（可多选，命中任意一种即输出 1）
    void SetCheckAny(bool value) { m_CheckAny = value; }
    void SetCheckDuplicateCell(bool value) { m_CheckDuplicateCell = value; }
    void SetCheckHiddenCell(bool value) { m_CheckHiddenCell = value; }
    bool GetCheckAny() const { return m_CheckAny; }
    bool GetCheckDuplicateCell() const { return m_CheckDuplicateCell; }
    bool GetCheckHiddenCell() const { return m_CheckHiddenCell; }

    // 查找可用的单元标记数组下标，找不到返回 -1
    int FindGhostArrayIndex(DataObject::Pointer input) const;

    bool Execute() override;

protected:
    GhostCellFilter();
    ~GhostCellFilter() override = default;

private:
    // vtkGhostType 的标准按位取值
    enum GhostTypeBit { DUPLICATE_CELL = 1, HIDDEN_CELL = 2 };

    // 把 0/1 掩码写入输出网格的单元属性
    bool AttachMask(DataObject::Pointer output, IGsize cellCount, ArrayObject::Pointer src) const;

    std::string m_GhostArrayName{"vtkGhostType"};
    std::string m_OutputArrayName{"GhostCellMask"};
    bool m_CheckAny{true};
    bool m_CheckDuplicateCell{false};
    bool m_CheckHiddenCell{false};
};
IGAME_NAMESPACE_END
#endif