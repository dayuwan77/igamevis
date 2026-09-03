#pragma once

#include <iGameFilter.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
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

    const std::vector<igIndex>& GetInvalidCellIds() const { return m_InvalidCellIds; }
    int GetInvalidCellCount() const { return static_cast<int>(m_InvalidCellIds.size()); }

protected:
    ValidateCellsFilter();
    ~ValidateCellsFilter() override = default;

private:
    std::vector<igIndex> m_InvalidCellIds;
};

IGAME_NAMESPACE_END
