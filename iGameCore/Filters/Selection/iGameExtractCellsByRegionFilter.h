#pragma once
#include <iGameBoundingBox.h>
#include <iGameDataObject.h>
#include <iGameFilter.h>
#include <iGamePoints.h>
#include <iGameUnstructuredMesh.h>
#include <vector>
IGAME_NAMESPACE_BEGIN
class ExtractCellsByRegionFilter : public Filter {
public:
    I_OBJECT(ExtractCellsByRegionFilter);
    static Pointer New() { return new ExtractCellsByRegionFilter; }

    void SetBox(const Vector3d& min, const Vector3d& max);
    void SetSphere(const Vector3d& center, double radius);
    void SetRequireAllPoints(bool requireAllPoints);

    bool Execute() override;

protected:
    ExtractCellsByRegionFilter();
    ~ExtractCellsByRegionFilter() override = default;

private:
    bool IsPointInRegion(const Vector3d& p) const;
    void BuildOutputMesh();
    // 按 oldToNew（原网格点号 -> 输出网格点号）把输入的点/单元属性搬运到输出网格
    void CopyAttributeDataToOutput(const UnstructuredMesh::Pointer& outMesh, const std::vector<int>& oldToNew);

    /* 区域参数 */
    enum RegionType { BOX, SPHERE };
    RegionType m_RegionType{BOX};
    BoundingBox m_Box;                 // BOX 用（默认构造 = setNull）
    Vector3d m_Center{0.0, 0.0, 0.0};  // SPHERE 用（显式清零，Vector 默认构造不初始化）
    double m_Radius{0.0};              // SPHERE 用
    bool m_RequireAllPoints{true};     // true=严格（默认）；false=宽松

    /* 输入/输出 */
    UnstructuredMesh::Pointer m_Mesh;
    UnstructuredMesh::Pointer m_OutputMesh;
    std::vector<int> m_Ids;
};
IGAME_NAMESPACE_END
