#ifndef iGameCleanToGridFilter_h
#define iGameCleanToGridFilter_h

#include "iGameFilter.h"
#include "iGamePointFinder.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class CleanToGridFilter : public Filter {
public:
    I_OBJECT(CleanToGridFilter);
    static Pointer New() { return new CleanToGridFilter; }

    CleanToGridFilter();
    ~CleanToGridFilter() override;

    bool Execute() override;

    // 参数设置函数
    void SetAbsoluteTolerance(double tol) { m_AbsoluteTolerance = tol; }
    double GetAbsoluteTolerance() const { return m_AbsoluteTolerance; }

    void SetToleranceIsAbsolute(bool isAbsolute) { m_ToleranceIsAbsolute = isAbsolute; }
    bool GetToleranceIsAbsolute() const { return m_ToleranceIsAbsolute; }

    void SetToleranceFraction(double frac) { m_ToleranceFraction = frac; }
    double GetToleranceFraction() const { return m_ToleranceFraction; }

    void SetMergePoints(bool merge) { m_MergePoints = merge; }
    bool GetMergePoints() const { return m_MergePoints; }

    void SetCompactPointFields(bool compact) { m_CompactPointFields = compact; }
    bool GetCompactPointFields() const { return m_CompactPointFields; }

    void SetRemoveDegenerateCells(bool remove) { m_RemoveDegenerateCells = remove; }
    bool GetRemoveDegenerateCells() const { return m_RemoveDegenerateCells; }

protected:
    double ComputeEffectiveTolerance(DataObject::Pointer input);

    // 使用暴力搜索合并重合点
    bool MergeCoincidentPointsBruteForce(Points::Pointer points, double tolerance, std::vector<igIndex>& oldToNewMap,
                                         igIndex& newPointCount, const std::vector<bool>& pointIsUsed);

    bool IsCellDegenerateWithIds(const igIndex* pointIds, int numPoints);

private:
    double m_AbsoluteTolerance = 0.001;
    double m_ToleranceFraction = 1e-6;
    bool m_ToleranceIsAbsolute = true;
    bool m_MergePoints = true;
    bool m_CompactPointFields = true;
    bool m_RemoveDegenerateCells = true;
};

IGAME_NAMESPACE_END
#endif