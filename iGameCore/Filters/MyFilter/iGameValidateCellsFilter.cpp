#include "iGameValidateCellsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameFlatArray.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kTol = 1.1920929e-7;
constexpr double kAreaTol = 1e-30;

inline bool PointsAreCoincident(const Point& a, const Point& b, double tol) {
    return std::abs(static_cast<double>(a[0] - b[0])) < tol &&
           std::abs(static_cast<double>(a[1] - b[1])) < tol &&
           std::abs(static_cast<double>(a[2] - b[2])) < tol;
}

inline bool HasDuplicatePoints(const std::vector<Point>& pts, double tol) {
    for (size_t i = 0; i < pts.size(); ++i) {
        for (size_t j = i + 1; j < pts.size(); ++j) {
            if ((pts[i] - pts[j]).norm() < tol) {
                return true;
            }
        }
    }
    return false;
}

inline double TriangleArea2Sq(const Point& a, const Point& b, const Point& c) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f n = CrossProduct(ab, ac);
    return static_cast<double>(DotProduct(n, n));
}

inline double TetraSignedVolume(const Point& a, const Point& b, const Point& c, const Point& d) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ad = d - a;
    const Vector3f cr = CrossProduct(ac, ad);
    return static_cast<double>(DotProduct(ab, cr)) / 6.0;
}

bool SegmentsIntersect(const Point& p1, const Point& p2,
                      const Point& q1, const Point& q2, double tol) {
    Vector3f u = p2 - p1;
    Vector3f v = q2 - q1;
    Vector3f w = p1 - q1;

    double uu = static_cast<double>(DotProduct(u, u));
    double vv = static_cast<double>(DotProduct(v, v));
    double uv = static_cast<double>(DotProduct(u, v));
    double uw = static_cast<double>(DotProduct(u, w));
    double vw = static_cast<double>(DotProduct(v, w));
    double denom = uu * vv - uv * uv;

    if (std::abs(denom) < 1e-30) {
        return false;
    }

    double s = (uv * vw - vv * uw) / denom;
    double t = (uu * vw - uv * uw) / denom;

    if (s < -tol || s > 1.0 + tol || t < -tol || t > 1.0 + tol) {
        return false;
    }

    s = std::clamp(s, 0.0, 1.0);
    t = std::clamp(t, 0.0, 1.0);

    const bool sInterior = std::abs(s) > tol && std::abs(s - 1.0) > tol;
    const bool tInterior = std::abs(t) > tol && std::abs(t - 1.0) > tol;
    return sInterior || tInterior;
}

unsigned short ValidateLine(const std::vector<Point>& pts) {
    if (pts.size() != 2) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (PointsAreCoincident(pts[0], pts[1], kTol)) {
        state |= Validity_Nonconvex;
    }
    return state;
}

unsigned short ValidateTriangle(const std::vector<Point>& pts) {
    if (pts.size() != 3) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, kTol)) {
        state |= Validity_Nonconvex;
    }
    if (TriangleArea2Sq(pts[0], pts[1], pts[2]) < kAreaTol) {
        state |= Validity_Nonconvex;
    }
    return state;
}

unsigned short ValidateQuad(const std::vector<Point>& pts) {
    if (pts.size() != 4) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, kTol)) {
        state |= Validity_Nonconvex;
    }

    const int edges[4][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (i == 3 && j == 0) {
                continue;
            }
            if (SegmentsIntersect(pts[edges[i][0]], pts[edges[i][1]],
                                  pts[edges[j][0]], pts[edges[j][1]], kTol)) {
                state |= Validity_IntersectingEdges;
                break;
            }
        }
        if ((state & Validity_IntersectingEdges) != 0) {
            break;
        }
    }

    return state;
}

unsigned short ValidatePolygon(const std::vector<Point>& pts) {
    const int n = static_cast<int>(pts.size());
    if (n < 3) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, kTol)) {
        state |= Validity_Nonconvex;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (j == i + 1 || (i == 0 && j == n - 1)) {
                continue;
            }

            if (SegmentsIntersect(pts[i], pts[(i + 1) % n],
                                  pts[j], pts[(j + 1) % n], kTol)) {
                state |= Validity_IntersectingEdges;
                break;
            }
        }
        if ((state & Validity_IntersectingEdges) != 0) {
            break;
        }
    }

    return state;
}

unsigned short ValidateTetra(const std::vector<Point>& pts) {
    if (pts.size() != 4) {
        return Validity_WrongNumberOfPoints;
    }

    unsigned short state = Validity_Valid;
    if (HasDuplicatePoints(pts, kTol)) {
        state |= Validity_Nonconvex;
    }

    const double volume = TetraSignedVolume(pts[0], pts[1], pts[2], pts[3]);
    if (std::abs(volume) < kTol) {
        state |= Validity_Nonconvex;
    } else if (volume < 0.0) {
        state |= Validity_FacesAreOrientedIncorrectly;
    }
    return state;
}

