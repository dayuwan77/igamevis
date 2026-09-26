#include "iGameRandomAttributesFilter.h"
#include <ctime>

IGAME_NAMESPACE_BEGIN

RandomAttributesFilter::RandomAttributesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}
RandomAttributesFilter::~RandomAttributesFilter() = default;

ArrayObject::Pointer RandomAttributesFilter::CreateDataArray(IGsize num, double lo, double hi, unsigned int seed) {
    switch (_dataType) {
    case IG_CHAR:                    return CreateTypedArray<CharArray, char>(num, lo, hi, seed);
    case IG_UNSIGNED_CHAR:           return CreateTypedArray<UnsignedCharArray, unsigned char>(num, lo, hi, seed);
    case IG_SHORT:                   return CreateTypedArray<ShortArray, short>(num, lo, hi, seed);
    case IG_UNSIGNED_SHORT:          return CreateTypedArray<UnsignedShortArray, unsigned short>(num, lo, hi, seed);
    case IG_INT:
    case IG_INDEX:                   return CreateTypedArray<IntArray, int>(num, lo, hi, seed);
    case IG_UNSIGNED_INT:            return CreateTypedArray<UnsignedIntArray, unsigned int>(num, lo, hi, seed);
    case IG_LONG_LONG:               return CreateTypedArray<LongLongArray, long long>(num, lo, hi, seed);
    case IG_UNSIGNED_LONG_LONG:      return CreateTypedArray<UnsignedLongLongArray, unsigned long long>(num, lo, hi, seed);
    case IG_FLOAT:                   return CreateTypedArray<FloatArray, float>(num, lo, hi, seed);
    case IG_DOUBLE:                  return CreateTypedArray<DoubleArray, double>(num, lo, hi, seed);
    default:
        _msg = "unsupported data type for RandomAttributesFilter.";
        return nullptr;
    }
}

// 针对不同 DataObject 精确类型计算 IG_POINT / IG_CELL 的元素个数
// 修复审核问题①：VolumeMesh 先于 SurfaceMesh 判断，避免继承链误命中 -> 面数当体单元数
bool RandomAttributesFilter::ComputeCount(DataObject::Pointer input, IGsize& out, std::string& why) const {
    if (_attach == IG_POINT) {
        auto ps = DynamicCast<PointSet>(input);
        if (!ps) { why = "input does not derive from PointSet; IG_POINT attachment unavailable."; return false; }
        out = ps->GetNumberOfPoints();
        return true;
    }
    // IG_CELL 分支：先精确匹配最具体的类型
    if (auto stm = DynamicCast<StructuredMesh>(input)) {
        // 结构化体网格：cell 指"体单元"
        out = stm->GetNumberOfVolumes();
        return true;
    }
    if (auto vm = DynamicCast<VolumeMesh>(input)) {
        // 体网格：cell 指"体单元"（这是审核指出的原 bug 修复点）
        out = vm->GetNumberOfVolumes();
        return true;
    }
    if (auto sm = DynamicCast<SurfaceMesh>(input)) {
        // 纯表面网格：cell 指"面"
        out = sm->GetNumberOfFaces();
        return true;
    }
    if (auto um = DynamicCast<UnstructuredMesh>(input)) {
        // 非结构网格：直接使用 VTK cell 数（2D/3D 语义随数据决定）
        out = um->GetNumberOfCells();
        return true;
    }
    why = "input type does not support IG_CELL attachment.";
    return false;
}

bool RandomAttributesFilter::Execute() {
    auto input = GetInput(0);
    if (!input) { _msg = "null input."; return false; }
    auto attrSet = input->GetAttributeSet();
    if (!attrSet) { _msg = "input has no AttributeSet."; return false; }

    IGsize num = 0;
    std::string why;
    if (!ComputeCount(input, num, why)) {
        _msg = why;
        return false;
    }
    if (num == 0) {
        _msg = "zero-sized attachment (no points/cells).";
        return false;
    }

    const double lo = (_max < _min) ? _max : _min;
    const double hi = (_max < _min) ? _min : _max;

    auto data = CreateDataArray(num, lo, hi, _seed);
    if (!data) return false;       // _msg 已在 CreateDataArray 里设置

    if (!_name.empty()) {
        data->SetName(_name);
    } else {
        // 业务约定：默认名 "RandomPointScalars" / "RandomCellScalars"
        // （CreateTypedArray 里已 SetName，这里仅作为兜底校验）
        const auto* expected = (_attach == IG_POINT) ? "RandomPointScalars" : "RandomCellScalars";
        if (data->GetName() != expected) data->SetName(expected);
    }

    attrSet->AddScalar(_attach, data);
    SetOutput(input);
    return true;
}

IGAME_NAMESPACE_END