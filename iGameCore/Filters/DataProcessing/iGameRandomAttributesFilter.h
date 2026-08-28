
#ifndef iGameRandomAttributesFilter_h 
#define iGameRandomAttributesFilter_h

#include "iGameFilter.h"          
#include "iGameSurfaceMesh.h"      
#include "iGameUnstructuredMesh.h" 
#include <random>           
#include <string>  
#include <type_traits>              
#include <cstdint>   


IGAME_NAMESPACE_BEGIN

class RandomAttributesFilter : public Filter { 
public:
	I_OBJECT(RandomAttributesFilter);
    static Pointer New() { return new RandomAttributesFilter; }
    ~RandomAttributesFilter();

     // 设置随机数的取值范围 [min, max]
    void SetRange(float set_Min, float set_Max)
    {
        min = set_Min; // 保存下限
        max = set_Max; // 保存上限
    }

    // 设置随机数的类型
    void SetDataType(IGenum type) { dataType = type; }

     // 设置随机数挂的对象
    void SetAttachmentType(IGenum type) { attachmentType = type; }

    // 设置随机数种子
    void SetSeed(unsigned int set_Seed) { seed = set_Seed; }

    // 设置随机数属性的名称
    void SetAttributeName(const std::string& setName) { name = setName; }

    bool Execute() override;

protected:
    RandomAttributesFilter();

    //按类型创建数组对象；返回空指针表示类型不支持
    template<typename TArray, typename TValue>
    ArrayObject::Pointer CreateTypedArray(IGsize num, double lo, double hi, unsigned int seed);

    // 把 dataType 对应的 IG_* 常量映射到具体数组类；返回空指针表示类型不支持
    ArrayObject::Pointer CreateDataArray(IGsize num, double lo, double hi, unsigned int seed);

    float min, max;      //随机数上下限
    IGenum dataType;
    IGenum attachmentType; //随机数挂的对象的类型
    unsigned int seed;     //随机数种子
    std::string name;
};

IGAME_NAMESPACE_END
#endif