unsigned short ValidateCellState(int cellType, const std::vector<Point>& pts) {
    switch (cellType) {
        case IG_EMPTY_CELL:
            return Validity_WrongNumberOfPoints;

        case IG_VERTEX:
            return (pts.size() == 1) ? Validity_Valid : Validity_WrongNumberOfPoints;

        case IG_LINE:
        case IG_POLY_LINE:
        case IG_QUADRATIC_EDGE:
            return ValidateLine(pts);

        case IG_TRIANGLE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
            return ValidateTriangle(pts);

        case IG_QUAD:
        case IG_QUADRATIC_QUAD:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
            return ValidateQuad(pts);

        case IG_POLYGON:
        case IG_FACE:
        case IG_QUADRATIC_POLYGON:
            return ValidatePolygon(pts);

        case IG_TETRA:
        case IG_QUADRATIC_TETRA:
            return ValidateTetra(pts);

        default:
            return Validity_UnsupportedCellType;
    }
}

std::vector<Point> GetCellPoints(const UnstructuredMesh::Pointer& mesh, IGsize cellId,
                                const IdArray::Pointer& ids) {
    ids->Reset();
    mesh->GetCellPointIds(cellId, ids);

    std::vector<Point> pts;
    pts.reserve(static_cast<size_t>(ids->GetNumberOfIds()));
    for (int i = 0; i < ids->GetNumberOfIds(); ++i) {
        pts.push_back(mesh->GetPoint(ids->GetId(i)));
    }
    return pts;
}

std::vector<Point> GetFacePoints(const SurfaceMesh::Pointer& mesh, IGsize faceId) {
    const int nPts = mesh->GetNumberOfPoints();
    if (nPts <= 0) {
        return {};
    }

    std::vector<igIndex> ids(static_cast<size_t>(nPts));
    const int nFacePts = mesh->GetFacePointIds(faceId, ids.data());
    if (nFacePts <= 0) {
        return {};
    }

    ids.resize(static_cast<size_t>(nFacePts));
    std::vector<Point> pts;
    pts.reserve(ids.size());

    for (igIndex id : ids) {
        pts.push_back(mesh->GetPoint(id));
    }
    return pts;
}

void RemoveExistingValidityState(DataObject* data) {
    if (data == nullptr) {
        return;
    }

    if (auto attrs = data->GetAttributeSet()) {
        while (true) {
            const int idx = attrs->GetAttributeIndex("ValidityState");
            if (idx < 0) {
                break;
            }
            attrs->DeleteAttribute(idx);
        }
    }
}

void AddValidityStateAttribute(DataObject* data, IntArray::Pointer stateArray) {
    if (data == nullptr || stateArray == nullptr) {
        return;
    }

    if (auto attrs = data->GetAttributeSet()) {
        RemoveExistingValidityState(data);
        stateArray->Modified();
        attrs->AddAttribute(IG_SCALAR, IG_CELL, stateArray);
        attrs->Modified();
    }
}

}  // namespace

ValidateCellsFilter::ValidateCellsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ValidateCellsFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) {
        return false;
    }

    m_InvalidCellIds.clear();

    if (auto mesh = DynamicCast<UnstructuredMesh>(input)) {
        const IGsize nCells = mesh->GetNumberOfCells();
        IntArray::Pointer stateArray = IntArray::New();
        stateArray->SetDimension(1);
        stateArray->Resize(static_cast<igIndex>(nCells));
        stateArray->SetName("ValidityState");

        IdArray::Pointer ids = IdArray::New();

        for (IGsize cellId = 0; cellId < nCells; ++cellId) {
            const std::vector<Point> pts = GetCellPoints(mesh, cellId, ids);
            const unsigned short state = ValidateCellState(mesh->GetCellType(cellId), pts);
            stateArray->SetValue(static_cast<igIndex>(cellId), static_cast<int>(state));
            if (state != Validity_Valid) {
                m_InvalidCellIds.push_back(static_cast<igIndex>(cellId));
            }
        }

        RemoveExistingValidityState(mesh);
        AddValidityStateAttribute(mesh, stateArray);
        mesh->Modified();

        if (m_Model != nullptr && m_Model->GetDataObject() == input) {
            m_Model->Modified();
        }

    } else if (auto mesh = DynamicCast<SurfaceMesh>(input)) {
        const IGsize nFaces = mesh->GetNumberOfFaces();
        IntArray::Pointer stateArray = IntArray::New();
        stateArray->SetDimension(1);
        stateArray->Resize(static_cast<igIndex>(nFaces));
        stateArray->SetName("ValidityState");

        for (IGsize faceId = 0; faceId < nFaces; ++faceId) {
            const std::vector<Point> pts = GetFacePoints(mesh, faceId);
            const unsigned short state = ValidatePolygon(pts);
            stateArray->SetValue(static_cast<igIndex>(faceId), static_cast<int>(state));
            if (state != Validity_Valid) {
                m_InvalidCellIds.push_back(static_cast<igIndex>(faceId));
            }
        }

        RemoveExistingValidityState(mesh);
        AddValidityStateAttribute(mesh, stateArray);
        mesh->Modified();

        if (m_Model != nullptr && m_Model->GetDataObject() == input) {
            m_Model->Modified();
        }

    } else {
        return false;
    }

    SetOutput(0, input);

    if (m_Model != nullptr && !m_InvalidCellIds.empty()) {
        auto selection = m_Model->GetSelection();
        if (selection != nullptr) {
            selection->ClearSelections();
            selection->SelectionCallBackEvent(IG_CELL, m_InvalidCellIds,
                                              Selection::Operate::Add);
        }
    }

    return true;
}

IGAME_NAMESPACE_END
