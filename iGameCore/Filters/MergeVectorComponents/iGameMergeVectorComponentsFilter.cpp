#include "iGameMergeVectorComponentsFilter.h"
#include "iGameDataObject.h"

#include <string>

IGAME_NAMESPACE_BEGIN

ArrayObject::Pointer MergeVectorComponentsFilter::CreateArrayOfType(IGenum type) {
    switch (type) {
        case IG_FloatArray:            return FloatArray::New();
        case IG_DoubleArray:           return DoubleArray::New();
        case IG_IntArray:              return IntArray::New();
        case IG_UnsignedIntArray:      return UnsignedIntArray::New();
        case IG_LongLongArray:         return LongLongArray::New();
        case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
        case IG_ShortArray:            return ShortArray::New();
        case IG_UnsignedShortArray:    return UnsignedShortArray::New();
        case IG_CharArray:             return CharArray::New();
        case IG_UnsignedCharArray:     return UnsignedCharArray::New();
        default:                       return DoubleArray::New();
    }
}

bool MergeVectorComponentsFilter::Execute() {
    m_Message.clear();

    auto input = GetInput(0);
    if (!input) {
        m_Message = "No input data; please load a model first.";
        return false;
    }
    auto attrSet = input->GetAttributeSet();
    if (!attrSet) {
        m_Message = "Input data has no AttributeSet.";
        return false;
    }

    // Resolve each component array and validate: exists / same attachment / scalar /
    // single-component / equal element count / record whether types are uniform
    std::vector<ArrayObject::Pointer> comps;
    comps.reserve(m_ComponentNames.size());
    IGsize elementCount = 0;
    IGenum commonType = IG_FloatArray;
    bool typeUniform = true;

    for (size_t c = 0; c < m_ComponentNames.size(); ++c) {
        const std::string& name = m_ComponentNames[c];
        if (name.empty()) {
            m_Message = "Scalar array name #" + std::to_string(c + 1) + " is empty.";
            return false;
        }
        // Look up by name AND attachment
        int attrIdx = -1;
        for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
            auto& a = attrSet->GetAttribute(i);
            if (a.IsNone()) continue;
            if (a.attachmentType == m_AttachmentType && a.pointer->GetName() == name) {
                attrIdx = i;
                break;
            }
        }
        if (attrIdx < 0) {
            m_Message = "Scalar array not found in " +
                        std::string(m_AttachmentType == IG_POINT ? "PointData" : "CellData") +
                        ": \"" + name + "\".";
            return false;
        }
        auto& attr = attrSet->GetAttribute(attrIdx);
        if (attr.type != IG_SCALAR) {
            m_Message = "Array \"" + name + "\" is not a scalar (IG_SCALAR).";
            return false;
        }
        if (!attr.pointer) {
            m_Message = "Array \"" + name + "\" has a null pointer.";
            return false;
        }
        if (attr.pointer->GetDimension() != 1) {
            m_Message = "Array \"" + name + "\" is not a single-component scalar (dimension="
                        + std::to_string(attr.pointer->GetDimension()) + ").";
            return false;
        }
        IGsize n = attr.pointer->GetNumberOfElements();
        if (c == 0) {
            elementCount = n;
            commonType = attr.pointer->GetArrayType();
        } else {
            if (n != elementCount) {
                m_Message = "Array \"" + name + "\" element count (" +
                            std::to_string(static_cast<unsigned long long>(n)) +
                            ") does not match the first (" +
                            std::to_string(static_cast<unsigned long long>(elementCount)) +
                            ").";
                return false;
            }
            if (attr.pointer->GetArrayType() != commonType) {
                typeUniform = false;
            }
        }
        comps.push_back(attr.pointer);
    }

    // Output vector name: default "vector"; delete any existing same-named attribute under
    // the same attachment so re-running replaces it (point/cell vectors coexist independently)
    std::string outName = m_OutputName.empty() ? std::string("vector") : m_OutputName;
    for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
        auto& a = attrSet->GetAttribute(i);
        if (!a.IsNone() && a.pointer
            && a.attachmentType == m_AttachmentType
            && a.pointer->GetName() == outName) {
            attrSet->DeleteAttribute(i);
        }
    }

    // Output type: keep original type if all components share the same type, else promote to DoubleArray
    constexpr int N = 3;
    ArrayObject::Pointer out = typeUniform ? CreateArrayOfType(commonType)
                                           : DoubleArray::New();
    out->SetName(outName);
    out->SetDimension(N);
    out->Resize(elementCount);

    // Interleaved write: element i -> [comps[0][i], comps[1][i], comps[2][i]]
    for (IGsize i = 0; i < elementCount; ++i) {
        out->SetValue(i * 3 + 0, comps[0]->GetValue(i));
        out->SetValue(i * 3 + 1, comps[1]->GetValue(i));
        out->SetValue(i * 3 + 2, comps[2]->GetValue(i));
    }

    IGsize idx = attrSet->AddAttribute(IG_VECTOR, m_AttachmentType, out);
    if (idx == static_cast<IGsize>(-1)) {
        m_Message = "Failed to add the merged vector attribute.";
        return false;
    }
    auto& mergedAttr = attrSet->GetAttribute(idx);
    if (mergedAttr.IsNone() || !mergedAttr.pointer) {
        m_Message = "Merged vector attribute is invalid after adding.";
        return false;
    }
    mergedAttr.UpdateAllDataRange();
    this->SetOutput(input);
    return true;
}

IGAME_NAMESPACE_END
