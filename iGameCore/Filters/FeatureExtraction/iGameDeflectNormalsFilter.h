#ifndef iGameDeflectNormalsFilter_h
#define iGameDeflectNormalsFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVector.h"
#include <string>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 按给定向量场对基准法向做线性偏转并归一化，输出点向量 "DeflectedNormals"。
 *
 * 修复点（相对初版）：
 *   ② 支持同名数组在 POINT/CELL 上的区分：通过 SetVectorFieldAttachment(IG_POINT / IG_CELL / -1)
 *      控制属性查找来源：
 *        - 指定 IG_POINT / IG_CELL  → 精确按 (attachmentType, name) 查找
 *        - 指定 IG_NONE = Auto 模式 → 先找 POINT，找不到再找 CELL，Cell 属性会做 cell→point
 *          的邻接单元平均，确保输出 ND 仍然每点一个向量。
 *   同时仍然保留 VolumeMesh/StructuredMesh 的表面化流程（走 GetRenderableObject），
 *   使法向计算链路与 SurfaceMesh 一致。
 */
class DeflectNormalsFilter : public Filter {
public:
    I_OBJECT(DeflectNormalsFilter);
    static Pointer New() { return new DeflectNormalsFilter; }
    ~DeflectNormalsFilter();

    // 向量场来源：两种方式二选一
    void SetAttributeByIndex(int idx) noexcept { _attrIdx = idx; }
    void SetAttributeByName(const std::string& name) { _attrName = name; }

    /** 向量场属性的挂载位置（可选）：
     *   IG_POINT -> 只在 PointData 中查找 name/index 对应的 3 分量属性
     *   IG_CELL  -> 只在 CellData 中查找；找到后会做 cell->point（邻接单元平均）
     *   IG_NONE  -> Auto：先 Point -> 再 Cell（审核问题②的默认安全值） */
    void SetVectorFieldAttachment(IGenum attach) noexcept { _vfAttach = attach; }

    void SetDeflectStrength(float s) noexcept { _strength = s; }

    // false：按曲面法向平均求 base；true：使用 userNormal 作为常数 base
    void SetUseUserNormal(bool use) noexcept { _useUserNormal = use; }
    void SetUserNormal(const Vector3f& n) noexcept { _userNormal = n; }
    void SetUserNormal(double x, double y, double z) noexcept { _userNormal = Vector3f(float(x), float(y), float(z)); }

    std::string GetMessage() const { return _msg; }

    bool Execute() override;

protected:
    DeflectNormalsFilter();

    // 把基于 cells 的属性通过邻接单元平均转到 point 维度
    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum);

    // 在 attrSet 中按 (_vfAttach, _attrIdx / _attrName) 精确定位向量属性
    // 找到后返回 true，并把 attachment / 属性引用 / Cell->Point 是否需要转换回传
    bool ResolveVectorField(AttributeSet* attrSet,
                            IGenum& outAttach, AttributeSet::Attribute*& outAttr,
                            bool& outNeedCellToPoint, std::string& why);

    int                _attrIdx{ -1 };
    std::string        _attrName;
    IGenum             _vfAttach{ IG_NONE };       // 默认 Auto：POINT -> CELL 兜底
    float              _strength{ 1.0f };
    bool               _useUserNormal{ false };
    Vector3f           _userNormal{ 0.f, 0.f, 1.f };
    std::string        _msg{ "Not Surface Mesh!" };
};

IGAME_NAMESPACE_END
#endif