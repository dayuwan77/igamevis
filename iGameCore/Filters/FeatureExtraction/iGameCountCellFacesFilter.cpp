#include "iGameCountCellFacesFilter.h"
#include "iGameLagrangeUnstructuredMesh.h"
#include "iGameStructuredMesh.h"

#include <exception>

IGAME_NAMESPACE_BEGIN

namespace {
bool TryGetFixedFaceCount(IGenum cellType, unsigned int& faceCount) {
    switch (cellType) {
        case IG_EMPTY_CELL:
        case IG_VERTEX:
        case IG_LINE:
        case IG_POLY_LINE:
        case IG_FACE:
        case IG_TRIANGLE:
        case IG_QUAD:
        case IG_POLYGON:
        case IG_QUADRATIC_EDGE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_QUADRATIC_QUAD:
        case IG_QUADRATIC_POLYGON:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_CURVE:
        case IG_LAGRANGE_TRIANGLE:
        case IG_LAGRANGE_QUADRILATERAL:
            faceCount = 0;
            return true;
        case IG_TETRA:
        case IG_QUADRATIC_TETRA:
        case IG_LAGRANGE_TETRAHEDRON:
            faceCount = 4;
            return true;
        case IG_HEXAHEDRON:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
            faceCount = 6;
            return true;
        case IG_PYRAMID:
        case IG_QUADRATIC_PYRAMID:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_LAGRANGE_PYRAMID:
            faceCount = 5;
            return true;
        case IG_PRISM:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_LAGRANGE_PRISM:
            faceCount = 5;
            return true;
        default:
            return false;
    }
}

bool TryGetPolyhedronFaceCount(UnstructuredMesh* mesh, IGsize cellId, unsigned int& faceCount) {
    if (mesh == nullptr) return false;

    const igIndex* entries = nullptr;
    const int entryCount = mesh->GetCellPointIds(cellId, entries);
    if (entries == nullptr || entryCount <= 1 || entries[0] < 0) return false;

    const IGsize declaredFaceCount = static_cast<IGsize>(entries[0]);
    IGsize cursor = 1;
    IGsize parsedFaceCount = 0;
    while (cursor < static_cast<IGsize>(entryCount)) {
        const igIndex pointCount = entries[cursor++];
        if (pointCount < 3
            || static_cast<IGsize>(pointCount) > static_cast<IGsize>(entryCount) - cursor) {
            return false;
        }
        cursor += static_cast<IGsize>(pointCount);
        ++parsedFaceCount;
    }

    if (cursor != static_cast<IGsize>(entryCount)
        || (declaredFaceCount != 0 && declaredFaceCount != parsedFaceCount)) {
        return false;
    }
    faceCount = static_cast<unsigned int>(parsedFaceCount);
    return true;
}
} // namespace

bool CountCellFacesFilter::Execute() {
    try {
        return ExecuteInternal();
    } catch (const std::exception& exception) {
        m_FaceCounts = nullptr;
        m_Message = std::string("Exception while counting cell faces: ") + exception.what();
        igError("[CountCellFacesFilter] {}", m_Message);
    } catch (...) {
        m_FaceCounts = nullptr;
        m_Message = "Unknown exception while counting cell faces.";
        igError("[CountCellFacesFilter] {}", m_Message);
    }
    return false;
}

