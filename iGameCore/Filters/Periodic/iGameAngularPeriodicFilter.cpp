#include "Periodic/iGameAngularPeriodicFilter.h"

#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// 一份旋转副本里的单元：普通单元 ids 为顶点号序列；
// IG_POLYHEDRON 单元 ids 为核心约定的编码序列：
//   [faceCount, face0_npts, face0顶点..., face1_npts, face1顶点..., ...]
struct CellRecord {
    std::vector<igIndex> ids;
    IGenum type{IG_EMPTY_CELL};
};

// 按单元类型搬运顶点偏移。
// 多面体编码里只有"顶点号"需要 +offset，faceCount/npts 等计数不能动。
void OffsetCellIds(const CellRecord& cell, igIndex offset, std::vector<igIndex>& shifted) {
    const auto& in = cell.ids;
    if (cell.type == IG_POLYHEDRON) {
        std::vector<igIndex> tmp;
        tmp.reserve(in.size());
        size_t r = 0;
        if (r >= in.size()) { shifted.clear(); return; }
        igIndex nFaces = in[r++];
        tmp.push_back(nFaces);
        for (igIndex f = 0; f < nFaces && r < in.size(); ++f) {
            igIndex nPts = in[r++];
            tmp.push_back(nPts);
            for (igIndex j = 0; j < nPts && r < in.size(); ++j) {
                tmp.push_back(in[r++] + offset);
            }
        }
        tmp.shrink_to_fit();
        shifted.swap(tmp);
    } else {
        shifted.resize(in.size());
        for (size_t i = 0; i < in.size(); ++i) { shifted[i] = in[i] + offset; }
    }
}

// 读出任意长度单元的点号序列（指针版，无固定缓冲越界风险）。
bool ReadCellRecord(CellArray* cellArray, IGsize cellId, CellRecord& record) {
    record.ids.clear();
    record.type = IG_EMPTY_CELL;
    if (cellArray == nullptr) return true;
    const igIndex* ids = nullptr;
    int n = cellArray->GetCellIds(cellId, ids);
    if (n > 0) { record.ids.assign(ids, ids + n); }
    return true;
}

