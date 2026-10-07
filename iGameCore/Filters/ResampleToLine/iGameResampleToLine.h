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
 * 沿给定线段均匀生成采样点，为每个采样点定位其所在的单元并插值；定位 + 插值的语义与
 * VTK / ParaView 的 vtkProbeFilter（Plot Over Line）一致：FindCell → InterpolatePoint，
 * 命中单元时对 Point Data 线性/形函数插值、对 Cell Data 复制命中单元的值，未命中任何
 * 单元的采样点用 "validpointmask" 标记为无效并把各属性填 0。
 *
 * 输入：任意类型的网格（等价 vtkProbeFilter 接受任意 vtkDataSet），内部统一转换为
 *      UnstructuredMesh 后采样：
 *   1) UnstructuredMesh：直接使用；
 *   2) SurfaceMesh / VolumeMesh：按其面 / 体单元连接关系转换；
 *   3) StructuredMesh：按 (ni, nj, nk) 尺寸生成 0D 顶点 / 1D 线 / 2D 四边形 / 3D 六面体
 *      单元，覆盖 1D 曲线、2D 平面（含 (1, nj, nk) 这类退化排布）与 3D 结构化网格；
 *   4) LagrangeUnstructuredMesh：保留 IG_LAGRANGE_* 单元类型转换；
 *   5) PointSet（点云）：每个点生成一个 IG_VERTEX 单元（等价 vtkPolyData 的 verts）；
 *   6) 多块输入：输入对象通过子数据对象组织多个网格块（例如 CGNS 多 zone 的父对象）
 *      时逐块转换并合并为一个网格（等价 ParaView 先 Merge Blocks 再 Plot Over Line），
 *      采样点命中多个块时取序号靠前的块的数据。
 *
 * 单元插值方式：
 *   1) 0D 顶点单元：采样点落在容差内即取该点数据；
 *   2) 1D 线 / 折线 / 二次边 / Lagrange 曲线：逐段投影取最近的一段做线性插值；
 *   3) 线性面单元（三角形 / 四边形 / 多边形）：线性插值（重心坐标 / 双线性 / 扇形三角化）；
 *   4) 体单元（四面体 / 六面体 / 三棱柱 / 金字塔 / 多面体）：均值坐标（Mean Value
 *      Coordinates）插值；四面体的均值坐标即重心坐标，直接用解析解；
 *   5) 二次 / 高次单元：二次形函数 + 参数坐标 Newton 反解，覆盖 10 类：
 *      6 节点三角形 / 8 节点四边形 / 10 节点四面体 / 20 节点六面体 /
 *      15 节点三棱柱（楔形） / 13 节点金字塔 /
 *      9 节点双二次四边形 / 6 节点二次-线性四边形 / 12 节点二次-线性楔形 / 27 节点三二次六面体；
 *      其中后四类的节点序与形函数按 VTK 硬编码，与 vtkBiQuadraticQuad / vtkQuadraticLinearQuad /
 *      vtkQuadraticLinearWedge / vtkTriQuadraticHexahedron 源码逐行一致。
 * 仍未实现的类型（18 节点双二次-二次楔形、24 节点双二次-二次六面体、19 节点三二次金字塔、
 * 含棱中点的 QuadraticPolygon、Lagrange* 任意阶）退化处理：面单元按角点线性、一维单元按
 * 节点折线、体单元按「角点线性 + 均值坐标」，并在 GetMessage() 中给出 degraded high-order
 * cells 数量提示。
 *
 * 采样点不在任何单元内（超出容差）时不再吸附最近单元，而是标记为无效：
 * 新增点数据属性 validpointmask（UnsignedCharArray），1 = 该采样点可插值，0 = 无效；数组名可用
 * SetValidPointMaskArrayName() 修改：默认 "validpointmask"（与本工程 ResampleWithDataSet 一致），
 * 设为 "vtkValidPointMask" 即与 VTK 默认命名一致。
 * 无效采样点各插值属性填 0（与 VTK vtkProbeFilter 行为一致），有效点 id 可用 GetValidPoints() 取出。
 *
 * 单元包含判定的参数域容差与 VTK 一致：VTK EvaluatePosition 允许 pcoords ∈ [-0.001, 1.001]
 * （[0,1] 参数），因此贴单元边界的采样点与 VTK 得到相同的有效 / 无效判定。
 * 插值结果保留源数组的数据类型与分量数（整型 / 字符型数组同样按原类型承载并截断）。
 *
 * 容差默认自动计算（包围盒对角线 × 1e-6，紧容差），也可用 SetTolerance() 手动指定。
 *
 * 输出：
 *   output 0 : UnstructuredMesh 折线（IG_LINE 单元），与既有流程兼容;
 *   output 1 : SurfaceMesh 折线（点 + 边），真正的折线数据，可直接渲染为折线。
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

    /** 是否使用自动容差（默认开启） */
    void SetAutoTolerance(bool flag) { m_AutoTolerance = flag; }
    bool IsAutoTolerance() const { return m_AutoTolerance; }

    /**
     * 手动指定容差（相对于模型包围盒对角线长度）。
     * t <= 0 时恢复自动容差（包围盒对角线 × 1e-6）。
     */
    void SetTolerance(double t) {
        if (t > 0.0) {
            m_AutoTolerance = false;
            m_Tolerance = t;
        } else {
            m_AutoTolerance = true;
            m_Tolerance = -1.0;
        }
    }
    /** 手动容差；自动容差时返回 -1 */
    double GetTolerance() const { return m_AutoTolerance ? -1.0 : m_Tolerance; }
    /** 上一次 Execute() 实际使用的容差（绝对长度） */
    double GetEffectiveTolerance() const { return m_EffectiveTolerance; }

    std::string GetMessage() const { return m_Message; }

    /* ---------------- 结果查询 ---------------- */

    /** output 1: 折线（SurfaceMesh，点 + 边） */
    SurfaceMesh::Pointer GetPolyLine() const { return m_PolyLine; }
    /** output 0: 折线（UnstructuredMesh，IG_LINE 单元） */
    UnstructuredMesh::Pointer GetLineMesh() const { return m_LineMesh; }
    /** 每个采样点命中的单元 id，-1 表示无效（不在任何单元内） */
    const std::vector<igIndex>& GetSampleCellIds() const { return m_SampleCellIds; }
    /** 每个采样点是否有效（1 = 可插值，0 = 无效），与 validpointmask 属性一致 */
    const std::vector<unsigned char>& GetSampleValidMask() const { return m_SampleValidMask; }
    /** 有效采样点 id 列表（等价 VTK vtkProbeFilter::GetValidPoints()） */
    std::vector<igIndex> GetValidPoints() const;
    /** validpointmask 数组名；传空串恢复默认 "validpointmask" */
    void SetValidPointMaskArrayName(const std::string& maskName) {
        m_ValidPointMaskArrayName = maskName.empty() ? std::string("validpointmask") : maskName;
    }
    const std::string& GetValidPointMaskArrayName() const { return m_ValidPointMaskArrayName; }
    /** 本次执行中退化为线性角点处理的二次/高次单元数量 */
    IGsize GetUnsupportedQuadraticCellCount() const { return m_UnsupportedQuadraticCellCount; }
    /** 上一次 Execute() 的输入数据对象类型（IG_*，见 iGameType.h） */
    IGenum GetLastInputType() const { return m_LastInputType; }
    /** 上一次 Execute() 实际参与采样的网格块数量（多块输入 > 1，等价 Merge Blocks 后的块数） */
    int GetLastInputBlockCount() const { return m_LastInputBlockCount; }

