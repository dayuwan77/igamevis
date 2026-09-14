#include "iGameAppendReduceFilter.h"
#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include <cmath>
#include <cstdint>
#include <set>

IGAME_NAMESPACE_BEGIN

AppendReduceFilter::AppendReduceFilter()
    : m_MergePoints(true)
    , m_Tolerance(1e-6f)
{
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

AppendReduceFilter::~AppendReduceFilter() = default;

void AppendReduceFilter::AddInput(DataObject::Pointer data) {
    int n = this->GetNumberOfInputs();
    if (n == 1 && this->GetInput(0) == nullptr) {
        this->SetInput(0, data);
    } else {
        this->SetNumberOfInputs(n + 1);
        this->SetInput(n, data);
    }
}

void AppendReduceFilter::CollectMeshes(DataObject::Pointer obj,
                                        std::vector<SurfaceMesh::Pointer>& meshes) {
    if (!obj) return;

    IGenum type = obj->GetDataObjectType();

    if (type == IG_SURFACE_MESH) {
        auto mesh = DynamicCast<SurfaceMesh>(obj);
        if (mesh) meshes.push_back(mesh);
    } else if (type == IG_VOLUME_MESH || type == IG_UNSTRUCTURED_MESH || type == IG_STRUCTURED_MESH) {
        auto converter = ConvertToSurfaceMeshFilter::New();
        converter->SetInput(obj);
        converter->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
        if (!converter->Execute()) {
            converter->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_CELL);
            if (!converter->Execute()) {
                converter->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_CONVERT_SURFACE_MESH);
                converter->Execute();
            }
        }
        auto smesh = converter->GetSurfaceMesh();
        if (smesh) meshes.push_back(smesh);
    } else if (type == IG_DRAW_OBJECT) {
        auto drawObj = DynamicCast<DrawObject>(obj);
        if (drawObj) {
            auto renderable = drawObj->GetRenderableObject();
            if (renderable && renderable.GetPointer() != obj.GetPointer()) {
                auto smesh = DynamicCast<SurfaceMesh>(renderable);
                if (smesh) {
                    meshes.push_back(smesh);
                } else {
                    CollectMeshes(renderable, meshes);
                }
            }
        }
        if (obj->HasSubDataObject()) {
            for (auto it = obj->SubDataObjectIteratorBegin();
                 it != obj->SubDataObjectIteratorEnd(); ++it) {
                CollectMeshes(it->second, meshes);
            }
        }
    } else if (type == IG_COMPOSITE_DATA_OBJECT) {
        if (obj->HasSubDataObject()) {
            for (auto it = obj->SubDataObjectIteratorBegin();
                 it != obj->SubDataObjectIteratorEnd(); ++it) {
                CollectMeshes(it->second, meshes);
            }
        }
    }
}

void AppendReduceFilter::AppendMeshSimple(SurfaceMesh::Pointer src,
                                           Points::Pointer outPoints,
                                           CellArray::Pointer outFaces,
                                           igIndex& pointOffset,
                                           std::vector<igIndex>& pointMap) {
    if (!src) return;

    auto srcPoints = src->GetPoints();
    auto srcFaces = src->GetFaces();
    if (!srcPoints) return;

    IGsize numPoints = srcPoints->GetNumberOfPoints();
    pointMap.resize(numPoints);

    for (IGsize i = 0; i < numPoints; i++) {
        const Point& p = srcPoints->GetPoint(i);
        outPoints->AddPoint(p);
        pointMap[i] = pointOffset + (igIndex)i;
    }
    if (!srcFaces) {
        pointOffset += (igIndex)numPoints;
        return;
    }
    IGsize numFaces = srcFaces->GetNumberOfCells();
    for (IGsize f = 0; f < numFaces; f++) {
        const igIndex* ids = nullptr;
        int numIds = srcFaces->GetCellIds(f, ids);

        std::vector<igIndex> adjustedIds(numIds);
        for (int i = 0; i < numIds; i++) {
            adjustedIds[i] = ids[i] + pointOffset;
        }
        outFaces->AddCellIds(adjustedIds.data(), numIds);
    }
    pointOffset += (igIndex)numPoints;
}

