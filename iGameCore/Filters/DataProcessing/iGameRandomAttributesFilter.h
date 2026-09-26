#ifndef iGameRandomAttributesFilter_h
#define iGameRandomAttributesFilter_h

#include "iGameFilter.h"
/* VolumeMesh 继承自 SurfaceMesh，必须单独 include 并在分支判断里优先匹配，
 * 否则 DynamicCast<SurfaceMesh> 会把 VolumeMesh 一并命中，
 * 导致 IG_CELL 时取到的是"面数"而不是"体单元数"。 */
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"

#include <random>
#include <string>
#include <type_traits>
#include <limits>

IGAME_NAMESPACE_BEGIN

/**
 * @brief 生成一组随机标量并挂接在输入数据对象的 POINT / CELL 上。
 *
 * 修复点（相对初版）：
 *   ① IG_CELL 对不同 DataObject 类型使用正确的计数来源：
 *      - SurfaceMesh        -> GetNumberOfFaces()（纯 2D 表面）
 *      - VolumeMesh         -> GetNumberOfVolumes()（体单元数，而非继承自 SurfaceMesh 的面数）
 *      - StructuredMesh     -> GetNumberOfVolumes()（结构化体单元）
 *      - UnstructuredMesh   -> GetNumberOfCells()（VTK cell 数，2D/3D 都可用）
 *      - 纯 PointSet        -> 报错（IG_CELL 不适用）
 *   ② 支持 10 种数据类型（char..double），并做数值范围裁剪到 TValue 的合法区间。
 *   ③ 输出属性命名 "RandomPointScalars" / "RandomCellScalars"（业务约定）。
 *   ④ 调用方负责在 Execute() 之前注入 min/max/seed/attachmentType。
 */
class RandomAttributesFilter : public Filter {
public:
    I_OBJECT(RandomAttributesFilter);
    static Pointer New() { return new RandomAttributesFilter; }
    ~RandomAttributesFilter();

    // 数值范围 [min, max]（浮点/整数统一用 double 容器，内部按类型裁剪）
    void SetRange(double lo, double hi) noexcept { _min = lo; _max = hi; }

    // IG_CHAR / IG_UNSIGNED_CHAR / IG_SHORT / IG_UNSIGNED_SHORT /
    // IG_INT  / IG_UNSIGNED_INT  / IG_LONG_LONG / IG_UNSIGNED_LONG_LONG /
    // IG_FLOAT / IG_DOUBLE
    void SetDataType(IGenum type) noexcept { _dataType = type; }

    // IG_POINT or IG_CELL
    void SetAttachmentType(IGenum type) noexcept { _attach = type; }

    // 伪随机种子；同一 seed + 同输入长度 -> 同一序列
    void SetSeed(unsigned int seed) noexcept { _seed = seed; }

    // 覆盖默认属性名（可选；为空时使用 "RandomPointScalars" / "RandomCellScalars"）
    void SetAttributeName(const std::string& name) { _name = name; }

    // 失败原因（供 GUI / CLI 回显）
    std::string GetMessage() const { return _msg; }

    bool Execute() override;

protected:
    RandomAttributesFilter();

    // 根据 <TArray, TValue> 创建 typed 数组并填入随机数
    template<typename TArray, typename TValue>
    ArrayObject::Pointer CreateTypedArray(IGsize num, double lo, double hi, unsigned int seed);

    // 根据 _dataType 分派到 CreateTypedArray
    ArrayObject::Pointer CreateDataArray(IGsize num, double lo, double hi, unsigned int seed);

    // 按照 attachmentType 与数据对象的精确类型计算"要生成几个随机数"
    bool ComputeCount(DataObject::Pointer input, IGsize& out, std::string& why) const;

    double            _min{ 0.0 };
    double            _max{ 1.0 };
    IGenum            _dataType{ IG_FLOAT };
    IGenum            _attach{ IG_POINT };
    unsigned int      _seed{ static_cast<unsigned int>(std::time(nullptr)) };
    std::string       _name;
    std::string       _msg{ "RandomAttributesFilter failed." };
};

template<typename TArray, typename TValue>
ArrayObject::Pointer RandomAttributesFilter::CreateTypedArray(
    IGsize num, double lo, double hi, unsigned int seed)
{
    auto arr = TArray::New();
    arr->SetName(_attach == IG_POINT ? "RandomPointScalars" : "RandomCellScalars");
    arr->SetDimension(1);
    if (num == 0) return arr;
    arr->Reserve(num);

    // 1) 把 lo/hi 裁剪到 TValue 的可表示数值区间
    using lim = std::numeric_limits<TValue>;
    if constexpr (std::is_floating_point_v<TValue>) {
        double lo2 = lo;
        double hi2 = hi;
        if (lo2 < static_cast<double>(lim::lowest()))  lo2 = static_cast<double>(lim::lowest());
        if (hi2 > static_cast<double>(lim::max()))     hi2 = static_cast<double>(lim::max());
        if (hi2 < lo2) hi2 = lo2;

        std::mt19937 gen(seed);
        std::uniform_real_distribution<double> dist(lo2, hi2);
        for (IGsize i = 0; i < num; ++i)
            arr->AddValue(static_cast<TValue>(dist(gen)));
    } else {
        // 整型范围
        long long loLL;
        long long hiLL;
        if constexpr (std::is_signed_v<TValue>) {
            constexpr auto MIN = static_cast<long long>(lim::min());
            constexpr auto MAX = static_cast<long long>(lim::max());
            loLL = static_cast<long long>(lo);
            hiLL = static_cast<long long>(hi);
            if (loLL < MIN) loLL = MIN;
            if (hiLL > MAX) hiLL = MAX;
        } else {
            using ULL = unsigned long long;
            constexpr auto MAX = static_cast<ULL>(lim::max());
            // 把 [lo, hi] 裁剪到 [0, MAX] 再转 long long（不会越界）
            ULL loU = static_cast<ULL>(lo);
            ULL hiU = static_cast<ULL>(hi);
            if (loU > MAX) loU = MAX;
            if (hiU > MAX) hiU = MAX;
            if (hiU < loU) hiU = loU;
            loLL = static_cast<long long>(loU);
            hiLL = static_cast<long long>(hiU);
        }
        if (hiLL < loLL) hiLL = loLL;

        std::mt19937 gen(seed);
        std::uniform_int_distribution<long long> dist(loLL, hiLL);
        for (IGsize i = 0; i < num; ++i)
            arr->AddValue(static_cast<TValue>(dist(gen)));
    }
    return arr;
}

IGAME_NAMESPACE_END
#endif