private:
    /** 单个采样点的定位与插值权重 */
    struct SampleLocation {
        igIndex cellId{-1};            // 命中的单元 id，-1 = 无效
        Point point{0.f, 0.f, 0.f};    // 采样点
        std::vector<igIndex> pointIds; // 命中单元的点 id（与权重一一对应）
        std::vector<double> weights;   // 插值权重
    };

    /* ---------------- 几何搜索 ---------------- */
    std::vector<std::vector<igIndex>> BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                       const BoundingBox& bbox, igIndex nx, igIndex ny, igIndex nz);
    bool LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                      const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                      double distTol, SampleLocation& out);

    /* ---------------- 单元权重 ---------------- */
    bool ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                            std::vector<double>& weights);
    bool ComputeLinearFaceWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                  std::vector<double>& weights);
    /** 0D 顶点单元 / 1D 线、折线、二次边、Lagrange 曲线单元：点包含判定与线性权重 */
    bool ComputeLineWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                            std::vector<double>& weights);
    bool ComputeMeanValueWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                 std::vector<double>& weights);
    bool ComputeQuadraticWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol,
                                 std::vector<double>& weights);
    /** 高次体单元退化：用线性角点单元的面表做均值坐标插值 */
    bool ComputeDegradedVolumeWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                     double distTol, std::vector<double>& weights);
    bool IsPointInVolumeCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, double distTol);
    double DistanceToCellBoundary(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p, Point& closest);

    /* ---------------- 属性处理 ---------------- */
    void InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                              const std::vector<SampleLocation>& locations, int sampleNum);
    void CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                      const std::vector<SampleLocation>& locations, int sampleNum);
    void AddValidPointMask(AttributeSet::Pointer outSet, int sampleNum);

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

    // 容差：默认自动（包围盒对角线 × kAutoToleranceRatio）
    bool m_AutoTolerance{true};
    double m_Tolerance{-1.0};
    double m_EffectiveTolerance{0.0};

    std::string m_Message{"Not Executed !"};
    IGsize m_UnsupportedQuadraticCellCount{0};
    IGenum m_LastInputType{IG_NONE};
    int m_LastInputBlockCount{0};

    SurfaceMesh::Pointer m_PolyLine{};
    UnstructuredMesh::Pointer m_LineMesh{};
    std::vector<igIndex> m_SampleCellIds;
    std::vector<unsigned char> m_SampleValidMask;
    std::string m_ValidPointMaskArrayName{"validpointmask"};
};

IGAME_NAMESPACE_END
#endif
