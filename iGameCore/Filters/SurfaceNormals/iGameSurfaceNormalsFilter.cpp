#include "iGameSurfaceNormalsFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGameIdArray.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

constexpr double kPi = 3.14159265358979323846;

struct EdgeKey {
    igIndex a;
    igIndex b;

    EdgeKey(igIndex x, igIndex y) : a(std::min(x, y)), b(std::max(x, y)) {}

    bool operator<(const EdgeKey& other) const {
        if (a != other.a) return a < other.a;
        return b < other.b;
    }
};

struct EdgeUse {
    int face;
    igIndex u;
    igIndex v;
};

std::vector<igIndex> ReadFacePointIds(SurfaceMesh* mesh, int faceId) {
    IdArray::Pointer ids = IdArray::New();
    mesh->GetFaces()->GetCellIds(faceId, ids);

    std::vector<igIndex> result;
    const int count = static_cast<int>(ids->GetNumberOfIds());
    const int pointCount = mesh->GetNumberOfPoints();
    result.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        const igIndex id = ids->GetId(i);
        if (id < 0 || static_cast<IGsize>(id) >= pointCount) {
            continue;
        }
        result.push_back(id);
    }
    return result;
}

std::vector<igIndex> CleanFacePointIds(const std::vector<igIndex>& raw) {
    std::vector<igIndex> result;
    result.reserve(raw.size());

    for (igIndex id : raw) {
        if (result.empty() || result.back() != id) {
            result.push_back(id);
        }
    }

    if (result.size() > 1 && result.front() == result.back()) {
        result.pop_back();
    }
    return result;
}

