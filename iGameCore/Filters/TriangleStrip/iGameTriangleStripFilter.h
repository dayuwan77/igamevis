#ifndef iGameTriangleStripFilter_h
#define iGameTriangleStripFilter_h

#include "iGameCellArray.h"
#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <array>
#include <cstdint>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class TriangleStripFilter
 * @brief 将相邻三角形组织为三角带，并将显式线段组织为折线。
 *
 * 三角带部分以 vtkStripper 的逐面访问流程为基础，同时借鉴 GLU
 * render.c 中 FaceCount、临时 trail 标记和多起始方向比较的结构。
 *
 * @warning SurfaceMesh::Faces 中的变长单元表示普通多边形，不能用来保存
 * 三角带。因此本类将三角带单独保存在 m_Strips 中。要把结果接入通用
 * DataObject/渲染管线，还需要数据模型提供独立的 Strips 容器，或者增加
 * IG_TRIANGLE_STRIP 单元类型。
 */
class TriangleStripFilter : public Filter {
public:
    I_OBJECT(TriangleStripFilter);
    static Pointer New() { return new TriangleStripFilter; }

    /**
     * 执行流程：准备表面输入、构造邻接、生成 strips/折线、构造输出对象。
     */
    bool Execute() override;

    /**
     * 设置单条三角带允许包含的最大三角形数。
     * 对折线表示最大线段数；默认值与 vtkStripper 相同，为 1000。
     */
    void SetMaximumLength(int length);
    int GetMaximumLength() const noexcept { return m_MaximumLength; }

    /**
     * 控制是否在折线生成后继续合并首尾点 ID 相同的连续折线。
     * 该选项只影响显式 IG_LINE/IG_POLY_LINE，不连接三角带。
     */
    void SetJoinContiguousSegments(bool enabled) noexcept { m_JoinContiguousSegments = enabled; }
    bool GetJoinContiguousSegments() const noexcept { return m_JoinContiguousSegments; }

    /** 每个 CellArray 单元是一条三角带，含 T 个三角形的带有 T+2 个点 ID。 */
    CellArray* GetStrips() const noexcept { return m_Strips.get(); }

    /** 未参与 strip 的非三角形面，语义与 vtkStripper 的 pass-through polys 相同。 */
    CellArray* GetPassThroughPolys() const noexcept { return m_PassThroughPolys.get(); }

    /** 由输入的显式线单元生成的折线；不包含 SurfaceMesh 自动构造的边。 */
    CellArray* GetPolyLines() const noexcept { return m_PolyLines.get(); }

    /**
     * 第 i 项保存第 i 条 strip 中各三角形对应的输入 faceId，顺序与 strip
     * 产生三角形的顺序一致，可用于传递原始 cell data 或生成 OriginalCellIds。
     */
    const std::vector<std::vector<igIndex>>& GetStripSourceFaceIds() const noexcept {
        return m_StripSourceFaceIds;
    }

    IGsize GetNumberOfStrips() const noexcept;
    IGsize GetLongestStripLength() const noexcept { return m_LongestStripLength; }

protected:
    TriangleStripFilter();
    ~TriangleStripFilter() override = default;

private:
    /**
     * iGame 中对 GLUhalfEdge 的轻量替代。
     * LocalEdge 表示三角形局部有向边：point[LocalEdge] ->
     * point[(LocalEdge + 1) % 3]。
     */
    struct OrientedEdge {
        igIndex FaceId{-1};
        int LocalEdge{-1};

        bool IsValid() const noexcept { return FaceId >= 0 && LocalEdge >= 0 && LocalEdge < 3; }
    };

    /**
     * 对应 GLU render.c 中 FaceCount 的扩展形式。
     * PointIds 是可直接交给 GL_TRIANGLE_STRIP 的有序点 ID；FaceIds 既给出
     * 候选长度，也保存输入三角形映射。
     */
    struct StripCandidate {
        std::vector<igIndex> PointIds;
        std::vector<igIndex> FaceIds;
        OrientedEdge StartEdge;

        IGsize GetTriangleCount() const noexcept { return static_cast<IGsize>(FaceIds.size()); }
        bool Empty() const noexcept { return FaceIds.empty(); }
    };

    /**
     * Free：尚未处理；Trial：当前候选临时占用；Committed：已输出。
     * Trial 配合 trail 使用，使不同起始方向可以试探后恢复，等价于 GLU 的
     * AddToTrail/FreeTrail。
     */
    enum class FaceMark : std::uint8_t {
        Free = 0,
        Trial,
        Committed,
    };

    // --------------------------- Execute phases ---------------------------

