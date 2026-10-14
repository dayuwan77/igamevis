#ifndef iGameGhostCellFilter_h
#define iGameGhostCellFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class GhostCellFilter : public Filter {
public:
    I_OBJECT(GhostCellFilter);
    static Pointer New() { return new GhostCellFilter; }

    /* 输入的单元标记数组名，默认 vtkGhostType（必须是单元上的单分量数组） */
    void SetGhostArrayName(const std::string& name);
    const std::string& GetGhostArrayName() const { return m_GhostArrayName; }

    /* 输出的单元掩码数组名，默认 GhostCellMask（取值 0 / 1） */
    void SetOutputArrayName(const std::string& name);
    const std::string& GetOutputArrayName() const { return m_OutputArrayName; }

    /* ghost 类型选择（可多选，"或"关系；默认只勾"任意"） */
    void SetCheckAny(bool value) { m_CheckAny = value; }
    void SetCheckDuplicateCell(bool value) { m_CheckDuplicateCell = value; }
    void SetCheckRefinedCell(bool value) { m_CheckRefinedCell = value; }
    void SetCheckExteriorCell(bool value) { m_CheckExteriorCell = value; }
    void SetCheckHiddenCell(bool value) { m_CheckHiddenCell = value; }

    bool GetCheckAny() const { return m_CheckAny; }
    bool GetCheckDuplicateCell() const { return m_CheckDuplicateCell; }
    bool GetCheckRefinedCell() const { return m_CheckRefinedCell; }
    bool GetCheckExteriorCell() const { return m_CheckExteriorCell; }
    bool GetCheckHiddenCell() const { return m_CheckHiddenCell; }

    /* 当前勾选的类型位掩码 */
    unsigned int GetGhostTypeMask() const;

    /* 查找可用的单元标记数组下标，找不到返回 -1 */
    int FindGhostArrayIndex(DataObject::Pointer input) const;

    bool Execute() override;

protected:
    GhostCellFilter();
    ~GhostCellFilter() override = default;

private:
    /* VTK / vtkDataSetAttributes 的 ghost 位标志 */
    enum GhostTypeBit {
        DUPLICATE_CELL = 1,
        REFINED_CELL = 8,
        EXTERIOR_CELL = 16,
        HIDDEN_CELL = 32,
    };

    static ArrayObject::Pointer CloneArray(ArrayObject::Pointer source);

    /* 把输入的属性集完整复制到输出（跳过空 / 已删除的属性，避免留下空指针占位） */
    static bool CopyAttributes(DataObject::Pointer output, DataObject::Pointer input);

    /* 把 0 / 1 掩码写到输出的单元属性上（同名 + 同附着类型则原地替换） */
    bool AttachMask(DataObject::Pointer output, IGsize cellCount, ArrayObject::Pointer ghostArray) const;

    std::string m_GhostArrayName{"vtkGhostType"};
    std::string m_OutputArrayName{"GhostCellMask"};
    bool m_CheckAny{true};
    bool m_CheckDuplicateCell{false};
    bool m_CheckRefinedCell{false};
    bool m_CheckExteriorCell{false};
    bool m_CheckHiddenCell{false};
};

IGAME_NAMESPACE_END
#endif