void AppendReduceFilter::AppendMeshWithMerge(
    SurfaceMesh::Pointer src,
    Points::Pointer outPoints,
    CellArray::Pointer outFaces,
    std::unordered_map<int64_t, std::vector<igIndex>>& pointHash,
    std::vector<igIndex>& pointMap)
{
    if (!src) return;

    auto srcPoints = src->GetPoints();
    auto srcFaces = src->GetFaces();
    if (!srcPoints) return;

    IGsize numPoints = srcPoints->GetNumberOfPoints();
    pointMap.resize(numPoints);

    float invTol = 1.0f / m_Tolerance;
    float tol2 = m_Tolerance * m_Tolerance;

    for (IGsize i = 0; i < numPoints; i++) {
        const Point& p = srcPoints->GetPoint(i);

        int64_t ix = (int64_t)(p[0] * invTol);
        int64_t iy = (int64_t)(p[1] * invTol);
        int64_t iz = (int64_t)(p[2] * invTol);
        int64_t key = (ix * 73856093LL) ^ (iy * 19349663LL) ^ (iz * 83492791LL);

        bool found = false;
        auto it = pointHash.find(key);
        if (it != pointHash.end()) {
            for (igIndex existingIdx : it->second) {
                const Point& existing = outPoints->GetPoint(existingIdx);
                float dx = p[0] - existing[0];
                float dy = p[1] - existing[1];
                float dz = p[2] - existing[2];
                if (dx * dx + dy * dy + dz * dz < tol2) {
                    pointMap[i] = existingIdx;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            igIndex newIdx = (igIndex)outPoints->AddPoint(p);
            pointHash[key].push_back(newIdx);
            pointMap[i] = newIdx;
        }
    }

    if (!srcFaces) return;

    IGsize numFaces = srcFaces->GetNumberOfCells();
    for (IGsize f = 0; f < numFaces; f++) {
        const igIndex* ids = nullptr;
        int numIds = srcFaces->GetCellIds(f, ids);

        std::vector<igIndex> mappedIds(numIds);
        for (int i = 0; i < numIds; i++) {
            mappedIds[i] = pointMap[ids[i]];
        }
        outFaces->AddCellIds(mappedIds.data(), numIds);
    }
}

void AppendReduceFilter::MergeAttributes(
    const std::vector<SurfaceMesh::Pointer>& meshes,
    const std::vector<MeshPointMap>& meshMaps,
    SurfaceMesh::Pointer outMesh)
{
    if (meshes.empty() || meshMaps.empty()) return;

    auto outAttrSet = outMesh->GetAttributeSet();
    if (!outAttrSet) return;

    struct AttrKey {
        std::string name;
        IGenum type;
        IGenum attachmentType;
        bool operator<(const AttrKey& o) const {
            if (name != o.name) return name < o.name;
            if (type != o.type) return type < o.type;
            return attachmentType < o.attachmentType;
        }
    };

    std::map<AttrKey, int> attrCount;
    for (auto& mesh : meshes) {
        if (!mesh) continue;
        auto attrSet = mesh->GetAttributeSet();
        if (!attrSet) continue;
        std::set<AttrKey> seen;
        for (int i = 0; i < attrSet->GetNumberOfAttributes(); i++) {
            auto& attr = attrSet->GetAttribute(i);
            if (attr.IsNone()) continue;
            AttrKey key{attr.pointer->GetName(), attr.type, attr.attachmentType};
            seen.insert(key);
        }
        for (auto& k : seen) {
            attrCount[k]++;
        }
    }

    IGsize numOutPoints = outMesh->GetNumberOfPoints();
    IGsize numOutFaces = outMesh->GetFaces() ? outMesh->GetFaces()->GetNumberOfCells() : 0;

    int mergedCount = 0;
    for (auto& [key, count] : attrCount) {
        if (count < (int)meshes.size()) continue;

        FloatArray::Pointer outArray = FloatArray::New();
        outArray->SetName(key.name);
        int dim = 1;

        std::vector<float> elemBuf(16, 0.0f);
        for (auto& mesh : meshes) {
            if (!mesh) continue;
            auto attrSet = mesh->GetAttributeSet();
            if (!attrSet) continue;
            auto& attr = attrSet->GetAttribute(key.name, key.type);
            if (attr.IsNone()) continue;
            dim = attr.pointer->GetDimension();
            break;
        }

        outArray->SetDimension(dim);
        IGsize numElements = (key.attachmentType == IG_POINT) ? numOutPoints : numOutFaces;
        outArray->Resize(numElements);

        for (size_t m = 0; m < meshes.size(); m++) {
            if (!meshes[m]) continue;
            auto attrSet = meshes[m]->GetAttributeSet();
            if (!attrSet) continue;
            auto& attr = attrSet->GetAttribute(key.name, key.type);
            if (attr.IsNone()) continue;

            int srcDim = attr.pointer->GetDimension();
            IGsize srcNum = attr.pointer->GetNumberOfElements();

            if (key.attachmentType == IG_POINT) {
                const auto& pmap = meshMaps[m].pointMap;
                for (IGsize i = 0; i < srcNum && i < pmap.size(); i++) {
                    igIndex outIdx = pmap[i];
                    if (outIdx < 0 || outIdx >= (igIndex)numElements) continue;
                    attr.pointer->GetElement(i, elemBuf.data());
                    outArray->SetElement(outIdx, elemBuf.data());
                }
            } else {
                igIndex faceOff = meshMaps[m].faceOffset;
                for (IGsize i = 0; i < srcNum; i++) {
                    igIndex outIdx = faceOff + (igIndex)i;
                    if (outIdx < 0 || outIdx >= (igIndex)numElements) continue;
                    attr.pointer->GetElement(i, elemBuf.data());
                    outArray->SetElement(outIdx, elemBuf.data());
                }
            }
        }

        outAttrSet->AddAttribute(key.type, key.attachmentType, outArray);
        mergedCount++;
    }
}

bool AppendReduceFilter::Execute() {
    std::vector<SurfaceMesh::Pointer> meshes;
    int inputCount = this->GetNumberOfInputs();
    for (int i = 0; i < inputCount; i++) {
        auto input = this->GetInput(i);
        if (input) {
            this->CollectMeshes(input, meshes);
        }
    }

    if (meshes.empty()) {
        return false;
    }

    auto outMesh = SurfaceMesh::New();
    auto outPoints = Points::New();
    auto outFaces = CellArray::New();

    std::vector<MeshPointMap> meshMaps(meshes.size());

    if (m_MergePoints) {
        std::unordered_map<int64_t, std::vector<igIndex>> pointHash;
        pointHash.reserve(1024);

        for (size_t i = 0; i < meshes.size(); i++) {
            AppendMeshWithMerge(meshes[i], outPoints, outFaces, pointHash, meshMaps[i].pointMap);
        }
    } else {
        igIndex pointOffset = 0;
        for (size_t i = 0; i < meshes.size(); i++) {
            AppendMeshSimple(meshes[i], outPoints, outFaces, pointOffset, meshMaps[i].pointMap);
        }
    }

    igIndex faceOffset = 0;
    for (size_t i = 0; i < meshes.size(); i++) {
        meshMaps[i].faceOffset = faceOffset;
        auto fcs = meshes[i]->GetFaces();
        if (fcs) {
            faceOffset += (igIndex)fcs->GetNumberOfCells();
        }
    }

    outMesh->SetPoints(outPoints);
    outMesh->SetFaces(outFaces);
    outMesh->RequestEditStatus();
    outMesh->Modified();

    MergeAttributes(meshes, meshMaps, outMesh);

    // Force re-conversion of drawable data to ensure color mapping works
    // This matches the pattern used by CurvatureFilter, GradientFilter, etc.
    auto outAttrSet = outMesh->GetAttributeSet();
    if (outAttrSet && outAttrSet->GetNumberOfAttributes() > 0) {
        outAttrSet->ForceReConvertToDrawableData();
    }

    this->SetOutput(outMesh);
    return true;
}

IGAME_NAMESPACE_END
