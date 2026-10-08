/**
 * @class   iGameYieldCriteria
 * @brief   在输入数据集的张量属性上计算屈服准则。
 */

#include "iGameYieldCriteriaFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <cmath>
#include <string>

namespace {
// 深拷贝输入网格，结果数组只加到新网格上，原模型保持不变
iGame::DataObject::Pointer CloneMesh(iGame::DataObject::Pointer input) {
    using namespace iGame;
    auto copyPoints = [](PointSet* src) -> Points::Pointer {
        auto pts = Points::New();
        pts->DeepCopy(src->GetPoints());
        return pts;
    };
    auto copyAttrs = [](DataObject* src) -> AttributeSet::Pointer {
        auto attrs = AttributeSet::New();
        attrs->DeepCopy(src->GetAttributeSet());
        return attrs;
    };

    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto src = DynamicCast<SurfaceMesh>(input);
            auto dst = SurfaceMesh::New();
            dst->SetPoints(copyPoints(src));
            if (src->GetFaces()) {
                auto faces = CellArray::New();
                faces->DeepCopy(src->GetFaces());
                dst->SetFaces(faces);
            }
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto src = DynamicCast<UnstructuredMesh>(input);
            auto dst = UnstructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            auto cells = CellArray::New();
            cells->DeepCopy(src->GetCells());
            auto types = UnsignedIntArray::New();
            types->DeepCopy(src->GetCellTypes());
            dst->SetCells(cells, types);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_VOLUME_MESH: {
            auto src = DynamicCast<VolumeMesh>(input);
            auto dst = VolumeMesh::New();
            dst->SetPoints(copyPoints(src));
            auto vols = CellArray::New();
            vols->DeepCopy(src->GetVolumes());
            dst->SetVolumes(vols);
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_STRUCTURED_MESH: {
            auto src = DynamicCast<StructuredMesh>(input);
            auto dst = StructuredMesh::New();
            dst->SetPoints(copyPoints(src));
            dst->SetDimensionSize(src->GetDimensionSize());
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        case IG_POINT_SET: {
            auto src = DynamicCast<PointSet>(input);
            auto dst = PointSet::New();
            dst->SetPoints(copyPoints(src));
            dst->SetAttributeSet(copyAttrs(src));
            dst->SetName(src->GetName());
            return dst;
        }
        default:
            return nullptr;
    }
}

// 把 6 分量（xx,yy,zz,xy,yz,zx）或 9 分量张量读成 3x3 对称矩阵
void ReadTensor3x3(iGame::ArrayObject* array, int dimension, IGsize tupleId, double a[3][3]) {
    if (dimension == 6) {
        const IGsize base = tupleId * 6;
        const double xx = array->GetValue(base + 0);
        const double yy = array->GetValue(base + 1);
        const double zz = array->GetValue(base + 2);
        const double xy = array->GetValue(base + 3);
        const double yz = array->GetValue(base + 4);
        const double zx = array->GetValue(base + 5);
        a[0][0] = xx;
        a[1][1] = yy;
        a[2][2] = zz;
        a[0][1] = a[1][0] = xy;
        a[1][2] = a[2][1] = yz;
        a[0][2] = a[2][0] = zx;
    } else {
        const IGsize base = tupleId * 9;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                a[i][j] = array->GetValue(base + i * 3 + j);
            }
        }
    }
}
} // namespace

IGAME_NAMESPACE_BEGIN

