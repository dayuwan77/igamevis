#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"

#include <cmath>

IGAME_NAMESPACE_BEGIN

namespace {
// 数值容差
constexpr double kEps = 1e-9;
// 参考单元边界松弛量（仅提前终止无效 Newton 迭代）
constexpr double kRefTol = 0.1;
constexpr int kMaxNewtonIter = 40;

// 有符号四面体体积：1/6 * dot(b-a, cross(c-a, d-a))
double SignedTetVolume(const Vector3d& a, const Vector3d& b, const Vector3d& c, const Vector3d& d) {
    return (b - a) * ((c - a).cross(d - a)) / 6.0;
}

// 求解 3x3 线性方程组 J * dx = b（克拉默法则）
bool Solve3x3(const Vector3d& c0, const Vector3d& c1, const Vector3d& c2, const Vector3d& b, Vector3d& dx) {
    double det = c0 * (c1.cross(c2));
    if (std::abs(det) < kEps) return false;
    dx[0] = b * (c1.cross(c2)) / det;
    dx[1] = c0 * (b.cross(c2)) / det;
    dx[2] = c0 * (c1.cross(b)) / det;
    return true;
}
} // namespace

PointVolumeInterpolatorFilter::PointVolumeInterpolatorFilter() {
    SetNumberOfInputs(2);   // input[0]: 体网格, input[1]: 查询点集
    SetNumberOfOutputs(1);
}

bool PointVolumeInterpolatorFilter::Execute() {
    // ==================== 输入获取 ====================
    auto meshInput = GetInput(0);
    auto queryInput = GetInput(1);
    if (meshInput == nullptr || queryInput == nullptr) {
        igError("PointVolumeInterpolatorFilter: input[0]/input[1] is null.");
        return false;
    }

    // 体网格：VolumeMesh 的单元 API 与 UnstructuredMesh 类似，直接遍历即可
    VolumeMesh::Pointer volumeMesh = DynamicCast<VolumeMesh>(meshInput);
    if (volumeMesh == nullptr) {
        auto unstructuredMesh = DynamicCast<UnstructuredMesh>(meshInput);
        if (unstructuredMesh == nullptr) {
            igError("PointVolumeInterpolatorFilter: input[0] must be a VolumeMesh or UnstructuredMesh.");
            return false;
        }
        volumeMesh = unstructuredMesh->TransferToVolumeMesh();
        if (volumeMesh == nullptr) {
            igError("PointVolumeInterpolatorFilter: cannot convert input[0] to VolumeMesh (contains non-3D cells?).");
            return false;
        }
    }

    // 查询点集
    auto querySet = DynamicCast<PointSet>(queryInput);
    if (querySet == nullptr) {
        igError("PointVolumeInterpolatorFilter: input[1] must be a PointSet.");
        return false;
    }
    Points::Pointer queryPts = querySet->GetPoints();

    // ==================== 属性选取 ====================
    auto attrs = volumeMesh->GetAttributeSet();
    int index = m_AttributeIndex;
    if (index == -1 && !m_AttributeName.empty()) {
        index = attrs->GetAttributeIndex(m_AttributeName);
    }
    if (index < 0 || static_cast<IGsize>(index) >= attrs->GetNumberOfAttributes()) {
        igError("PointVolumeInterpolatorFilter: attribute index {} is out of range.", index);
        return false;
    }
    auto& attr = attrs->GetAttribute(index);
    ArrayObject* data = attr.pointer.get();
    if (data == nullptr) {
        igError("PointVolumeInterpolatorFilter: attribute {} is empty.", index);
        return false;
    }
    int dim = data->GetDimension();

    // ==================== 输出准备 ====================
    auto output = PointSet::New();
    output->SetPoints(queryPts);

    FloatArray::Pointer result = FloatArray::New();
    result->SetDimension(dim);
    result->Resize(queryPts->GetNumberOfPoints());

    // ==================== 空间加速：体单元包围盒 ====================
    // 对每个体单元求轴对齐包围盒，查询点只测试包围盒包含它的单元。
    const IGsize numVolumes = volumeMesh->GetNumberOfVolumes();
    std::vector<Vector3d> boxMin(numVolumes), boxMax(numVolumes);
    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    for (IGsize v = 0; v < numVolumes; ++v) {
        int n = volumeMesh->GetVolumePointIds(v, ptIds);
        Vector3d lo(1e30, 1e30, 1e30), hi(-1e30, -1e30, -1e30);
        for (int j = 0; j < n; ++j) {
            const Point& p = volumeMesh->GetPoint(ptIds[j]);
            for (int d = 0; d < 3; ++d) {
                lo[d] = std::min(lo[d], double(p[d]));
                hi[d] = std::max(hi[d], double(p[d]));
            }
        }
        boxMin[v] = lo;
        boxMax[v] = hi;
    }

    // ==================== 插值主循环 ====================
    std::vector<float> val(dim, 0.f);
    std::vector<double> weights;
    // 命中掩码：1=查询点落在体网格内，0=未命中
    UnsignedCharArray::Pointer hitMask = UnsignedCharArray::New();
    hitMask->SetDimension(1);
    hitMask->Resize(queryPts->GetNumberOfPoints());

    const IGsize numQuery = queryPts->GetNumberOfPoints();
    for (IGsize i = 0; i < numQuery; ++i) {
        Point q = queryPts->GetPoint(i);
        std::fill(val.begin(), val.end(), 0.f);

        bool hit = false;
        for (IGsize v = 0; v < numVolumes; ++v) {
            // 包围盒快速剔除
            if (q[0] < boxMin[v][0] || q[0] > boxMax[v][0] ||
                q[1] < boxMin[v][1] || q[1] > boxMax[v][1] ||
                q[2] < boxMin[v][2] || q[2] > boxMax[v][2]) {
                continue;
            }
            auto cell = volumeMesh->GetVolume(v);
            if (ComputeBarycentric(q, cell, weights)) {
                InterpolateAttribute(weights, data, cell, val.data());
                hit = true;
                break;
            }
        }
        // 若查询点在所有单元之外，val 保持 0 且命中掩码为 0
        hitMask->SetValue(i, hit ? 1 : 0);
        result->SetElement(i, val);
    }

    // ==================== 挂属性并输出 ====================
    if (dim == 1) {
        output->GetAttributeSet()->AddScalar(IG_POINT, result);
    } else {
        output->GetAttributeSet()->AddVector(IG_POINT, result);
    }
    hitMask->SetName(HitMaskName);
    output->GetAttributeSet()->AddScalar(IG_POINT, hitMask);
    SetOutput(output);
    return true;
}