// Newell 方法计算多边形面法向量，并归一化。退化面返回零向量。
Vector3f ComputeFaceNormal(SurfaceMesh* mesh, const std::vector<igIndex>& ptIds,
                           bool& degenerate) {
    if (ptIds.size() < 3) {
        degenerate = true;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    double nx = 0.0, ny = 0.0, nz = 0.0;
    const int npts = static_cast<int>(ptIds.size());
    for (int i = 0; i < npts; ++i) {
        const int j = (i + 1) % npts;
        const Point& a = mesh->GetPoint(ptIds[i]);
        const Point& b = mesh->GetPoint(ptIds[j]);
        nx += static_cast<double>(a[1] - b[1]) * static_cast<double>(a[2] + b[2]);
        ny += static_cast<double>(a[2] - b[2]) * static_cast<double>(a[0] + b[0]);
        nz += static_cast<double>(a[0] - b[0]) * static_cast<double>(a[1] + b[1]);
    }

    Vector3f n(static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz));
    const float len = static_cast<float>(n.norm());
    if (len <= 1e-30f) {
        degenerate = true;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    const float invLen = 1.0f / len;
    degenerate = false;
    return Vector3f(n[0] * invLen, n[1] * invLen, n[2] * invLen);
}

float Dot(const Vector3f& a, const Vector3f& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void ReverseFace(std::vector<igIndex>& ids) {
    std::reverse(ids.begin(), ids.end());
}

// 复制输入网格的全部单元属性到输出。
void CopyCellAttributes(AttributeSet* src, AttributeSet* dst, int nFaces) {
    if (src == nullptr || dst == nullptr) return;
    auto cellAttrs = src->GetAllCellAttributes();
    if (cellAttrs == nullptr) return;

    for (int i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
        AttributeSet::Attribute& inAttr = cellAttrs->GetElement(i);
        if (inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inData = inAttr.GetPointer();
        if (inData == nullptr) continue;

        const std::string& name = inData->GetName();
        if (name == "Normals" || name == "Normals_Magnitude") continue;

        const int dim = std::max(1, inData->GetDimension());
        DoubleArray::Pointer newData = DoubleArray::New();
        newData->SetName(name);
        newData->SetDimension(dim);
        newData->Resize(nFaces);

        std::vector<double> tmp(static_cast<size_t>(dim));
        for (int j = 0; j < nFaces; ++j) {
            inData->GetElement(j, tmp.data());
            newData->SetElement(j, tmp.data());
        }

        dst->AddAttribute(inAttr.GetType(), IG_CELL, newData, inAttr.GetDataRange());
    }
}

// 复制输入网格的全部点属性到输出。
void CopyPointAttributes(AttributeSet* src, AttributeSet* dst,
                         const std::vector<int>& newPtOrigId, int newPtCount) {
    if (src == nullptr || dst == nullptr) return;
    auto pointAttrs = src->GetAllPointAttributes();
    if (pointAttrs == nullptr) return;

    for (int i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        AttributeSet::Attribute& inAttr = pointAttrs->GetElement(i);
        if (inAttr.IsDeleted()) continue;

        ArrayObject::Pointer inData = inAttr.GetPointer();
        if (inData == nullptr) continue;

        const std::string& name = inData->GetName();
        if (name == "Normals" || name == "Normals_Magnitude") continue;

        const int dim = std::max(1, inData->GetDimension());
        DoubleArray::Pointer newData = DoubleArray::New();
        newData->SetName(name);
        newData->SetDimension(dim);
        newData->Resize(newPtCount);

        std::vector<double> tmp(static_cast<size_t>(dim));
        for (int j = 0; j < newPtCount; ++j) {
            const int orig = newPtOrigId[static_cast<size_t>(j)];
            inData->GetElement(orig, tmp.data());
            newData->SetElement(j, tmp.data());
        }

        dst->AddAttribute(inAttr.GetType(), IG_POINT, newData, inAttr.GetDataRange());
    }
}

}  // namespace

SurfaceNormalsFilter::SurfaceNormalsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool SurfaceNormalsFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    auto mesh = DynamicCast<SurfaceMesh>(input);
    if (mesh == nullptr) return false;

    const int nFaces = mesh->GetNumberOfFaces();
    const int nPoints = mesh->GetNumberOfPoints();
    if (nFaces == 0 || nPoints == 0) return false;

    // ---------------------------------------------------------------
    // 1. 读取每个面的原始点序，并生成去除重复点后的计算点序。
    // ---------------------------------------------------------------
    std::vector<std::vector<igIndex>> rawFaceIds(static_cast<size_t>(nFaces));
    std::vector<std::vector<igIndex>> cleanFaceIds(static_cast<size_t>(nFaces));
    std::vector<Vector3f> faceNormals(static_cast<size_t>(nFaces), Vector3f(0.0f, 0.0f, 0.0f));
    std::vector<bool> degenerate(static_cast<size_t>(nFaces), false);

    for (int faceId = 0; faceId < nFaces; ++faceId) {
        rawFaceIds[static_cast<size_t>(faceId)] = ReadFacePointIds(mesh, faceId);
        cleanFaceIds[static_cast<size_t>(faceId)] =
                CleanFacePointIds(rawFaceIds[static_cast<size_t>(faceId)]);
        bool isDegenerate = false;
        faceNormals[static_cast<size_t>(faceId)] =
                ComputeFaceNormal(mesh, cleanFaceIds[static_cast<size_t>(faceId)],
                                  isDegenerate);
        degenerate[static_cast<size_t>(faceId)] = isDegenerate;
    }

    // ---------------------------------------------------------------
    // 2. 建立无向边到有向半边使用记录的映射。
    // ---------------------------------------------------------------
    std::map<EdgeKey, std::vector<EdgeUse>> edgeUses;
    for (int faceId = 0; faceId < nFaces; ++faceId) {
        if (degenerate[static_cast<size_t>(faceId)]) continue;

        const auto& pts = cleanFaceIds[static_cast<size_t>(faceId)];
        const int m = static_cast<int>(pts.size());
        for (int i = 0; i < m; ++i) {
            const igIndex u = pts[static_cast<size_t>(i)];
            const igIndex v = pts[static_cast<size_t>((i + 1) % m)];
            edgeUses[EdgeKey(u, v)].push_back(EdgeUse{faceId, u, v});
        }
    }

    // ---------------------------------------------------------------
    // 3. Consistency：统一相邻面的环绕方向。
    // ---------------------------------------------------------------
    if (m_Consistency) {
        std::vector<char> oriented(static_cast<size_t>(nFaces), 0);

        for (int seed = 0; seed < nFaces; ++seed) {
            if (oriented[static_cast<size_t>(seed)] || degenerate[static_cast<size_t>(seed)]) {
                continue;
            }

            std::queue<int> q;
            q.push(seed);
            oriented[static_cast<size_t>(seed)] = 1;

            while (!q.empty()) {
                const int curFace = q.front();
                q.pop();

                const auto& pts = cleanFaceIds[static_cast<size_t>(curFace)];
                const int m = static_cast<int>(pts.size());

                for (int i = 0; i < m; ++i) {
                    const igIndex u = pts[static_cast<size_t>(i)];
                    const igIndex v = pts[static_cast<size_t>((i + 1) % m)];
                    const auto it = edgeUses.find(EdgeKey(u, v));
                    if (it == edgeUses.end()) continue;

                    for (const EdgeUse& use : it->second) {
                        const int neighbor = use.face;
                        if (neighbor == curFace || oriented[static_cast<size_t>(neighbor)]) {
                            continue;
                        }

                        if (use.u == u && use.v == v) {
                            ReverseFace(cleanFaceIds[static_cast<size_t>(neighbor)]);
                            ReverseFace(rawFaceIds[static_cast<size_t>(neighbor)]);
                            bool neighborDegenerate = false;
                            faceNormals[static_cast<size_t>(neighbor)] =
                                    ComputeFaceNormal(mesh,
                                                      cleanFaceIds[static_cast<size_t>(neighbor)],
                                                      neighborDegenerate);
                            degenerate[static_cast<size_t>(neighbor)] = neighborDegenerate;
                        }

                        oriented[static_cast<size_t>(neighbor)] = 1;
                        q.push(neighbor);
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------
    // 4. FlipNormals：按需翻转面环绕方向和面法向量。
    // ---------------------------------------------------------------
    if (m_FlipNormals) {
        for (int faceId = 0; faceId < nFaces; ++faceId) {
            ReverseFace(cleanFaceIds[static_cast<size_t>(faceId)]);
            ReverseFace(rawFaceIds[static_cast<size_t>(faceId)]);
            faceNormals[static_cast<size_t>(faceId)] =
                    Vector3f(-faceNormals[static_cast<size_t>(faceId)][0],
                             -faceNormals[static_cast<size_t>(faceId)][1],
                             -faceNormals[static_cast<size_t>(faceId)][2]);
        }
    }

    // ---------------------------------------------------------------
    // 5. 计算锐边分裂。
    // ---------------------------------------------------------------
    std::vector<int> faceComponent(static_cast<size_t>(nFaces), -1);
    int numComponents = 0;

    if (m_Splitting) {
        const double clampedAngle = std::max(0.0, std::min(180.0, m_FeatureAngle));
        const float featureAngleCos = static_cast<float>(
                std::cos(clampedAngle * kPi / 180.0));

        for (int seed = 0; seed < nFaces; ++seed) {
            if (faceComponent[static_cast<size_t>(seed)] >= 0) continue;

            if (degenerate[static_cast<size_t>(seed)]) {
                faceComponent[static_cast<size_t>(seed)] = numComponents++;
                continue;
            }

            faceComponent[static_cast<size_t>(seed)] = numComponents;
            std::queue<int> q;
            q.push(seed);

            while (!q.empty()) {
                const int curFace = q.front();
                q.pop();

                const auto& pts = cleanFaceIds[static_cast<size_t>(curFace)];
                const int m = static_cast<int>(pts.size());
                for (int i = 0; i < m; ++i) {
                    const igIndex u = pts[static_cast<size_t>(i)];
                    const igIndex v = pts[static_cast<size_t>((i + 1) % m)];
                    const auto it = edgeUses.find(EdgeKey(u, v));
                    if (it == edgeUses.end()) continue;

                    for (const EdgeUse& use : it->second) {
                        const int neighbor = use.face;
                        if (neighbor == curFace || faceComponent[static_cast<size_t>(neighbor)] >= 0) {
                            continue;
                        }

                        if (degenerate[static_cast<size_t>(neighbor)]) {
                            faceComponent[static_cast<size_t>(neighbor)] = numComponents;
                            continue;
                        }

                        const float dot = Dot(faceNormals[static_cast<size_t>(curFace)],
                                              faceNormals[static_cast<size_t>(neighbor)]);
                        if (dot >= featureAngleCos) {
                            faceComponent[static_cast<size_t>(neighbor)] = numComponents;
                            q.push(neighbor);
                        }
                    }
                }
            }

            ++numComponents;
        }
    } else {
        numComponents = 1;
        for (int faceId = 0; faceId < nFaces; ++faceId) {
            faceComponent[static_cast<size_t>(faceId)] = 0;
        }
    }

    // ---------------------------------------------------------------
    // 6. 生成输出点编号与输出面连接。
    // ---------------------------------------------------------------
    std::vector<std::map<igIndex, int>> compVertexMap;
    Points::Pointer newPoints = Points::New();
    std::vector<int> newPtOrigId;
    std::vector<std::vector<igIndex>> outputFaceIds(static_cast<size_t>(nFaces));
    int newPtCount = 0;

    if (m_Splitting) {
        compVertexMap.assign(static_cast<size_t>(numComponents), std::map<igIndex, int>());

        for (int faceId = 0; faceId < nFaces; ++faceId) {
            const int comp = faceComponent[static_cast<size_t>(faceId)];
            const bool useClean = !degenerate[static_cast<size_t>(faceId)];
            const auto& srcIds = useClean ? cleanFaceIds[static_cast<size_t>(faceId)]
                                          : rawFaceIds[static_cast<size_t>(faceId)];

            auto& outIds = outputFaceIds[static_cast<size_t>(faceId)];
            outIds.reserve(srcIds.size());
            for (igIndex origPt : srcIds) {
                auto& compMap = compVertexMap[static_cast<size_t>(comp)];
                auto it = compMap.find(origPt);
                if (it == compMap.end()) {
                    const int newId = newPtCount++;
                    compMap[origPt] = newId;
                    newPtOrigId.push_back(static_cast<int>(origPt));
                    outIds.push_back(newId);
                } else {
                    outIds.push_back(it->second);
                }
            }
        }

        newPoints->Reserve(newPtCount);
        for (int i = 0; i < newPtCount; ++i) {
            newPoints->AddPoint(mesh->GetPoint(newPtOrigId[static_cast<size_t>(i)]));
        }
    } else {
        newPtCount = nPoints;
        newPtOrigId.resize(static_cast<size_t>(nPoints));
        for (int i = 0; i < nPoints; ++i) {
            newPtOrigId[static_cast<size_t>(i)] = i;
        }

        newPoints->DeepCopy(mesh->GetPoints());
        for (int faceId = 0; faceId < nFaces; ++faceId) {
            outputFaceIds[static_cast<size_t>(faceId)] =
                    degenerate[static_cast<size_t>(faceId)]
                            ? rawFaceIds[static_cast<size_t>(faceId)]
                            : cleanFaceIds[static_cast<size_t>(faceId)];
        }
    }

    // ---------------------------------------------------------------
    // 7. 计算点法向量。
    // ---------------------------------------------------------------
    std::vector<Vector3f> pointNormals(static_cast<size_t>(newPtCount), Vector3f(0.0f, 0.0f, 0.0f));
    for (int faceId = 0; faceId < nFaces; ++faceId) {
        if (degenerate[static_cast<size_t>(faceId)]) continue;

        const auto& pts = cleanFaceIds[static_cast<size_t>(faceId)];
        const Vector3f& n = faceNormals[static_cast<size_t>(faceId)];

        if (m_Splitting) {
            const int comp = faceComponent[static_cast<size_t>(faceId)];
            auto& compMap = compVertexMap[static_cast<size_t>(comp)];
            for (igIndex origPt : pts) {
                pointNormals[static_cast<size_t>(compMap[origPt])] += n;
            }
        } else {
            for (igIndex pt : pts) {
                pointNormals[static_cast<size_t>(pt)] += n;
            }
        }
    }

    // ---------------------------------------------------------------
    // 8. 组装新 SurfaceMesh。
    // ---------------------------------------------------------------
    SurfaceMesh::Pointer newMesh = SurfaceMesh::New();
    newMesh->SetName(mesh->GetName() + "_normals");
    newMesh->SetPoints(newPoints);

    CellArray::Pointer newFaces = CellArray::New();
    for (int faceId = 0; faceId < nFaces; ++faceId) {
        auto& ids = outputFaceIds[static_cast<size_t>(faceId)];
        if (ids.empty()) {
            ids.push_back(0);
        }
        newFaces->AddCellIds(ids.data(), static_cast<int>(ids.size()));
    }
    newMesh->SetFaces(newFaces);

    auto newAttrs = newMesh->GetAttributeSet();
    auto srcAttrs = mesh->GetAttributeSet();

    CopyCellAttributes(srcAttrs, newAttrs, nFaces);
    CopyPointAttributes(srcAttrs, newAttrs, newPtOrigId, newPtCount);

    while (true) {
        const int idx = newAttrs->GetAttributeIndex("Normals");
        if (idx < 0) break;
        newAttrs->DeleteAttribute(idx);
    }
    while (true) {
        const int idx = newAttrs->GetAttributeIndex("Normals_Magnitude");
        if (idx < 0) break;
        newAttrs->DeleteAttribute(idx);
    }

    // ---------------------------------------------------------------
    // 9. 添加面法向量属性。
    // ---------------------------------------------------------------
    if (m_ComputeCellNormals) {
        FloatArray::Pointer cellNormals = FloatArray::New();
        cellNormals->SetDimension(3);
        cellNormals->Reserve(nFaces);
        cellNormals->SetName("Normals");

        FloatArray::Pointer cellMag = FloatArray::New();
        cellMag->SetDimension(1);
        cellMag->Reserve(nFaces);
        cellMag->SetName("Normals_Magnitude");

        for (int faceId = 0; faceId < nFaces; ++faceId) {
            const Vector3f& n = faceNormals[static_cast<size_t>(faceId)];
            cellNormals->AddElement3(n[0], n[1], n[2]);
            cellMag->AddValue(degenerate[static_cast<size_t>(faceId)] ? 0.0f : 1.0f);
        }

        newAttrs->AddAttribute(IG_NORMAL, IG_CELL, cellNormals);
        newAttrs->AddAttribute(IG_SCALAR, IG_CELL, cellMag);
    }

    // ---------------------------------------------------------------
    // 10. 添加点法向量属性。
    // ---------------------------------------------------------------
    if (m_ComputePointNormals) {
        FloatArray::Pointer pointNormalsArr = FloatArray::New();
        pointNormalsArr->SetDimension(3);
        pointNormalsArr->Reserve(newPtCount);
        pointNormalsArr->SetName("Normals");

        FloatArray::Pointer pointMag = FloatArray::New();
        pointMag->SetDimension(1);
        pointMag->Reserve(newPtCount);
        pointMag->SetName("Normals_Magnitude");

        for (int i = 0; i < newPtCount; ++i) {
            Vector3f n = pointNormals[static_cast<size_t>(i)];
            const float len = static_cast<float>(n.norm());
            if (len > 1e-30f) {
                const float invLen = 1.0f / len;
                n = Vector3f(n[0] * invLen, n[1] * invLen, n[2] * invLen);
            } else {
                n = Vector3f(0.0f, 0.0f, 0.0f);
            }

            pointNormalsArr->AddElement3(n[0], n[1], n[2]);
            pointMag->AddValue(len > 1e-30f ? 1.0f : 0.0f);
        }

        newAttrs->AddAttribute(IG_NORMAL, IG_POINT, pointNormalsArr);
        newAttrs->AddAttribute(IG_SCALAR, IG_POINT, pointMag);
    }

    newAttrs->ForceReConvertToDrawableData();
    newAttrs->Modified();
    newMesh->Modified();

    SetOutput(0, newMesh);
    return true;
}

IGAME_NAMESPACE_END
