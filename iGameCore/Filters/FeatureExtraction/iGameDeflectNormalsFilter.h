
#ifndef iGameDeflectNormalsFilter_h
#define iGameDeflectNormalsFilter_h

#include "iGameFilter.h"            
#include "iGameSurfaceMesh.h"       
#include "iGameUnstructuredMesh.h"  
#include "iGameVector.h"           


IGAME_NAMESPACE_BEGIN

class DeflectNormalsFilter : public Filter {
public:
    I_OBJECT(DeflectNormalsFilter);
    static Pointer New() { return new DeflectNormalsFilter; }
    ~DeflectNormalsFilter();

    // 按索引指定输入向量场属性（与按名二选一）
    void SetAttributeByIndex(int index) { curIndex = index; }
    // 按名称指定输入向量场属性（常用）
    void SetAttributeByName(const std::string& name) { this->name = name; }

    // 设置偏转强度 strength（向量场对法向的影响程度），默认 1.0
    void SetDeflectStrength(float setStrength) { strength = setStrength; }

    // 是否用用户指定的常数法向替代"由曲面计算出的法向"。默认 false（用曲面法向）。
    void SetUseUserNormal(bool use) { ifUseUserNormal = use; }

    // 用户指定的常数法向（当 UseUserNormal = true 时作为偏转的基准法向）。
    void SetUserNormal(const Vector3f& n) { userNormal = n; }
    void SetUserNormal(double x, double y, double z) { userNormal = Vector3f(float(x), float(y), float(z)); }

    std::string GetMessage() const { return errorMessage; }

    bool Execute() override;

protected:
    DeflectNormalsFilter();

    // 将 CellArray 中的属性转换为 PointArray 中的属性
    ArrayObject::Pointer AttributeCell2Point(CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum);

    int curIndex{-1}; // 输入属性索引（-1 表示未指定）
    std::string name; // 输入属性名（为空表示未指定）
    float strength; // 偏转强度 strength（向量场对法向的影响）
    bool ifUseUserNormal; 
    Vector3f userNormal;

    std::string errorMessage{"Not Surface Mesh !"};
};

IGAME_NAMESPACE_END
#endif