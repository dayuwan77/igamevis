#include "iGameRandomAttributesFilter.h"
#include <ctime>

IGAME_NAMESPACE_BEGIN

RandomAttributesFilter::RandomAttributesFilter()
{
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    this->max = 1.0f;
    this->min = 0.0f;
    this->seed = static_cast<unsigned>(std::time(nullptr));
    this->attachmentType = IG_POINT;
    this->dataType = IG_FLOAT;
}

RandomAttributesFilter::~RandomAttributesFilter() {}


 template<typename TArray, typename TValue>
ArrayObject::Pointer RandomAttributesFilter::CreateTypedArray(IGsize num, double lo, double hi, unsigned int seed) {
    auto arr = TArray::New(); 
    arr->SetDimension(1);     
    arr->Reserve(num);        

    std::mt19937 gen(seed); 

    if constexpr (std::is_floating_point_v<TValue>) 
    {
        // ------ 浮点类型：用均匀实数分布，结果保留小数 ------
        std::uniform_real_distribution<double> dist(lo, hi);
        for (IGsize i = 0; i < num; ++i) 
            arr->AddValue(static_cast<TValue>(dist(gen))); // 转成对应类型存入
    } 
    else 
    {
        // ------ 整数类型：用均匀整数分布，结果为整数 ------
        long long loI = static_cast<long long>(lo); 
        long long hiI = static_cast<long long>(hi); 
        if (hiI < loI) 
            hiI = loI;                   
        std::uniform_int_distribution<long long> dist(loI, hiI);
        for (IGsize i = 0; i < num; ++i) 
            arr->AddValue(static_cast<TValue>(dist(gen))); // 转换成目标整数类型
    }
    return arr; 
}

ArrayObject::Pointer RandomAttributesFilter::CreateDataArray(IGsize num, double lo, double hi, unsigned int seed) {
    switch (dataType) 
    {
        case IG_CHAR:
            return CreateTypedArray<CharArray, char>(num, lo, hi, seed);
        case IG_UNSIGNED_CHAR:
            return CreateTypedArray<UnsignedCharArray, unsigned char>(num, lo, hi, seed);
        case IG_SHORT:
            return CreateTypedArray<ShortArray, short>(num, lo, hi, seed);
        case IG_UNSIGNED_SHORT:
            return CreateTypedArray<UnsignedShortArray, unsigned short>(num, lo, hi, seed);
        case IG_INT: 
        case IG_INDEX:
            return CreateTypedArray<IntArray, int>(num, lo, hi, seed);
        case IG_UNSIGNED_INT:
            return CreateTypedArray<UnsignedIntArray, unsigned int>(num, lo, hi, seed);
        case IG_LONG_LONG:
            return CreateTypedArray<LongLongArray, long long>(num, lo, hi, seed);
        case IG_UNSIGNED_LONG_LONG:
            return CreateTypedArray<UnsignedLongLongArray, unsigned long long>(num, lo, hi, seed);
        case IG_FLOAT:
            return CreateTypedArray<FloatArray, float>(num, lo, hi, seed);
        case IG_DOUBLE:
            return CreateTypedArray<DoubleArray, double>(num, lo, hi, seed);
        default:
            return nullptr; 
    }
}


bool RandomAttributesFilter::Execute()
{
    auto input = GetInput(0);
    if (input == nullptr) return false;
    
    auto attrSet = input->GetAttributeSet();
    if (attrSet == nullptr) return false;

    IGsize num = 0;
    if (attachmentType == IG_POINT) 
    { 
        auto ps = DynamicCast<PointSet>(input);
        if (ps == nullptr) return false;
        num = ps->GetNumberOfPoints();
    } 
    else 
    { 
        auto sm = DynamicCast<SurfaceMesh>(input);
        if (sm != nullptr) 
        {
            num = sm->GetNumberOfFaces(); 
        } 
        else 
        {
            auto um = DynamicCast<UnstructuredMesh>(input);
            if (um == nullptr) return false;
            num = um->GetNumberOfCells(); 
        }
    }
    if (num == 0) return false;

    double lo = max < min ? static_cast<double>(max) : static_cast<double>(min);
    double hi = max < min ? static_cast<double>(min) : static_cast<double>(max);

    ArrayObject::Pointer data = CreateDataArray(num, lo, hi, seed);
    if (data == nullptr) return false; 
   
    if (name.empty())
    {
        if (attachmentType == IG_POINT) 
            data->SetName("RandomPointScalars");
        else
            data->SetName("RandomCellScalars");      
    } 
    else
        data->SetName(name);

    attrSet->AddScalar(attachmentType, data); 

    SetOutput(input);

    return true;
}

IGAME_NAMESPACE_END
