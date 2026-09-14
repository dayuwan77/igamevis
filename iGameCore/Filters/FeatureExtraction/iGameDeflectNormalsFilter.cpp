#include "iGameDeflectNormalsFilter.h"

IGAME_NAMESPACE_BEGIN

DeflectNormalsFilter::DeflectNormalsFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
DeflectNormalsFilter::~DeflectNormalsFilter() = default;

ArrayObject::Pointer DeflectNormalsFilter::AttributeCell2Point(
    CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum)
{
    const int dim = ori->GetDimension();
    auto newArr = FloatArray::New();
    newArr->SetName(ori->GetName());
    newArr->SetDimension(dim);
    if (pointNum == 0) return newArr;
    newArr->Reserve(pointNum);

    float s[16] = { 0 }, t[16] = { 0 };
    for (size_t i = 0; i < pointNum; ++i) newArr->AddElement(s);

    std::vector<int> adj(pointNum, 0);
    igIndex cell[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < cells->GetNumberOfCells(); ++i) {
        const int size = cells->GetCellIds(i, cell);
        ori->GetElement(i, s);
        for (int j = 0; j < size; ++j) {
            if (static_cast<size_t>(cell[j]) >= pointNum) continue;
            adj[cell[j]]++;
            newArr->GetElement(cell[j], t);
            for (int d = 0; d < dim; ++d) t[d] += s[d];
            newArr->SetElement(cell[j], t);
        }
    }
    for (size_t i = 0; i < pointNum; ++i) {
        if (adj[i] > 0) {
            newArr->GetElement(i, t);
            for (int d = 0; d < dim; ++d) t[d] /= static_cast<float>(adj[i]);
            newArr->SetElement(i, t);
        }
    }
    return newArr;
}

bool DeflectNormalsFilter::ResolveVectorField(
    AttributeSet* attrSet,
    IGenum& outAttach, AttributeSet::Attribute*& outAttr,
    bool& outNeedCellToPoint, std::string& why)
{
    auto N = attrSet->GetNumberOfAttributes();
    auto MatchIndex = [&](IGenum attach, AttributeSet::Attribute*& a) -> bool {
        for (int i = 0; i < N; ++i) {
            auto& ref = attrSet->GetAttribute(i);
            if (ref.isDeleted || !ref.pointer) continue;
            if (ref.attachmentType != attach) continue;
            if (_attrIdx >= 0 && _attrIdx == i) { a = &ref; return true; }
        }
        return false;
    };
    auto MatchName = [&](IGenum attach, AttributeSet::Attribute*& a) -> bool {
        for (int i = 0; i < N; ++i) {
            auto& ref = attrSet->GetAttribute(i);
            if (ref.isDeleted || !ref.pointer) continue;
            if (ref.attachmentType != attach) continue;
            if (!_attrName.empty() && ref.pointer->GetName() == _attrName) { a = &ref; return true; }
        }
        return false;
    };
    auto CheckDim = [&](AttributeSet::Attribute* a, IGenum attach, bool& needC2P) -> bool {
        if (!a || !a->pointer) return false;
        if (a->pointer->GetDimension() != 3) { why = "vector field attribute must have dimension == 3."; return false; }
        outAttach = attach;
        outAttr   = a;
        outNeedCellToPoint = (attach == IG_CELL);
        return true;
    };
    auto Try = [&](IGenum attach) -> bool {
        AttributeSet::Attribute* a = nullptr;
        if (_attrIdx >= 0 ? MatchIndex(attach, a) : MatchName(attach, a)) {
            return CheckDim(a, attach, outNeedCellToPoint);
        }
        return false;
    };

    if (_vfAttach == IG_POINT) {
        if (Try(IG_POINT)) return true;
        why = "vector field '" + _attrName + "' not found on POINT data.";
        return false;
    }
    if (_vfAttach == IG_CELL) {
        if (Try(IG_CELL)) return true;
        why = "vector field '" + _attrName + "' not found on CELL data.";
        return false;
    }
    // IG_NONE / Auto 模式
    if (Try(IG_POINT)) return true;
    if (Try(IG_CELL))  return true;
    why = "vector field '" + _attrName + "' not found on POINT or CELL data.";
    return false;
}