bool PointVolumeInterpolatorFilter::ComputeBarycentric(const Point& p, Cell* cell, std::vector<double>& weights) {
    const int n = cell->GetNumberOfPoints();
    weights.assign(n, 0.0);

    switch (cell->GetCellType()) {
        case IG_TETRA: {
            // 体积坐标：w_i = V(q, 其余三点) / V(四面体)
            Vector3d p0 = cell->GetPoint(0);
            Vector3d p1 = cell->GetPoint(1);
            Vector3d p2 = cell->GetPoint(2);
            Vector3d p3 = cell->GetPoint(3);
            Vector3d q = p;
            double detT = SignedTetVolume(p0, p1, p2, p3);
            if (std::abs(detT) < kEps) return false;
            weights[0] = SignedTetVolume(q, p1, p2, p3) / detT;
            weights[1] = SignedTetVolume(p0, q, p2, p3) / detT;
            weights[2] = SignedTetVolume(p0, p1, q, p3) / detT;
            weights[3] = SignedTetVolume(p0, p1, p2, q) / detT;
            break;
        }
        case IG_HEXAHEDRON: {
            // 参考单元三线性坐标 (r,s,t) in [0,1]，Newton 迭代求解
            Vector3d pts[8];
            for (int i = 0; i < 8; ++i) pts[i] = cell->GetPoint(i);
            Vector3d q = p;

            Vector3d x(0.5, 0.5, 0.5);
            double shape[8];
            bool converged = false;
            for (int iter = 0; iter < kMaxNewtonIter; ++iter) {
                double r = x[0], s = x[1], t = x[2];
                // 三线性形函数（VTK 顶点顺序）
                shape[0] = (1 - r) * (1 - s) * (1 - t);
                shape[1] = r * (1 - s) * (1 - t);
                shape[2] = r * s * (1 - t);
                shape[3] = (1 - r) * s * (1 - t);
                shape[4] = (1 - r) * (1 - s) * t;
                shape[5] = r * (1 - s) * t;
                shape[6] = r * s * t;
                shape[7] = (1 - r) * s * t;

                // 当前位置及其与目标点的差
                Vector3d pos(0, 0, 0), F(0, 0, 0);
                for (int i = 0; i < 8; ++i) pos += pts[i] * shape[i];
                F = pos - q;
                if (F.length() < kEps) { converged = true; break; }

                // Jacobian 列向量：d pos / d(r,s,t)
                double dNdr[8] = {-(1 - s) * (1 - t), (1 - s) * (1 - t), s * (1 - t), -s * (1 - t),
                                  -(1 - s) * t,        (1 - s) * t,        s * t,        -s * t};
                double dNds[8] = {-(1 - r) * (1 - t), -r * (1 - t), r * (1 - t), (1 - r) * (1 - t),
                                  -(1 - r) * t,        -r * t,        r * t,        (1 - r) * t};
                double dNdt[8] = {-(1 - r) * (1 - s), -r * (1 - s), -r * s, -(1 - r) * s,
                                  (1 - r) * (1 - s),  r * (1 - s),  r * s,  (1 - r) * s};
                Vector3d Jr(0, 0, 0), Js(0, 0, 0), Jt(0, 0, 0);
                for (int i = 0; i < 8; ++i) {
                    Jr += pts[i] * dNdr[i];
                    Js += pts[i] * dNds[i];
                    Jt += pts[i] * dNdt[i];
                }

                // 解 J * dx = -F
                Vector3d dx;
                if (!Solve3x3(Jr, Js, Jt, -F, dx)) return false;
                x += dx;

                // 若偏离参考单元过远，直接判为单元外，提前终止
                if (x[0] < -kRefTol || x[0] > 1.0 + kRefTol ||
                    x[1] < -kRefTol || x[1] > 1.0 + kRefTol ||
                    x[2] < -kRefTol || x[2] > 1.0 + kRefTol) {
                    return false;
                }
            }
            // 迭代未收敛时参考坐标不可靠，不能用于插值
            if (!converged) return false;
            for (int i = 0; i < 8; ++i) weights[i] = shape[i];
            break;
        }
        default:
            return false;
    }

    // 点必须在单元内：所有权重非负
    for (double w : weights) {
        if (w < -kEps) return false;
    }
    return true;
}

void PointVolumeInterpolatorFilter::InterpolateAttribute(const std::vector<double>& weights, ArrayObject* data, Cell* cell, float* result) {
    const int dim = data->GetDimension();
    for (int d = 0; d < dim; ++d) result[d] = 0.f;
    for (size_t i = 0; i < weights.size(); ++i) {
        igIndex pid = cell->GetPointId(int(i));
        for (int d = 0; d < dim; ++d) {
            result[d] += float(weights[i] * data->GetElementValue(pid, d));
        }
    }
}

IGAME_NAMESPACE_END
