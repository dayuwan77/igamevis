#include "iGameExtractEnclosedPointsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace
{

struct Tri {
    Vector3d v0, v1, v2;
};

using EdgeKey = std::pair<igIndex, igIndex>;

inline EdgeKey MakeEdgeKey(igIndex a, igIndex b) { return a < b ? EdgeKey(a, b) : EdgeKey(b, a); }

// 不调用 SurfaceMesh::BuildEdges()，避免污染输入。
bool IsClosedSurface(const SurfaceMesh::Pointer& surface) {
    if (surface == nullptr) return false;
    const IGsize nFaces = surface->GetNumberOfFaces();
    if (nFaces == 0) return false;

    CellArray* faces = surface->GetFaces();
    if (faces == nullptr) return false;

    std::map<EdgeKey, int> edgeCount;
    for (IGsize f = 0; f < nFaces; ++f) {
        const igIndex* ids = nullptr;
        const int n = faces->GetCellIds(f, ids);
        if (n < 3) continue;
        for (int i = 0; i < n; ++i) { ++edgeCount[MakeEdgeKey(ids[i], ids[(i + 1) % n])]; }
    }
    for (const auto& kv: edgeCount) {
        if (kv.second != 2) return false;
    }
    return true;
}

void BuildTriangles(const SurfaceMesh::Pointer& surface, std::vector<Tri>& outTris) {
    const IGsize nFaces = surface->GetNumberOfFaces();
    CellArray* faces = surface->GetFaces();
    outTris.clear();
    outTris.reserve(nFaces);

    for (IGsize f = 0; f < nFaces; ++f) {
        const igIndex* ids = nullptr;
        const int n = faces->GetCellIds(f, ids);
        if (n < 3) continue;

        const Point& p0 = surface->GetPoint(ids[0]);
        for (int i = 1; i + 1 < n; ++i) {
            const Point& p1 = surface->GetPoint(ids[i]);
            const Point& p2 = surface->GetPoint(ids[i + 1]);
            Tri tri;
            tri.v0 = Vector3d(p0[0], p0[1], p0[2]);
            tri.v1 = Vector3d(p1[0], p1[1], p1[2]);
            tri.v2 = Vector3d(p2[0], p2[1], p2[2]);
            outTris.push_back(tri);
        }
    }
}

bool RayTriangle(const Vector3d& orig, const Vector3d& dir, const Tri& tri, double tolerance, double& tOut) {
    constexpr double EPS = 1e-12;
    const Vector3d e1 = tri.v1 - tri.v0;
    const Vector3d e2 = tri.v2 - tri.v0;
    const Vector3d h = dir.cross(e2);
    const double a = e1.dot(h);
    if (std::abs(a) < EPS) return false;
    const double f = 1.0 / a;
    const Vector3d s = orig - tri.v0;
    const double u = f * s.dot(h);
    if (u < 0.0 || u > 1.0) return false;
    const Vector3d q = s.cross(e1);
    const double v = f * dir.dot(q);
    if (v < 0.0 || u + v > 1.0) return false;
    const double t = f * e2.dot(q);
    if (t <= tolerance) return false;
    tOut = t;
    return true;
}

bool IsPointInside(const Vector3d& p, const std::vector<Tri>& tris, double tolerance, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    int insideVotes = 0;
    const int attempts = 3;

    for (int k = 0; k < attempts; ++k) {
        Vector3d dir(dist(rng), dist(rng), dist(rng));
        const double len = dir.norm();
        if (len < 1e-6) continue;
        dir = dir / len;

        int hits = 0;
        for (const auto& tri: tris) {
            double t;
            if (RayTriangle(p, dir, tri, tolerance, t)) ++hits;
        }
        if (hits % 2 == 1) ++insideVotes;
    }
    return insideVotes > (attempts / 2);
}

ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& inArray) {
    if (inArray == nullptr) return nullptr;
    switch (inArray->GetArrayType()) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        default:
            return nullptr;
    }
}