// 按输入网格类型把拓扑归一化为统一 CellRecord 序列。
// 返回 false 表示遇到无法安全复制的拓扑（输入本身单元结构异常）。
bool CollectCells(PointSet* src, std::vector<CellRecord>& cells, std::string& message) {
    cells.clear();
    if (src == nullptr) return true;

    // ---------- 结构化网格：物化隐式拓扑 ----------
    if (auto structured = DynamicCast<StructuredMesh>(src)) {
        igIndex* dims = structured->GetDimensionSize();
        structured->GenStructuredCellConnectivities();
        if (dims[2] > 1) {
            CellArray* volumes = structured->GetVolumes();
            const IGsize n = volumes ? volumes->GetNumberOfCells() : 0;
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(volumes, c, record);
                if (record.ids.size() != 8) {
                    message = "StructuredMesh: 3D 结构化单元不是 8 点六面体。";
                    return false;
                }
                record.type = IG_HEXAHEDRON;
                cells.push_back(std::move(record));
            }
        } else {
            CellArray* faces = structured->GetFaces();
            const IGsize n = faces ? faces->GetNumberOfCells() : 0;
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(faces, c, record);
                if (record.ids.size() != 4) {
                    message = "StructuredMesh: 2D 结构化单元不是 4 点四边形。";
                    return false;
                }
                record.type = IG_QUAD;
                cells.push_back(std::move(record));
            }
        }
        return true;
    }

    // ---------- 体网格 ----------
    if (auto volumeMesh = DynamicCast<VolumeMesh>(src)) {
        if (volumeMesh->GetIsPolyhedronType()) {
            // 多面体按核心约定的面编码存储在 CellArray 中，委托官方转换以拿到规范编码
            UnstructuredMesh::Pointer converted = nullptr;
            if (!UnstructuredMesh::TransferVolumeMeshToUnstructuredMesh(volumeMesh, converted) ||
                converted == nullptr) {
                message = "VolumeMesh: 无法把多面体体网格转换为规范的 UnstructuredMesh。";
                return false;
            }
            CellArray* cellArray = converted->GetCells();
            const IGsize n = converted->GetNumberOfCells();
            for (IGsize c = 0; c < n; ++c) {
                CellRecord record;
                ReadCellRecord(cellArray, c, record);
                record.type = IG_POLYHEDRON;
                cells.push_back(std::move(record));
            }
            return true;
        }

        CellArray* volumes = volumeMesh->GetVolumes();
        const IGsize n = volumes ? volumes->GetNumberOfCells() : 0;
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(volumes, c, record);
            IGenum type = VolumeMesh::GetVolumeTypeWithPointNum(static_cast<int>(record.ids.size()));
            if (type == IG_EMPTY_CELL) {
                message = "VolumeMesh: 不支持的体单元顶点数 " +
                          std::to_string(record.ids.size()) + "（需为 4/5/6/8）。";
                return false;
            }
            record.type = type;
            cells.push_back(std::move(record));
        }
        return true;
    }

    // ---------- 非结构化网格：类型权威在 m_Types ----------
    if (auto unstructured = DynamicCast<UnstructuredMesh>(src)) {
        CellArray* cellArray = unstructured->GetCells();
        const IGsize n = unstructured->GetNumberOfCells();
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(cellArray, c, record);
            if (!record.ids.empty()) {
                record.type = unstructured->GetCellType(c);
                cells.push_back(std::move(record));
            }
        }
        return true;
    }

    // ---------- 曲面网格：按面点数推断类型（>4 点归为 IG_POLYGON） ----------
    if (auto surface = DynamicCast<SurfaceMesh>(src)) {
        CellArray* faces = surface->GetFaces();
        const IGsize n = faces ? faces->GetNumberOfCells() : 0;
        for (IGsize c = 0; c < n; ++c) {
            CellRecord record;
            ReadCellRecord(faces, c, record);
            if (record.ids.empty()) continue;
            IGenum type = SurfaceMesh::GetFaceTypeWithPointNum(static_cast<int>(record.ids.size()));
            if (type == IG_EMPTY_CELL) continue;
            record.type = type;
            cells.push_back(std::move(record));
        }
        return true;
    }

    // 其余（纯 PointSet 点云等）：无拓扑，只复制点
    return true;
}

// 把源属性数组原样复制 copies 遍（点/单元属性在输出里按每份连续排列）。
template <typename TArray>
ArrayObject::Pointer DuplicateAttributeArrayN(typename TArray::Pointer input, int copies) {
    if (!input) return nullptr;
    auto output = TArray::New();
    output->SetName(input->GetName());
    output->SetDimension(input->GetDimension());

    const IGsize tuples = input->GetNumberOfElements();
    const IGsize values = input->GetNumberOfValues();
    output->Resize(tuples * copies);

    const auto* src = input->RawPointer();
    auto* dst = output->RawPointer();
    for (int c = 0; c < copies; ++c) {
        std::copy(src, src + values, dst + static_cast<IGsize>(c) * values);
    }
    return output;
}

ArrayObject::Pointer DuplicateAttribute(const AttributeSet::Attribute& attr, int copies) {
    auto duplicate = [&](auto array) -> ArrayObject::Pointer {
        using ArrayType = typename decltype(array)::ObjectType;
        return DuplicateAttributeArrayN<ArrayType>(array, copies);
    };

    if (auto array = DynamicCast<FloatArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<DoubleArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<IntArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<ShortArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<CharArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<LongLongArray>(attr.pointer)) return duplicate(array);

    if (auto array = DynamicCast<UnsignedIntArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<UnsignedShortArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<UnsignedCharArray>(attr.pointer)) return duplicate(array);
    if (auto array = DynamicCast<UnsignedLongLongArray>(attr.pointer)) return duplicate(array);

    return nullptr;
}

