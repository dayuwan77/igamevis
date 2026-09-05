#include "iGamePassArraysFilter.h"
#include "iGameArrayObject.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"

#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"
#include <algorithm>

IGAME_NAMESPACE_BEGIN

// 辅助函数：深拷贝一个 ArrayObject（根据实际类型调用对应 DeepCopy）
static ArrayObject::Pointer DeepCopyArray(ArrayObject::Pointer src) {
    if (!src) return nullptr;

    // 逐个尝试所有具体数组类型，调用其 DeepCopy 方法
    if (auto farr = DynamicCast<FloatArray>(src)) {
        auto copy = FloatArray::New();
        copy->DeepCopy(farr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto darr = DynamicCast<DoubleArray>(src)) {
        auto copy = DoubleArray::New();
        copy->DeepCopy(darr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto uarr = DynamicCast<UnsignedIntArray>(src)) {
        auto copy = UnsignedIntArray::New();
        copy->DeepCopy(uarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto iarr = DynamicCast<IntArray>(src)) {
        auto copy = IntArray::New();
        copy->DeepCopy(iarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto sarr = DynamicCast<ShortArray>(src)) {
        auto copy = ShortArray::New();
        copy->DeepCopy(sarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto usarr = DynamicCast<UnsignedShortArray>(src)) {
        auto copy = UnsignedShortArray::New();
        copy->DeepCopy(usarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto carr = DynamicCast<CharArray>(src)) {
        auto copy = CharArray::New();
        copy->DeepCopy(carr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto ucarr = DynamicCast<UnsignedCharArray>(src)) {
        auto copy = UnsignedCharArray::New();
        copy->DeepCopy(ucarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto llarr = DynamicCast<LongLongArray>(src)) {
        auto copy = LongLongArray::New();
        copy->DeepCopy(llarr);
        copy->SetName(src->GetName());
        return copy;
    }
    if (auto ullarr = DynamicCast<UnsignedLongLongArray>(src)) {
        auto copy = UnsignedLongLongArray::New();
        copy->DeepCopy(ullarr);
        copy->SetName(src->GetName());
        return copy;
    }

    // 如果出现未覆盖的类型则报错并返回空指针，避免隐式浅拷贝导致共享数据。
    igError("DeepCopyArray: unsupported array type (not in FlatArray macro list).");
    return nullptr;
}

bool PassArrays::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (!input) return false;

    IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output) return false;

    if (type == IG_UNSTRUCTURED_MESH) {
        auto inMesh = DynamicCast<UnstructuredMesh>(input);
        auto outMesh = DynamicCast<UnstructuredMesh>(output);
        if (!inMesh || !outMesh) return false;

        auto inPoints = inMesh->GetPoints();
        if (inPoints) {
            auto newPoints = Points::New();
            newPoints->DeepCopy(inPoints);
            outMesh->SetPoints(newPoints);
        }
        auto inCells = inMesh->GetCells();
        auto inTypes = inMesh->GetCellTypes();
        if (inCells && inTypes) {
            auto newCells = CellArray::New();
            newCells->DeepCopy(inCells);
            auto newTypes = UnsignedIntArray::New();
            newTypes->DeepCopy(inTypes);
            outMesh->SetCells(newCells, newTypes);
        }
    } else if (type == IG_SURFACE_MESH) {
        auto inMesh = DynamicCast<SurfaceMesh>(input);
        auto outMesh = DynamicCast<SurfaceMesh>(output);
        if (!inMesh || !outMesh) return false;

        auto inPoints = inMesh->GetPoints();
        if (inPoints) {
            auto newPoints = Points::New();
            newPoints->DeepCopy(inPoints);
            outMesh->SetPoints(newPoints);
        }
        auto inFaces = inMesh->GetFaces();
        if (inFaces) {
            auto newFaces = CellArray::New();
            newFaces->DeepCopy(inFaces);
            outMesh->SetFaces(newFaces);
        }
    } else if (type == IG_VOLUME_MESH) {
        auto inMesh = DynamicCast<VolumeMesh>(input);
        auto outMesh = DynamicCast<VolumeMesh>(output);
        if (!inMesh || !outMesh) return false;

        auto inPoints = inMesh->GetPoints();
        if (inPoints) {
            auto newPoints = Points::New();
            newPoints->DeepCopy(inPoints);
            outMesh->SetPoints(newPoints);
        }
        auto inVolumes = inMesh->GetVolumes();
        if (inVolumes) {
            auto newVolumes = CellArray::New();
            newVolumes->DeepCopy(inVolumes);
            outMesh->SetVolumes(newVolumes);
        }
    } else if (type == IG_POINT_SET) {
        auto inMesh = DynamicCast<PointSet>(input);
        auto outMesh = DynamicCast<PointSet>(output);
        if (!inMesh || !outMesh) return false;

        auto inPoints = inMesh->GetPoints();
        if (inPoints) {
            auto newPoints = Points::New();
            newPoints->DeepCopy(inPoints);
            outMesh->SetPoints(newPoints);
        }
        // PointSet 无单元，跳过
    } else if (type == IG_STRUCTURED_MESH) {
        auto inMesh = DynamicCast<StructuredMesh>(input);
        auto outMesh = DynamicCast<StructuredMesh>(output);
        if (!inMesh || !outMesh) return false;

        // 复制维度
        igIndex* dims = inMesh->GetDimensionSize();
        if (dims) {
            igIndex newDims[3] = {dims[0], dims[1], dims[2]};
            outMesh->SetDimensionSize(newDims);
        }

        // 复制点坐标
        auto inPoints = inMesh->GetPoints();
        if (inPoints) {
            auto newPoints = Points::New();
            newPoints->DeepCopy(inPoints);
            outMesh->SetPoints(newPoints);
        }
    } else {
        igError("iGamePassArrays does not support this data type.");
        return false;
    }
    // 属性深拷贝过滤
    auto inAttrSet = input->GetAttributeSet();
    auto outAttrSet = AttributeSet::New();
    if (inAttrSet) {
        // 点属性
        auto pointAttrs = inAttrSet->GetAllPointAttributes();
        if (pointAttrs) {
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                if (std::find(m_ArrayNames.begin(), m_ArrayNames.end(), attr.pointer->GetName()) !=
                    m_ArrayNames.end()) {
                    // 深拷贝属性数组
                    auto copiedArray = DeepCopyArray(attr.pointer);
                    if (copiedArray) {

                        DoubleArray::Pointer copiedRange = nullptr;
                        if (attr.dataRange) {
                            copiedRange = DoubleArray::New();
                            copiedRange->DeepCopy(attr.dataRange);
                        }
                        outAttrSet->AddAttribute(attr.type, IG_POINT, copiedArray, copiedRange);
                    }
                }
            }
        }
        // 单元属性
        auto cellAttrs = inAttrSet->GetAllCellAttributes();
        if (cellAttrs) {
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                if (std::find(m_ArrayNames.begin(), m_ArrayNames.end(), attr.pointer->GetName()) !=
                    m_ArrayNames.end()) {
                    auto copiedArray = DeepCopyArray(attr.pointer);
                    if (copiedArray) {
                        DoubleArray::Pointer copiedRange = nullptr;
                        if (attr.dataRange) {
                            copiedRange = DoubleArray::New();
                            copiedRange->DeepCopy(attr.dataRange);
                        }
                        outAttrSet->AddAttribute(attr.type, IG_CELL, copiedArray, copiedRange);
                    }
                }
            }
        }
    }
    output->SetAttributeSet(outAttrSet);


    SetOutput(0, output);
    return true;
}

IGAME_NAMESPACE_END