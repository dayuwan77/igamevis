#ifndef iGameDataObjectCopy_h
#define iGameDataObjectCopy_h

#include "iGameAttributeSet.h"
#include "iGameDataObject.h"
#include "iGameSmartPointer.h"

IGAME_NAMESPACE_BEGIN

/**
 * @brief 按类型深拷贝一个 DataObject（点、拓扑、单元类型与属性全部独立）。
 *
 * 支持的数据对象类型：
 *   - UnstructuredMesh（单元 + 单元类型）
 *   - SurfaceMesh（面片 + 边）
 *   - VolumeMesh（体单元；多面体网格会同时重建面片与「体-面」索引）
 *   - StructuredMesh（维度尺寸 + 面片 + 边）
 *   - LagrangeUnstructuredMesh（单元点序 + 单元类型 + 阶次）
 *   - PointSet（仅点坐标与属性）
 *
 * @param input      被复制的对象。
 * @param attributes 输出对象使用的属性集：
 *                   - 传 nullptr：深拷贝 input 自身的属性集（默认行为）；
 *                   - 传非空：输出直接使用该属性集。用于「几何/拓扑取自 A、
 *                     属性来自另一次计算结果」的场景（例如 ResampleWithDataSet
 *                     把被采样网格的插值结果挂到采样点网格的深拷贝上）。
 * @return 新的同类型 DataObject；类型不支持或转换失败时返回 nullptr。
 *
 * @note 输出的名字沿用 input 的名字，调用方如需改名请自行 SetName()。
 */
DataObject::Pointer DeepCopyDataObject(DataObject::Pointer input, AttributeSet::Pointer attributes = nullptr);

IGAME_NAMESPACE_END
#endif