    /** 清空上次执行产生的输出数组、统计值和访问标记。 */
    void ResetWorkingState();

    /**
     * 将 SurfaceMesh 或只含二维单元的 UnstructuredMesh 准备为 m_InputMesh。
     * 混合/体网格应先走表面提取；非三角形面在 vtkStripper 模式下原样传递。
     */
    bool PrepareInput();

    /** 构造 face->edge 和 edge->face 邻接，为共享边查询提供支持。 */
    bool BuildTriangleAdjacency();

    /** 遍历所有未提交三角面，为每个种子面生成并提交最佳 strip。 */
    bool BuildTriangleStrips();

    /**
     * 处理 UnstructuredMesh 中的显式 IG_LINE/IG_POLY_LINE；MaximumLength
     * 对此表示最大线段数。
     */
    bool BuildPolyLines();

    /**
     * vtkStripper 风格的折线后处理：比较折线首尾点 ID，必要时反转后拼接。
     */
    void JoinContiguousPolyLines();

    /**
     * 将 m_Strips、m_PassThroughPolys、m_PolyLines 和共享 Points 组装为过滤器
     * 输出。实现前必须先确定 iGame 的三角带持久化类型。
     */
    bool BuildOutputDataObject();

    // ----------------------- Triangle-strip search ------------------------

    /**
     * 从种子三角形的三个有向边分别试探，返回覆盖未访问三角形最多的候选。
     * 这是对 vtkStripper 单路径贪心的 GLU MaximumStrip 风格增强。
     */
    StripCandidate FindBestStrip(igIndex seedFaceId);

    /**
     * 从给定有向边建立候选，沿当前 strip 尾部共享边不断加入邻接三角形，
     * 直到拓扑中断、遇到已访问面或达到 MaximumLength。
     */
    StripCandidate TraceStrip(const OrientedEdge& startEdge);

    /** 将候选写入 m_Strips，并把候选中的 face 标记为 Committed。 */
    void CommitStrip(StripCandidate&& candidate);

    /** 将一个面临时标为 Trial 并记录到 trail，若不可标记则返回 false。 */
    bool AddFaceToTrial(igIndex faceId, std::vector<igIndex>& trail);

    /** 撤销本次试探产生的 Trial 标记，不影响已经 Committed 的面。 */
    void FreeTrial(std::vector<igIndex>& trail);

    // -------------------------- Topology helpers --------------------------

    bool IsTriangleFace(igIndex faceId) const;
    bool GetTrianglePointIds(igIndex faceId, std::array<igIndex, 3>& pointIds) const;
    bool GetOrientedEdgePointIds(const OrientedEdge& edge, igIndex& origin, igIndex& destination) const;
    igIndex GetMeshEdgeId(const OrientedEdge& edge) const;

    /**
     * 返回共享 edgeId 的另一个可用三角面。边界边返回 -1；对非流形边只在
     * 能唯一确定可用邻面时继续，否则终止当前 strip。
     */
    igIndex FindAvailableNeighbor(igIndex edgeId, igIndex currentFaceId) const;

    /** 返回三角面中不属于共享边 (edgePoint0, edgePoint1) 的第三个点。 */
    igIndex FindThirdPoint(igIndex faceId, igIndex edgePoint0, igIndex edgePoint1) const;

    /** 在 faceId 中查找 origin->destination 对应的局部有向边。 */
    OrientedEdge FindOrientedEdge(igIndex faceId, igIndex origin, igIndex destination) const;

    /** 将非三角形面复制到 m_PassThroughPolys。 */
    void PassThroughPolygon(igIndex faceId);

    /**
     * 为展开后的 SurfaceMesh 创建独立 AttributeSet。点属性保持原数组，
     * 单元属性按照 outputSourceFaceIds 重排到输出面顺序。
     */
    bool BuildOutputAttributes(const std::vector<igIndex>& outputSourceFaceIds,
                               AttributeSet::Pointer& outputAttributes) const;

private:
    DataObject::Pointer m_SourceInput{};
    SurfaceMesh::Pointer m_InputMesh{};
    UnstructuredMesh::Pointer m_UnstructuredInput{};

    CellArray::Pointer m_Strips{};
    CellArray::Pointer m_PassThroughPolys{};
    CellArray::Pointer m_PolyLines{};

    std::vector<FaceMark> m_FaceMarks;
    std::vector<std::vector<igIndex>> m_StripSourceFaceIds;
    std::vector<igIndex> m_PassThroughPolySourceFaceIds;

    int m_MaximumLength{1000};
    bool m_JoinContiguousSegments{false};
    IGsize m_LongestStripLength{0};
};

IGAME_NAMESPACE_END

#endif
