#include "iGameDataObjectCopy.h"

#include "iGameLagrangeUnstructuredMesh.h"
#include "iGamePointSet.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

IGAME_NAMESPACE_BEGIN

namespace
{

// 复制单元数组，避免 CellArray::DeepCopy 在混合单元尺寸下产生错误偏移
CellArray::Pointer DeepCopyCellArray(CellArray* source) {
    auto output = CellArray::New();
    if (!source) {
        return output;
    }

    for (IGsize i = 0; i < source->GetNumberOfCells(); ++i) {
        const igIndex* ids = nullptr;
        const int count = source->GetCellIds(i, ids);
        output->AddCellIds(ids, count);
    }

    return output;
}

// 复制 PointSet 共有的点坐标和属性
void DeepCopyPointSetBase(PointSet* output, PointSet* input, AttributeSet::Pointer attributes) {
    auto points = Points::New();
    if (input->GetPoints()) {
        points->DeepCopy(input->GetPoints());
    }
    output->SetPoints(points);

    if (attributes != nullptr) {
        // 调用方给定属性集：几何取自 input、属性来自别处
        output->SetAttributeSet(attributes);
    } else {
        auto copiedAttributes = AttributeSet::New();
        if (input->GetAttributeSet()) {
            copiedAttributes->DeepCopy(input->GetAttributeSet());
        }
        output->SetAttributeSet(copiedAttributes);
    }
    output->SetName(input->GetName());
}

} // namespace

// 按数据类型创建并深拷贝一个独立输出对象
DataObject::Pointer DeepCopyDataObject(DataObject::Pointer input, AttributeSet::Pointer attributes) {
    if (input == nullptr) {
        return nullptr;
    }

    const IGenum type = input->GetDataObjectType();
    DataObject::Pointer output = DataObject::CreateDataObject(type);
    if (!output && type == IG_LAGRANGE_UNSTRUCTURED_MESH) {
        output = LagrangeUnstructuredMesh::New();
    }
    if (!output) {
        return nullptr;
    }

    auto inputPointSet = DynamicCast<PointSet>(input);
    auto outputPointSet = DynamicCast<PointSet>(output);
    if (!inputPointSet || !outputPointSet) {
        return nullptr;
    }
    DeepCopyPointSetBase(outputPointSet.get(), inputPointSet.get(), attributes);

    switch (type) {
        case IG_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<UnstructuredMesh>(input);
            auto outputMesh = DynamicCast<UnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            auto cellTypes = UnsignedIntArray::New();
            if (inputMesh->GetCellTypes()) {
                cellTypes->DeepCopy(inputMesh->GetCellTypes());
            }
            outputMesh->SetCells(DeepCopyCellArray(inputMesh->GetCells()), cellTypes);
            break;
        }
        case IG_SURFACE_MESH: {
            auto inputMesh = DynamicCast<SurfaceMesh>(input);
            auto outputMesh = DynamicCast<SurfaceMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_VOLUME_MESH: {
            auto inputMesh = DynamicCast<VolumeMesh>(input);
            auto outputMesh = DynamicCast<VolumeMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            if (inputMesh->GetIsPolyhedronType()) {
                // 多面体网格需要同时重建面片和体-面索引
                auto faces = DeepCopyCellArray(inputMesh->GetFaces());
                auto volumeFaces = CellArray::New();
                for (IGsize i = 0; i < inputMesh->GetNumberOfVolumes(); ++i) {
                    std::vector<igIndex> faceIds(IGAME_CELL_MAX_SIZE);
                    const int faceCount = inputMesh->GetVolumeFaceIds(i, faceIds.data());
                    volumeFaces->AddCellIds(faceIds.data(), faceCount);
                }
                outputMesh->InitVolumesWithPolyhedron(faces, volumeFaces);
                break;
            }

            if (inputMesh->GetVolumes()) {
                outputMesh->SetVolumes(DeepCopyCellArray(inputMesh->GetVolumes()));
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            outputMesh->SetIsPolyhedronType(false);
            break;
        }
        case IG_STRUCTURED_MESH: {
            auto inputMesh = DynamicCast<StructuredMesh>(input);
            auto outputMesh = DynamicCast<StructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            igIndex* dimensions = inputMesh->GetDimensionSize();
            if (dimensions) {
                igIndex copiedDimensions[3]{dimensions[0], dimensions[1], dimensions[2]};
                outputMesh->SetDimensionSize(copiedDimensions);
            }
            if (inputMesh->GetFaces()) {
                outputMesh->SetFaces(DeepCopyCellArray(inputMesh->GetFaces()));
            }
            if (inputMesh->GetEdges()) {
                outputMesh->SetEdges(DeepCopyCellArray(inputMesh->GetEdges()));
            }
            break;
        }
        case IG_LAGRANGE_UNSTRUCTURED_MESH: {
            auto inputMesh = DynamicCast<LagrangeUnstructuredMesh>(input);
            auto outputMesh = DynamicCast<LagrangeUnstructuredMesh>(output);
            if (!inputMesh || !outputMesh) {
                return nullptr;
            }

            for (IGsize i = 0; i < inputMesh->GetNumberOfCells(); ++i) {
                const igIndex* ids = nullptr;
                const int count = inputMesh->GetCellPointIds(i, ids);
                std::vector<igIndex> cellIds;
                if (count > 0 && ids) {
                    cellIds.assign(ids, ids + count);
                }
                outputMesh->AddCell(cellIds.data(), count,
                                    inputMesh->GetSpecificCellType(i),
                                    inputMesh->GetCellOrder(i));
            }
            break;
        }
        case IG_POINT_SET:
        default:
            break;
    }

    return output;
}

IGAME_NAMESPACE_END