void ComputeSymmetricEigen(double a[3][3], double values[3], double vectors[3][3]) {
    // Jacobi 旋转法：迭代把非对角元素消到 0
    double v[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

    for (int sweep = 0; sweep < 64; ++sweep) {
        const double off = std::fabs(a[0][1]) + std::fabs(a[0][2]) + std::fabs(a[1][2]);
        if (off < 1e-15) break;

        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                if (std::fabs(a[p][q]) < 1e-18) continue;

                const double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                const double sign = (theta >= 0.0) ? 1.0 : -1.0;
                const double t = sign / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;

                for (int k = 0; k < 3; ++k) {
                    if (k == p || k == q) continue;
                    const double akp = a[k][p];
                    const double akq = a[k][q];
                    a[k][p] = a[p][k] = c * akp - s * akq;
                    a[k][q] = a[q][k] = s * akp + c * akq;
                }
                const double app = a[p][p];
                const double aqq = a[q][q];
                const double apq = a[p][q];
                a[p][p] = c * c * app - 2.0 * s * c * apq + s * s * aqq;
                a[q][q] = s * s * app + 2.0 * s * c * apq + c * c * aqq;
                a[p][q] = a[q][p] = 0.0;

                for (int k = 0; k < 3; ++k) {
                    const double vkp = v[k][p];
                    const double vkq = v[k][q];
                    v[k][p] = c * vkp - s * vkq;
                    v[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }

    for (int i = 0; i < 3; ++i) { values[i] = a[i][i]; }

    // 特征值降序，并同步交换对应特征向量（v 的列 -> 输出行）
    int order[3] = {0, 1, 2};
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (values[order[j]] > values[order[i]]) {
                const int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
    double sortedValues[3] = {values[order[0]], values[order[1]], values[order[2]]};
    double sortedVectors[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { sortedVectors[i][k] = v[k][order[i]]; }
    }
    for (int i = 0; i < 3; ++i) {
        values[i] = sortedValues[i];
        for (int k = 0; k < 3; ++k) { vectors[i][k] = sortedVectors[i][k]; }
    }
}

bool YieldCriteriaFilter::Execute() {
    m_StatusMessage.clear();

    auto input = GetInput(0);
    if (input == nullptr) {
        m_StatusMessage = "输入为空。";
        return false;
    }
    auto srcPointSet = DynamicCast<PointSet>(input);
    if (srcPointSet == nullptr) {
        m_StatusMessage = "当前对象不支持屈服准则计算（需要网格 / 点集）。";
        return false;
    }
    auto srcAttrSet = input->GetAttributeSet();
    if (srcAttrSet == nullptr) {
        m_StatusMessage = "输入没有属性数据。";
        return false;
    }

    // 1) 找到参与计算的张量属性：优先用户指定，其次优先标记为张量的属性，
    //    最后接受任意 6 / 9 分量属性（部分读取器不把 6 分量数组标记为张量类型）
    int tensorIndex = -1;
    if (!m_TensorArrayName.empty()) {
        tensorIndex = srcAttrSet->GetAttributeIndex(m_TensorArrayName);
        if (tensorIndex < 0) {
            m_StatusMessage = "找不到张量属性：" + m_TensorArrayName;
            return false;
        }
    } else {
        int fallbackIndex = -1;
        const size_t numAttrs = srcAttrSet->GetNumberOfAttributes();
        for (size_t i = 0; i < numAttrs; ++i) {
            auto& attr = srcAttrSet->GetAttribute(static_cast<IGsize>(i));
            if (attr.isDeleted || attr.pointer == nullptr) continue;
            const int dim = attr.pointer->GetDimension();
            if (dim != 6 && dim != 9) continue;
            if (attr.type == IG_TENSOR) {
                tensorIndex = static_cast<int>(i);
                break;
            }
            if (fallbackIndex < 0) { fallbackIndex = static_cast<int>(i); }
        }
        if (tensorIndex < 0) { tensorIndex = fallbackIndex; }
        if (tensorIndex < 0) {
            m_StatusMessage = "输入里没有 6 或 9 分量的张量属性。";
            return false;
        }
    }

    auto& tensorAttr = srcAttrSet->GetAttribute(static_cast<IGsize>(tensorIndex));
    auto tensorArray = tensorAttr.pointer;
    const int dimension = tensorArray->GetDimension();
    if (dimension != 6 && dimension != 9) {
        m_StatusMessage = "张量属性必须是 6 分量（对称）或 9 分量。";
        return false;
    }
    const IGenum attachment = tensorAttr.attachmentType;
    const IGsize numTuples = tensorArray->GetNumberOfElements();
    const std::string tensorName = tensorArray->GetName();

    // 2) 深拷贝输入（几何/拓扑不变，结果只作为新数组追加）
    auto newMesh = CloneMesh(input);
    if (newMesh == nullptr) {
        m_StatusMessage = "当前数据类型不支持屈服准则计算。";
        return false;
    }
    auto dstAttrSet = newMesh->GetAttributeSet();

    auto makeScalarArray = [](const std::string& name, IGsize count) {
        auto array = DoubleArray::New();
        array->SetName(name);
        array->SetDimension(1);
        array->Reserve(count);
        return array;
    };
    auto makeVectorArray = [](const std::string& name, IGsize count) {
        auto array = DoubleArray::New();
        array->SetName(name);
        array->SetDimension(3);
        array->Reserve(count);
        return array;
    };

    // 3) 逐个点（单元）计算
    if (m_Criterion == PRINCIPAL_STRESS) {
        auto sigma1 = makeScalarArray(tensorName + " - Sigma 1", numTuples);
        auto sigma2 = makeScalarArray(tensorName + " - Sigma 2", numTuples);
        auto sigma3 = makeScalarArray(tensorName + " - Sigma 3", numTuples);
        auto dir1 = makeVectorArray(tensorName + " - Sigma 1 (Vector)", numTuples);
        auto dir2 = makeVectorArray(tensorName + " - Sigma 2 (Vector)", numTuples);
        auto dir3 = makeVectorArray(tensorName + " - Sigma 3 (Vector)", numTuples);

        for (IGsize t = 0; t < numTuples; ++t) {
            double a[3][3];
            ReadTensor3x3(tensorArray.GetPointer(), dimension, t, a);
            double values[3];
            double vectors[3][3];
            ComputeSymmetricEigen(a, values, vectors);
            sigma1->AddValue(values[0]);
            sigma2->AddValue(values[1]);
            sigma3->AddValue(values[2]);
            dir1->AddElement3(vectors[0][0], vectors[0][1], vectors[0][2]);
            dir2->AddElement3(vectors[1][0], vectors[1][1], vectors[1][2]);
            dir3->AddElement3(vectors[2][0], vectors[2][1], vectors[2][2]);
        }

        dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma1);
        dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma2);
        dstAttrSet->AddAttribute(IG_SCALAR, attachment, sigma3);
        dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir1);
        dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir2);
        dstAttrSet->AddAttribute(IG_VECTOR, attachment, dir3);
    } else {
        const bool isTresca = (m_Criterion == TRESCA);
        const std::string resultName =
                tensorName + (isTresca ? " - Tresca Criterion" : " - Von Mises Criterion");
        auto result = makeScalarArray(resultName, numTuples);

        for (IGsize t = 0; t < numTuples; ++t) {
            double a[3][3];
            ReadTensor3x3(tensorArray.GetPointer(), dimension, t, a);
            double values[3];
            double vectors[3][3];
            ComputeSymmetricEigen(a, values, vectors);
            const double sigma1 = values[0];
            const double sigma2 = values[1];
            const double sigma3 = values[2];
            double value = 0.0;
            if (isTresca) {
                value = sigma1 - sigma3;
            } else {
                const double d12 = sigma1 - sigma2;
                const double d23 = sigma2 - sigma3;
                const double d31 = sigma3 - sigma1;
                value = std::sqrt(0.5 * (d12 * d12 + d23 * d23 + d31 * d31));
            }
            result->AddValue(value);
        }
        dstAttrSet->AddAttribute(IG_SCALAR, attachment, result);
    }

    dstAttrSet->ForceReConvertToDrawableData();
    newMesh->SetName(input->GetName() + "_YieldCriteria");
    SetOutput(newMesh);

    const char* criterionName = (m_Criterion == PRINCIPAL_STRESS)
                                        ? "主应力 (Principal Stress)"
                                        : (m_Criterion == TRESCA ? "Tresca 准则" : "von Mises 准则");
    m_StatusMessage = std::string("基于张量属性 \"") + tensorName + "\" 计算 " + criterionName + "。";
    return true;
}

void YieldCriteriaFilter::SetTensorArrayName(const std::string& name) { m_TensorArrayName = name; }

const std::string& YieldCriteriaFilter::GetTensorArrayName() const { return m_TensorArrayName; }

void YieldCriteriaFilter::SetCriterion(Criterion criterion) { m_Criterion = criterion; }

YieldCriteriaFilter::Criterion YieldCriteriaFilter::GetCriterion() const { return m_Criterion; }

const std::string& YieldCriteriaFilter::GetStatusMessage() const { return m_StatusMessage; }

YieldCriteriaFilter::YieldCriteriaFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

IGAME_NAMESPACE_END
