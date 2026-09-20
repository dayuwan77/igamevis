/**
 * @class    iGameIsoVolumeFilter
 * @brief    等值面之间的体提取
 * 根据标量场的范围 [Lower, Upper] 提取体网格中数值在该区间内的部分。
 * 完全在内的单元直接保留；与边界相交的单元会被裁剪，生成新的非结构化网格。
 */

#ifndef iGameIsoVolumeFilter_h
#define iGameIsoVolumeFilter_h

#include "../Clip/iGameCellClip.h"
#include <functional>
#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class IsoVolumeFilter : public Filter {
public:
    I_OBJECT(IsoVolumeFilter);
    static Pointer New() { return new IsoVolumeFilter; }
    ~IsoVolumeFilter();

    bool Execute() override;

    // 返回提取后的非结构化网格
    UnstructuredMesh::Pointer GetOutputMesh() { return DynamicCast<UnstructuredMesh>(this->GetOutput()); }

    // 设置用于区间判断的标量数组以及 [lower, upper] 范围
    // dimension 表示使用数组的第几个分量（默认 0）
    void SetIsoScalarData(ArrayObject::Pointer array, double lower, double upper, int dimension = 0);

    // 进度回调(可选): 每到一个进度节点被调用一次, 由 GUI 注入。
    // 核心库不依赖 Qt —— 这是"只影响本次提取"的关键。
    void SetProgressNotify(std::function<void(double)> cb) { m_ProgressNotify = std::move(cb); }

    // 时序数据(.pvd 等容器对象): 默认(-1)使用容器【当前已加载】的帧 ——
    // 即框架动画控件正在显示的那一帧(打开 .pvd 时为第一帧);
    // 传 n >= 0 可显式指定帧号(越界钳制到最后一帧)。
    void SetTimeStep(int timeStep) { m_TimeStep = timeStep; }
    int GetTimeStep() const { return m_TimeStep; }

protected:
    IsoVolumeFilter();

    ArrayObject::Pointer m_SelectedScalar{nullptr};
    std::string m_SelectedScalarName;
    double m_LowerValue{0.0};
    double m_UpperValue{0.0};
    double m_SelectDimension{0.0};
    std::function<void(double)> m_ProgressNotify;
    int m_TimeStep{-1};  // <0: 用容器当前帧(默认); >=0: 显式指定帧号

    bool ExecuteWithUnstructuredMesh(UnstructuredMesh::Pointer um);
    bool ExecuteWithVolumeMesh(VolumeMesh::Pointer vm);
    bool ExecuteWithSurfaceMesh(SurfaceMesh::Pointer sm);
    bool ExecuteWithVolumeMeshWithPolyhedronType(VolumeMesh::Pointer vm);

    // 在单个网格上完成"按名称解析点标量 + 两趟裁剪", 结果写入 output
    bool ExtractToMesh(UnstructuredMesh::Pointer input, UnstructuredMesh::Pointer output);

    // 把多个分块的提取结果合并成一个网格(多分块帧使用)
    UnstructuredMesh::Pointer MergeParts(const std::vector<UnstructuredMesh::Pointer>& parts);

public:
    // 内部点(裁剪片段内部的"心点")的插值信息
    // 说明: case 表中部分输出单元含"单元内部点"(心扇的顶点);
    //       其位置/属性由该 case 边界点按权重合成, 保证落在片段内部。
    struct InteriorInterp {
        igIndex pointId{-1};             // 输出点 id
        std::vector<igIndex> v;          // 参与的原始点 id
        std::vector<double> w;           // 对应权重(和为 1)
    };

private:

    // 表驱动的单元裁剪(tet / wedge / pyramid / hex 共用)
    void ClipCellByTable(Cell::Pointer cell, int cellType, const double* values, bool keepAbove,
                         Points::Pointer points, CellArray::Pointer connectivity,
                         UnsignedIntArray::Pointer types, igIndex cellId,
                         std::vector<CellClip::InterpolateEdge>& OriginEdge,
                         std::vector<igIndex>& originCell,
                         std::vector<InteriorInterp>& interiorPts);

    // 计算每个顶点相对于给定等值面的带符号距离，以及每个 cell 的在内/在外/相交状态
    void ComputePointValueAndCellVisible(Points::Pointer inPoints, CellArray::Pointer inCells,
                                         DoubleArray::Pointer PointIsoArray, CharArray::Pointer CellVisible,
                                         ArrayObject::Pointer scalarArray, double isoValue, bool keepAbove);

    // 对输入网格按单个标量阈值进行裁剪
    // keepAbove = true 保留 scalar >= isoValue 的部分
    // keepAbove = false 保留 scalar <= isoValue 的部分
    bool ClipMeshByScalar(UnstructuredMesh::Pointer input, ArrayObject::Pointer scalarArray, double isoValue,
                          bool keepAbove, UnstructuredMesh::Pointer output);

    // 复制属性数据，对裁剪产生的新点进行插值
    void CopyAttributeSetData(igIndex outPointNum, igIndex outCellNum, AttributeSet::Pointer inData,
                              AttributeSet::Pointer outData, std::vector<CellClip::InterpolateEdge> OriginEdge,
                              std::vector<igIndex> OriginCell, const std::vector<InteriorInterp>& interiorPts);
};

IGAME_NAMESPACE_END
#endif