// 把源 PointData/CellData 全部属性复制到输出（每份值不变，仅逐份重复）。
bool CopyAttributesToOutput(PointSet* src,
                            UnstructuredMesh* output,
                            int copies,
                            std::string& message) {
    auto outputAttributes = AttributeSet::New();

    auto srcAttributes = src->GetAttributeSet();
    if (srcAttributes) {
        auto all = srcAttributes->GetAllAttributes();
        const IGsize count = all->GetNumberOfElements();
        for (IGsize i = 0; i < count; ++i) {
            auto& attr = all->GetElement(i);
            if (attr.IsNone() || !attr.pointer) continue;

            auto copied = DuplicateAttribute(attr, copies);
            if (!copied) {
                message = "不支持复制该属性数组类型: " + attr.pointer->GetName();
                return false;
            }
            const IGsize index = outputAttributes->AddAttribute(attr.type, attr.attachmentType, copied);
            if (index == static_cast<IGsize>(-1)) {
                message = "添加输出属性失败: " + attr.pointer->GetName();
                return false;
            }
            outputAttributes->GetAttribute(index).UpdateAllDataRange();
        }
    }

    output->SetAttributeSet(outputAttributes);
    return true;
}

} // namespace

AngularPeriodicFilter::AngularPeriodicFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void AngularPeriodicFilter::SetRotationAxis(const Point& origin, const Vector3d& axis) {
    m_AxisOrigin = origin;
    m_AxisNormalized = axis;
    if (m_AxisNormalized.length() > 1e-12) {
        m_AxisNormalized.normalize();
    }
}

bool AngularPeriodicFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "no input mesh";
        return false;
    }
    if (m_AxisNormalized.length() < 1e-12) {
        m_Message = "rotation axis has zero length";
        return false;
    }
    if (m_NumberOfCopies < 1) {
        m_Message = "number of copies is invalid";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (mesh == nullptr) {
        m_Message = "input is not a surface/unstructured mesh";
        return false;
    }

    Points* srcPoints = mesh->GetPoints();
    const IGsize numPoints = srcPoints ? srcPoints->GetNumberOfPoints() : 0;
    if (numPoints == 0) {
        m_Message = "input mesh has no points";
        return false;
    }

    // 归一化：一次采集源拓扑（含正确单元类型），避免每份复制时猜类型/丢单元
    std::vector<CellRecord> cells;
    if (!CollectCells(mesh.get(), cells, m_Message)) {
        return false;
    }

    auto outputPoints = Points::New();
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();

    const float stepAngle =
        m_Angle / m_NumberOfCopies * 3.14159265358979f / 180.0f;
    for (int i = 0; i < m_NumberOfCopies; ++i) {
        const float angleRad = stepAngle * static_cast<float>(i);
        const igIndex pointOffset = static_cast<igIndex>(numPoints * i);

        for (IGsize p = 0; p < numPoints; ++p) {
            outputPoints->AddPoint(RotatePoint(srcPoints->GetPoint(p), angleRad));
        }

        std::vector<igIndex> shifted;
        for (const CellRecord& cell : cells) {
            OffsetCellIds(cell, pointOffset, shifted);
            if (shifted.empty()) continue;
            outputCells->AddCellIds(shifted.data(), static_cast<int>(shifted.size()));
            outputTypes->AddValue(static_cast<unsigned int>(cell.type));
        }
    }

    auto output = UnstructuredMesh::New();
    output->SetPoints(outputPoints);
    output->SetCells(outputCells, outputTypes);

    // PointData/CellData 逐份复制到输出，不丢属性
    if (!CopyAttributesToOutput(mesh.get(), output.get(), m_NumberOfCopies, m_Message)) {
        return false;
    }

    SetOutput(output);
    return true;
}

Point AngularPeriodicFilter::RotatePoint(const Point& p, float angleRad) {
    double cosA = std::cos(angleRad);
    double sinA = std::sin(angleRad);

    Vector3d v(p[0] - m_AxisOrigin[0],
               p[1] - m_AxisOrigin[1],
               p[2] - m_AxisOrigin[2]);
    Vector3d axis = m_AxisNormalized;

    Vector3d cross = axis.cross(v);
    double dot = axis.dot(v);

    Vector3d rotated = v * cosA + cross * sinA + axis * (dot * (1.0 - cosA));

    return Point(static_cast<float>(m_AxisOrigin[0] + rotated[0]),
                 static_cast<float>(m_AxisOrigin[1] + rotated[1]),
                 static_cast<float>(m_AxisOrigin[2] + rotated[2]));
}

IGAME_NAMESPACE_END