bool CountCellFacesFilter::ExecuteInternal() {
    m_FaceCounts = UnsignedIntArray::New();
    m_FaceCounts->SetName(ResultAttributeName);
    m_FaceCounts->SetDimension(1);

    auto input = GetInput(0);
    if (input.IsNull()) {
        m_Message = "No input data object.";
        igError("[CountCellFacesFilter] No input data object; Execute aborted.");
        return false;
    }

    IGsize cellCount = 0;
    IGsize unsupportedCellCount = 0;

    if (auto mesh = DynamicCast<StructuredMesh>(input); !mesh.IsNull()) {
        cellCount = mesh->GetNumberOfCells();
        m_FaceCounts->Resize(cellCount);
        const unsigned int faceCount = mesh->GetDimension() == 3 ? 6u : 0u;
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            m_FaceCounts->SetValue(cellId, faceCount);
        }
    } else if (auto mesh = DynamicCast<LagrangeUnstructuredMesh>(input); !mesh.IsNull()) {
        cellCount = mesh->GetNumberOfCells();
        m_FaceCounts->Resize(cellCount);
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            unsigned int faceCount = 0;
            if (!TryGetFixedFaceCount(mesh->GetSpecificCellType(cellId), faceCount)) {
                ++unsupportedCellCount;
                m_FaceCounts->SetValue(cellId, 0);
                continue;
            }
            m_FaceCounts->SetValue(cellId, faceCount);
        }
    } else if (auto mesh = DynamicCast<UnstructuredMesh>(input); !mesh.IsNull()) {
        cellCount = mesh->GetNumberOfCells();
        auto cells = mesh->GetCells();
        auto* cellTypes = mesh->GetCellTypes();
        if (cellCount > 0 && (cells.IsNull() || cellTypes == nullptr
                             || cellTypes->GetNumberOfValues() < cellCount)) {
            m_Message = "UnstructuredMesh has inconsistent cell connectivity or type arrays.";
            igError("[CountCellFacesFilter] {}", m_Message);
            return false;
        }
        m_FaceCounts->Resize(cellCount);
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            const IGenum cellType = mesh->GetCellType(cellId);
            unsigned int faceCount = 0;
            const bool supported = cellType == IG_POLYHEDRON
                    ? TryGetPolyhedronFaceCount(mesh.GetPointer(), cellId, faceCount)
                    : TryGetFixedFaceCount(cellType, faceCount);
            if (!supported) {
                ++unsupportedCellCount;
                m_FaceCounts->SetValue(cellId, 0);
                continue;
            }
            m_FaceCounts->SetValue(cellId, faceCount);
        }
    } else if (auto mesh = DynamicCast<VolumeMesh>(input); !mesh.IsNull()) {
        cellCount = mesh->GetNumberOfVolumes();
        auto* volumes = mesh->GetVolumes();
        if (cellCount > 0 && volumes == nullptr) {
            m_Message = "VolumeMesh has no volume connectivity array.";
            igError("[CountCellFacesFilter] {}", m_Message);
            return false;
        }
        m_FaceCounts->Resize(cellCount);
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            if (mesh->GetIsPolyhedronType()) {
                igIndex faceIds[IGAME_CELL_MAX_SIZE]{};
                const int faceCount = mesh->GetVolumeFaceIds(cellId, faceIds);
                if (faceCount > 0) {
                    m_FaceCounts->SetValue(cellId, static_cast<unsigned int>(faceCount));
                    continue;
                }
            } else {
                const IGuint pointCount = volumes->GetCellSize(cellId);
                unsigned int faceCount = 0;
                bool supported = true;
                switch (pointCount) {
                    case 4: faceCount = 4; break; // tetrahedron
                    case 5: faceCount = 5; break; // pyramid
                    case 6: faceCount = 5; break; // prism
                    case 8: faceCount = 6; break; // hexahedron
                    default: supported = false; break;
                }
                if (supported) {
                    m_FaceCounts->SetValue(cellId, faceCount);
                    continue;
                }
            }

            ++unsupportedCellCount;
            m_FaceCounts->SetValue(cellId, 0);
        }
    } else if (auto mesh = DynamicCast<SurfaceMesh>(input); !mesh.IsNull()) {
        cellCount = mesh->GetNumberOfFaces();
        m_FaceCounts->Resize(cellCount);
        for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
            // A surface cell is already a 2D face and has no 3D sub-faces.
            m_FaceCounts->SetValue(cellId, 0);
        }
    } else {
        m_Message = "Unsupported mesh type: " + std::to_string(input->GetDataObjectType());
        igDebug("[CountCellFacesFilter] {}", m_Message);
        return false;
    }

    if (unsupportedCellCount > 0) {
        igDebug("[CountCellFacesFilter] {} cells use unsupported cell types and were assigned 0.",
                unsupportedCellCount);
    }

    auto* newAttrs = input->GetAttributeSet();
    if (!newAttrs) {
        m_Message = "Input mesh has no attribute set.";
        igError("[CountCellFacesFilter] {}", m_Message);
        return false;
    }

    const int oldAttributeIndex = newAttrs->GetAttributeIndex(ResultAttributeName);
    if (oldAttributeIndex >= 0) {
        newAttrs->DeleteAttribute(static_cast<IGsize>(oldAttributeIndex));
    }
    newAttrs->AddScalar(IG_CELL, m_FaceCounts);
    if (newAttrs->GetAttributeIndex(ResultAttributeName) < 0) {
        m_Message = std::string("Failed to add IG_CELL attribute '")
                    + ResultAttributeName + "'.";
        igError("[CountCellFacesFilter] {}", m_Message);
        return false;
    }
    newAttrs->ForceReConvertToDrawableData();

    SetOutput(input);
    m_Message = std::string("IG_CELL attribute '") + ResultAttributeName +
                "' computed: " + std::to_string(cellCount);
    igDebug("[CountCellFacesFilter] {}", m_Message);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
