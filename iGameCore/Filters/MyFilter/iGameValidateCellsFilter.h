#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <array>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

// Validity state bitmask — matches VTK vtkCellValidator::State.
// A cell may have multiple issues; the value is a bitwise OR of these flags.
// 0 means Valid; non-zero means invalid.
enum ValidityState : unsigned short {
    Validity_Valid                       = 0x00,
    Validity_WrongNumberOfPoints         = 0x01,
    Validity_IntersectingEdges           = 0x02,
    Validity_IntersectingFaces           = 0x04,
    Validity_NoncontiguousEdges          = 0x08,
    Validity_Nonconvex                   = 0x10,
    Validity_FacesAreOrientedIncorrectly = 0x20,
    Validity_UnsupportedCellType         = 0x40
};

class ValidateCellsFilter : public Filter {
public:
    I_OBJECT(ValidateCellsFilter);
    static Pointer New() { return new ValidateCellsFilter; }

    bool Execute() override;

    /// 输出单元标量名称，挂载到独立输出节点上。
    static const char* ValidityStateArrayName() { return "ValidityState"; }

    /// 当前实现公开支持的单元类型。
    static bool IsSupportedCellType(IGenum cellType);
    static std::string GetSupportedCellTypesDescription();

    /// 将单个 ValidityState 位解析为人类可读名称。
    static std::string GetValidityFlagName(unsigned short flag);

    /// 将整个位掩码解析为“边相交、非凸”这样的可读文本。
    static std::string GetValidityStateText(unsigned short state);

    /// 每个被检查元素（UnstructuredMesh 的 cell，或 SurfaceMesh 的 face）的状态。
    const std::vector<unsigned short>& GetValidityStates() const { return m_ValidityStates; }

    /// 几何无效单元（不包含“未支持类型”）。
    const std::vector<igIndex>& GetInvalidCellIds() const { return m_InvalidCellIds; }

    /// 未支持类型单元。
    const std::vector<igIndex>& GetUnsupportedCellIds() const { return m_UnsupportedCellIds; }

    /// 命中某一位标志的单元，用于结果面板按类别高亮。
    const std::vector<igIndex>& GetCellIdsWithFlag(unsigned short flag) const;

    int GetCheckedCellCount() const { return static_cast<int>(m_ValidityStates.size()); }
    int GetInvalidCellCount() const { return static_cast<int>(m_InvalidCellIds.size()); }
    int GetUnsupportedCellCount() const { return static_cast<int>(m_UnsupportedCellIds.size()); }
    int GetValidCellCount() const {
        return GetCheckedCellCount() - GetInvalidCellCount() - GetUnsupportedCellCount();
    }

    const std::string& GetLastError() const { return m_LastError; }

protected:
    ValidateCellsFilter();
    ~ValidateCellsFilter() override = default;

private:
    void ResetResults(IGsize count);
    void RecordCell(IGsize cellId, IGenum cellType, unsigned short state);
    void ApplyResultAttribute(DataObject* output);

    std::vector<unsigned short> m_ValidityStates;
    std::array<std::vector<igIndex>, 7> m_CellIdsByFlag;
    std::vector<igIndex> m_InvalidCellIds;
    std::vector<igIndex> m_UnsupportedCellIds;
    std::string m_LastError;
};

IGAME_NAMESPACE_END
