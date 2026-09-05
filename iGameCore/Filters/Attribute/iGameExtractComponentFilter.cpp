#include "iGameExtractComponentFilter.h"

#include "iGameAttributeSet.h"
#include "iGameFlatArray.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

ExtractComponentFilter::ExtractComponentFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool ExtractComponentFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "输入数据对象为空";
        return false;
    }

    auto attributeSet = input->GetAttributeSet();
    if (attributeSet == nullptr) {
        m_Message = "输入数据对象没有属性集";
        return false;
    }

    // Resolve the input array: by explicit name (with optional attachment restriction) or,
    // when no name is given, the first array matching the attachment restriction.
    // The attachment type disambiguates same-named arrays attached to both PointData and CellData.
    AttributeSet::Attribute attr;
    auto matches = [&](const AttributeSet::Attribute& candidate) {
        if (candidate.IsNone()) return false;
        if (!m_InputArrayName.empty() && candidate.pointer->GetName() != m_InputArrayName) return false;
        if (m_InputAttachmentType != IG_NONE && candidate.attachmentType != m_InputAttachmentType) return false;
        return true;
    };
    auto inputAttributes = attributeSet->GetAllAttributes();
    for (IGsize i = 0; i < inputAttributes->GetNumberOfElements(); ++i) {
        auto& candidate = inputAttributes->GetElement(i);
        if (matches(candidate)) {
            attr = candidate;
            break;
        }
    }
    if (attr.IsNone()) {
        m_Message = m_InputArrayName.empty() ? "找不到输入数组" : "找不到输入数组: " + m_InputArrayName;
        return false;
    }

    // 维度校验：不允许提取不存在的分量（如 1/2 维数组提取超出维度的分量）
    if (attr.pointer->GetDimension() <= m_Component) {
        m_Message = "分量索引超出数组维度";
        return false;
    }

    // Create the output scalar array with the same concrete type as the input
    // (e.g. Int/LongLong stay Int/LongLong) to avoid silent type conversion or precision loss.
    IGsize elementNum = attr.pointer->GetNumberOfElements();
    ArrayObject::Pointer output;
    if (DynamicCast<FloatArray>(attr.pointer) != nullptr) {
        output = FloatArray::New();
    } else if (DynamicCast<DoubleArray>(attr.pointer) != nullptr) {
        output = DoubleArray::New();
    } else if (DynamicCast<IntArray>(attr.pointer) != nullptr) {
        output = IntArray::New();
    } else if (DynamicCast<UnsignedIntArray>(attr.pointer) != nullptr) {
        output = UnsignedIntArray::New();
    } else if (DynamicCast<CharArray>(attr.pointer) != nullptr) {
        output = CharArray::New();
    } else if (DynamicCast<UnsignedCharArray>(attr.pointer) != nullptr) {
        output = UnsignedCharArray::New();
    } else if (DynamicCast<ShortArray>(attr.pointer) != nullptr) {
        output = ShortArray::New();
    } else if (DynamicCast<UnsignedShortArray>(attr.pointer) != nullptr) {
        output = UnsignedShortArray::New();
    } else if (DynamicCast<LongLongArray>(attr.pointer) != nullptr) {
        output = LongLongArray::New();
    } else if (DynamicCast<UnsignedLongLongArray>(attr.pointer) != nullptr) {
        output = UnsignedLongLongArray::New();
    } else {
        output = DoubleArray::New();  // fallback
    }
    output->SetDimension(1);
    output->SetName(m_OutputArrayName);
    output->Resize(elementNum);
    for (IGsize i = 0; i < elementNum; ++i) {
        output->SetValue(i, attr.pointer->GetElementValue(i, m_Component));
    }

    // 继承语义：输出新数据对象，几何与输入共享；
    // 结果属性集 = 输入属性集的拷贝（跳过与输出名同名的旧数组，即覆盖语义）+ 新增结果数组。
    // 注意：不能用 DeleteAttribute 标记删除（渲染路径按索引遍历会解引用空指针），
    // 拷贝时直接跳过同名旧数组，保证结果属性集不含 isDeleted 残留项
    auto resultAttrSet = AttributeSet::New();
    auto allAttributes = attributeSet->GetAllAttributes();
    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& src = allAttributes->GetElement(i);
        if (src.IsNone()) continue;
        if (src.pointer->GetName() == m_OutputArrayName) continue;
        ArrayObject::Pointer copied;
        if (DynamicCast<FloatArray>(src.pointer) != nullptr) {
            auto p = FloatArray::New();
            p->DeepCopy(DynamicCast<FloatArray>(src.pointer));
            p->SetName(src.pointer->GetName());
            copied = p;
        } else if (DynamicCast<DoubleArray>(src.pointer) != nullptr) {
            auto p = DoubleArray::New();
            p->DeepCopy(DynamicCast<DoubleArray>(src.pointer));
            p->SetName(src.pointer->GetName());
            copied = p;
        } else {
            copied = src.pointer;  // 其他类型共享指针（只读属性，安全）
        }
        resultAttrSet->AddAttribute(src.type, src.attachmentType, copied);
    }
    resultAttrSet->AddScalar(attr.attachmentType, output);

    if (auto unstructured = DynamicCast<UnstructuredMesh>(input); unstructured != nullptr) {
        auto result = UnstructuredMesh::New();
        result->SetName(unstructured->GetName() + "_ExtractComponent");
        result->SetPoints(unstructured->GetPoints());
        result->SetCells(unstructured->GetCells(), UnsignedIntArray::Pointer(unstructured->GetCellTypes()));
        result->SetAttributeSet(resultAttrSet);
        SetOutput(result);
        return true;
    }

    if (auto surface = DynamicCast<SurfaceMesh>(input); surface != nullptr) {
        auto result = SurfaceMesh::New();
        result->SetName(surface->GetName() + "_ExtractComponent");
        result->SetPoints(surface->GetPoints());
        result->SetFaces(surface->GetFaces());
        result->SetAttributeSet(resultAttrSet);
        SetOutput(result);
        return true;
    }

    m_Message = "暂不支持该数据类型（仅支持非结构化网格/表面网格）";
    return false;
}

IGAME_NAMESPACE_END