bool DeflectNormalsFilter::Execute() {
    auto input = GetInput(0);
    if (!input) return false;

    // ----- 表面网格准备 -----
    SurfaceMesh::Pointer mesh;
    CellArray::Pointer surfaceFaces; // 用于 Cell->Point 转换时的"面/单元集合"

    switch (input->GetDataObjectType()) {
    case IG_SURFACE_MESH:
        mesh = DynamicCast<SurfaceMesh>(input);
        break;
    case IG_VOLUME_MESH: {
        // VolumeMesh：使用渲染表面（边界面）作为法向的几何来源
        auto vm = DynamicCast<VolumeMesh>(input);
        if (!vm) break;
        mesh = DynamicCast<SurfaceMesh>(vm->GetRenderableObject());
        break;
    }
    case IG_STRUCTURED_MESH: {
        // StructuredMesh 也是体网格：同样用可渲染的边界面
        auto sm = DynamicCast<StructuredMesh>(input);
        if (!sm) break;
        mesh = DynamicCast<SurfaceMesh>(sm->GetRenderableObject());
        break;
    }
    case IG_UNSTRUCTURED_MESH: {
        auto um = DynamicCast<UnstructuredMesh>(input);
        if (!um) break;
        mesh = um->TransferToSurfaceMesh();
        if (!mesh) mesh = um->ExtractSurfaceMesh();
        break;
    }
    default:
        _msg = "Not Surface Mesh!";
        return false;
    }
    if (!mesh) { _msg = "Failed to obtain surface mesh for normal computation."; return false; }
    mesh->BuildFaceLinks();
    surfaceFaces = mesh->GetFaces();   // Cell->Point 基于"面数组"（与法向计算邻接保持一致）

    const int numPts = static_cast<int>(mesh->GetNumberOfPoints());
    if (numPts <= 0) { _msg = "mesh has no points."; return false; }

    // ----- 先建立基准法向（曲面平均 / UserNormal），存放到 result 作为临时缓存 -----
    FloatArray::Pointer result = FloatArray::New();
    result->SetName("DeflectedNormals");
    result->SetDimension(3);
    result->Reserve(numPts);

    igIndex faceIds[256];
    for (int ptId = 0; ptId < numPts; ++ptId) {
        Vector3f base(0.f, 0.f, 0.f);
        if (_useUserNormal) {
            base = _userNormal;
        } else {
            const int n = mesh->GetPointToNeighborFaces(ptId, faceIds);
            for (int j = 0; j < n; ++j) {
                auto f = mesh->GetFace(faceIds[j]);
                if (f) base += f->GetNormal();
            }
        }
        base.normalize();
        const float out[3] = { base[0], base[1], base[2] };
        result->AddElement(out);
    }

    // ----- 解析向量场：② 精确区分 POINT/CELL 同名数组 -----
    auto attrSet = mesh->GetAttributeSet();
    if (!attrSet) { _msg = "mesh has no AttributeSet; cannot resolve vector field."; return false; }
    if (_attrIdx < 0 && _attrName.empty()) {
        _msg = "please choose a vector field attribute."; return false;
    }

    IGenum vfAttach{ IG_NONE };
    AttributeSet::Attribute* attr = nullptr;
    bool needCellToPoint = false;
    std::string why;
    if (!ResolveVectorField(attrSet, vfAttach, attr, needCellToPoint, why)) {
        _msg = why.empty() ? std::string("please choose a valid vector field.") : why;
        return false;
    }
    if (!attr || !attr->pointer) { _msg = "vector field attribute has null pointer."; return false; }

    ArrayObject::Pointer vecField = attr->pointer;
    if (needCellToPoint && surfaceFaces) {
        // Cell -> Point 按照"面"邻接做平均（与法向邻接一致，避免 VolumeMesh/UnstructuredMesh
        // 误用体单元索引做 cell->point，而 mesh 上的点数是"表面点"）
        vecField = AttributeCell2Point(surfaceFaces, vecField, numPts);
    }
    if (!vecField) { _msg = "vector field is null after resolve."; return false; }

    // ----- 偏转公式：ND = normalize(base + strength * V) -----
    float buf[3] = { 0, 0, 0 };
    for (int ptId = 0; ptId < numPts; ++ptId) {
        result->GetElement(ptId, buf);
        const Vector3f base(buf[0], buf[1], buf[2]);

        vecField->GetElement(ptId, buf);
        const Vector3f V(buf[0], buf[1], buf[2]);

        Vector3f ND = base + V * _strength;
        ND.normalize();

        const float out[3] = { ND[0], ND[1], ND[2] };
        result->SetElement(ptId, out);
    }

    attrSet->AddVector(IG_POINT, result);
    attrSet->ForceReConvertToDrawableData();
    SetOutput(mesh);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END