#pragma once
#ifndef iGameResampleToLine_h
#define iGameResampleToLine_h

#include "iGameCellCenter.h"
#include "iGameDrawObject.h"
#include "iGameFilter.h"
#include "iGamePointSet.h"
#include "iGameSceneManager.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include "iGameVolumeMesh.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 重采样至直线（Resample to line）
 *
 * 沿给定线段均匀生成采样点，为每个采样点定位其所在的（体）单元：
 *   1) 优先查找真正包含该采样点的单元；
 *   2) 若没有单元包含它，则吸附到最近的单元（可用 SetSnapToNearestCell 关闭）。
 * 定位到单元后使用该单元的形函数 / 参数坐标（自然坐标）插值所有数值型 Point Data，
 * 保留数组的分量数并选择合理的数据类型；同时把命中单元的 Cell Data 复制到对应采样点。
 *
 * 输出：
 *   output 0 : UnstructuredMesh 折线（IG_LINE 单元），与既有流程兼容;
 *   output 1 : SurfaceMesh 折线（点 + 边），真正的折线数据，可直接渲染为折线。
 *
 * 采样点坐标保持在线段上（即"沿直线重采样"），命中单元信息可通过
 * GetSampleCellIds() / GetSampleCellInside() 查询。
 */
class ResampleToLine : public Filter {
public:
    I_OBJECT(ResampleToLine);
    static Pointer New() { return new ResampleToLine; }

    bool Execute() override;

    /* ---------------- 参数设置 ---------------- */

    /** 设置采样线段：起点 p0、终点 p1、采样点数量 x */
    void setOrigTarget(const Point& p0, const Point& p1, const int& x) {
        orig = p0;
        target = p1;
        n = x;
    }
    void setOrigTarget(const Vector3d& p0, const Vector3d& p1, const int& x);
    void SetOrigTarget(const Point& p0, const Point& p1) {
        orig = p0;
        target = p1;
    }
    void SetSampleNumber(int x) { n = x; }
    int GetSampleNumber() const { return n; }

    /** 没有单元包含采样点时，是否吸附到最近的单元（默认开启） */
    void SetSnapToNearestCell(bool flag) { m_SnapToNearest = flag; }
    bool GetSnapToNearestCell() const { return m_SnapToNearest; }

    /** 单元判定容差（相对于模型包围盒对角线长度，默认 1e-4） */
    void SetTolerance(double t) { m_Tolerance = t; }
    double GetTolerance() const { return m_Tolerance; }

    std::string GetMessage() const { return m_Message; }

    /* ---------------- 结果查询 ---------------- */

    /** output 1: 折线（SurfaceMesh，点 + 边） */
    SurfaceMesh::Pointer GetPolyLine() const { return m_PolyLine; }
    /** output 0: 折线（UnstructuredMesh，IG_LINE 单元） */
    UnstructuredMesh::Pointer GetLineMesh() const { return m_LineMesh; }
    /** 每个采样点命中的单元 id，-1 表示没有可用的单元 */
    const std::vector<igIndex>& GetSampleCellIds() const { return m_SampleCellIds; }
    /** 每个采样点是否真正落在单元内部（0/1） */
    const std::vector<unsigned char>& GetSampleCellInside() const { return m_SampleCellInside; }

private:
    /** 单个采样点的定位与插值权重 */
    struct SampleLocation {
        igIndex cellId{-1};            // 命中的单元 id
        bool inside{false};            // 是否真正位于单元内部
        Point point{0.f, 0.f, 0.f};    // 采样点（未命中时为其在最近单元上的投影点）
        std::vector<igIndex> pointIds; // 命中单元的点 id（与权重一一对应）
        std::vector<double> weights;   // 形函数权重
    };

    /* ---------------- 几何搜索 ---------------- */
    std::vector<std::vector<igIndex>> BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                       const BoundingBox& bbox, igIndex nx, igIndex ny, igIndex nz);
    bool LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                      const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                      double distTol, SampleLocation& out);
    bool ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                            double distTol, std::vector<double>& weights, bool& inside);
    double DistanceToCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, Point& closest);

    /* ---------------- 属性处理 ---------------- */
    void InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                              const std::vector<SampleLocation>& locations, int sampleNum);
    void CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                      const std::vector<SampleLocation>& locations, int sampleNum);

    /* ---------------- 输出构建 ---------------- */
    void BuildPolyLineOutputs(const Points::Pointer& samples, AttributeSet::Pointer attrSet, int sampleNum);

    ResampleToLine() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(2);
    }
    ~ResampleToLine() override = default;

    Point orig{-1.0f, -0.983795f, -0.35714f};
    Point target{1.0f, 0.983795f, 0.35714f};
    int n{40};

    double m_Tolerance{1e-4};
    bool m_SnapToNearest{true};

    std::string m_Message{"Not Unstructured Mesh !"};

    SurfaceMesh::Pointer m_PolyLine{};
    UnstructuredMesh::Pointer m_LineMesh{};
    std::vector<igIndex> m_SampleCellIds;
    std::vector<unsigned char> m_SampleCellInside;
};

IGAME_NAMESPACE_END
#endif