void CopyPointAttributes(AttributeSet* inSet, AttributeSet* outSet, const std::vector<igIndex>& newToOld) {
    if (inSet == nullptr || outSet == nullptr || newToOld.empty()) return;

    auto inAttrs = inSet->GetAllPointAttributes();
    if (inAttrs == nullptr) return;

    const IGsize outNum = static_cast<IGsize>(newToOld.size());
    const size_t nAttrs = inAttrs->GetNumberOfElements();

    for (size_t i = 0; i < nAttrs; ++i) {
        auto& inAttr = inAttrs->GetElement(i);
        if (inAttr.IsNone() || inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inArray = inAttr.GetPointer();
        if (inArray == nullptr) continue;

        auto outArray = CreateArrayLike(inArray);
        if (outArray == nullptr) continue;

        outArray->SetName(inArray->GetName());
        outArray->SetDimension(inArray->GetDimension());
        outArray->Resize(outNum);

        const int dim = inArray->GetDimension();
        std::vector<double> tuple(static_cast<size_t>(dim), 0.0);
        const IGsize inNumElems = inArray->GetNumberOfElements();

        for (IGsize j = 0; j < outNum; ++j) {
            const igIndex src = newToOld[static_cast<size_t>(j)];
            if (src >= 0 && static_cast<IGsize>(src) < inNumElems) {
                inArray->GetElement(src, tuple.data());
            } else {
                std::fill(tuple.begin(), tuple.end(), 0.0);
            }
            outArray->SetElement(j, tuple.data());
        }

        outSet->AddAttribute(inAttr.GetType(), IG_POINT, outArray, inAttr.GetDataRange());
    }
}

} // namespace

bool ExtractEnclosedPointsFilter::Execute() {
    UpdateProgress(0.0);

    auto pointInput = DynamicCast<PointSet>(GetInput(0));
    auto surfaceInput = DynamicCast<SurfaceMesh>(GetInput(1));

    if (pointInput.IsNull()) {
        std::cerr << "[ExtractEnclosedPoints] Input 0 (point set) is invalid" << std::endl;
        return false;
    }
    if (surfaceInput.IsNull()) {
        std::cerr << "[ExtractEnclosedPoints] Input 1 (surface mesh) is invalid" << std::endl;
        return false;
    }
    if (surfaceInput->GetNumberOfFaces() == 0) {
        std::cerr << "[ExtractEnclosedPoints] Input surface has no face" << std::endl;
        return false;
    }

    if (m_CheckSurface && !IsClosedSurface(surfaceInput)) {
        std::cerr << "[ExtractEnclosedPoints] Surface is not closed; "
                     "output will be empty"
                  << std::endl;
        auto emptyOut = PointSet::New();
        emptyOut->SetName(pointInput->GetName() + "_enclosed");
        SetOutput(emptyOut);
        return true;
    }
    UpdateProgress(0.1);

    std::vector<Tri> tris;
    BuildTriangles(surfaceInput, tris);
    if (tris.empty()) {
        auto emptyOut = PointSet::New();
        emptyOut->SetName(pointInput->GetName() + "_enclosed");
        SetOutput(emptyOut);
        return true;
    }
    UpdateProgress(0.2);

    const IGsize nPts = pointInput->GetNumberOfPoints();
    auto outPoints = Points::New();
    outPoints->Reserve(nPts);

    std::vector<igIndex> newToOld;
    newToOld.reserve(static_cast<size_t>(nPts));

    std::mt19937 rng(12345);

    for (IGsize i = 0; i < nPts; ++i) {
        const Point& p = pointInput->GetPoint(i);
        const Vector3d pD(p[0], p[1], p[2]);

        const bool inside = IsPointInside(pD, tris, m_Tolerance, rng);
        const bool keep = m_InsideOut ? !inside : inside;

        if (keep) {
            outPoints->AddPoint(p);
            newToOld.push_back(static_cast<igIndex>(i));
        }

        if ((i & 0xFFF) == 0 && nPts > 0) {
            UpdateProgress(0.2 + 0.7 * static_cast<double>(i) / static_cast<double>(nPts));
        }
    }
    UpdateProgress(0.9);

    auto output = PointSet::New();
    output->SetPoints(outPoints);
    output->SetName(pointInput->GetName() + "_enclosed");

    CopyPointAttributes(pointInput->GetAttributeSet(), output->GetAttributeSet(), newToOld);

    UpdateProgress(1.0);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END