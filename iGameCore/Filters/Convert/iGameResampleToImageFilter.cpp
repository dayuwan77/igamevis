#include "iGameResampleToImageFilter.h"

#include "iGameAttributeSet.h"
#include "iGameArrayObject.h"
#include "iGameCellArray.h"
#include "iGameCellType.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGamePoints.h"
#include "iGameStructuredMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

IGAME_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// 匿名命名空间：点定位与插值权重的几何计算。
// 等价于 VTK 中 vtkCell::EvaluatePosition（对常见线性单元），返回点是否在
// 单元内，并给出各节点的插值权重。
namespace {

constexpr double kInsideEps = 1.0e-7;    // 重心/局部坐标的「在单元内」容差
constexpr double kDegenerateEps = 1.0e-20; // 退化解的最小行列式

// 与 vtkDataSetAttributes 一致（保持 ghost 数值一一对应）
constexpr unsigned char kHiddenPoint = 2;  // vtkDataSetAttributes::HIDDENPOINT
constexpr unsigned char kHiddenCell = 32;  // vtkDataSetAttributes::HIDDENCELL

inline double Dot(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void Cross(const double a[3], const double b[3], double out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// det([a b c]) = a . (b x c)
inline double Det3(const double a[3], const double b[3], const double c[3]) {
    return a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) +
           a[2] * (b[0] * c[1] - b[1] * c[0]);
}

inline bool Solve3(const double A[3][3], const double b[3], double x[3]) {
    double M[3][4];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) M[i][j] = A[i][j];
        M[i][3] = b[i];
    }
    for (int col = 0; col < 3; ++col) {
        int piv = col;
        for (int r = col + 1; r < 3; ++r) {
            if (std::fabs(M[r][col]) > std::fabs(M[piv][col])) piv = r;
        }
        if (std::fabs(M[piv][col]) < kDegenerateEps) return false;
        if (piv != col) {
            for (int c = 0; c < 4; ++c) std::swap(M[piv][c], M[col][c]);
        }
        for (int r = col + 1; r < 3; ++r) {
            const double f = M[r][col] / M[col][col];
            for (int c = col; c < 4; ++c) M[r][c] -= f * M[col][c];
        }
    }
    for (int r = 2; r >= 0; --r) {
        double s = M[r][3];
        for (int c = r + 1; c < 3; ++c) s -= M[r][c] * x[c];
        x[r] = s / M[r][r];
    }
    return true;
}

inline bool Solve2(const double A[2][2], const double b[2], double x[2]) {
    const double det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (std::fabs(det) < kDegenerateEps) return false;
    x[0] = (b[0] * A[1][1] - b[1] * A[0][1]) / det;
    x[1] = (A[0][0] * b[1] - A[1][0] * b[0]) / det;
    return true;
}

// 顶点单元
bool EvalVertex(const double pts[][3], const double x[3], double w[8]) {
    double d = 0.0;
    for (int i = 0; i < 3; ++i) d += (x[i] - pts[0][i]) * (x[i] - pts[0][i]);
    w[0] = 1.0;
    return d <= 1.0e-12;
}

// 线段单元（线性插值）
bool EvalLine(const double pts[][3], const double x[3], double w[8]) {
    double dir[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double rel[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double denom = Dot(dir, dir);
    if (denom < kDegenerateEps) return EvalVertex(pts, x, w);
    const double t = Dot(rel, dir) / denom;
    w[0] = 1.0 - t;
    w[1] = t;
    return t >= -kInsideEps && t <= 1.0 + kInsideEps;
}

// 三角形单元（平面重心坐标）
bool EvalTriangle(const double pts[][3], const double x[3], double w[8]) {
    double v0[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double v1[3] = {pts[2][0] - pts[0][0], pts[2][1] - pts[0][1], pts[2][2] - pts[0][2]};
    double v2[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double d00 = Dot(v0, v0), d01 = Dot(v0, v1), d11 = Dot(v1, v1);
    const double d20 = Dot(v2, v0), d21 = Dot(v2, v1);
    const double denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < kDegenerateEps) return false;
    const double v = (d11 * d20 - d01 * d21) / denom;
    const double u = (d00 * d21 - d01 * d20) / denom;
    const double t = 1.0 - v - u;
    w[0] = t;
    w[1] = v;
    w[2] = u;
    return t >= -kInsideEps && v >= -kInsideEps && u >= -kInsideEps;
}

// 四面体单元（三维重心坐标，与节点顺序无关）
bool EvalTetra(const double pts[][3], const double x[3], double w[8]) {
    double e1[3] = {pts[1][0] - pts[0][0], pts[1][1] - pts[0][1], pts[1][2] - pts[0][2]};
    double e2[3] = {pts[2][0] - pts[0][0], pts[2][1] - pts[0][1], pts[2][2] - pts[0][2]};
    double e3[3] = {pts[3][0] - pts[0][0], pts[3][1] - pts[0][1], pts[3][2] - pts[0][2]};
    double rhs[3] = {x[0] - pts[0][0], x[1] - pts[0][1], x[2] - pts[0][2]};
    const double det = Det3(e1, e2, e3);
    if (std::fabs(det) < kDegenerateEps) return false;
    const double w1 = Det3(rhs, e2, e3) / det;
    const double w2 = Det3(e1, rhs, e3) / det;
    const double w3 = Det3(e1, e2, rhs) / det;
    const double w0 = 1.0 - w1 - w2 - w3;
    w[0] = w0;
    w[1] = w1;
    w[2] = w2;
    w[3] = w3;
    return w0 >= -kInsideEps && w1 >= -kInsideEps && w2 >= -kInsideEps && w3 >= -kInsideEps;
}

// 四边形单元（双线性，Newton 迭代求局部坐标 r,s）
bool EvalQuad(const double pts[][3], const double x[3], double w[8]) {
    const double R[4] = {-1.0, 1.0, 1.0, -1.0};
    const double S[4] = {-1.0, -1.0, 1.0, 1.0};
    double r = 0.0, s = 0.0;
    for (int iter = 0; iter < 100; ++iter) {
        double P[3] = {0.0, 0.0, 0.0};
        double dPdr[3] = {0.0, 0.0, 0.0};
        double dPds[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < 4; ++i) {
            const double N = 0.25 * (1.0 + r * R[i]) * (1.0 + s * S[i]);
            const double dNdr = 0.25 * R[i] * (1.0 + s * S[i]);
            const double dNds = 0.25 * S[i] * (1.0 + r * R[i]);
            for (int c = 0; c < 3; ++c) {
                P[c] += N * pts[i][c];
                dPdr[c] += dNdr * pts[i][c];
                dPds[c] += dNds * pts[i][c];
            }
        }
        double f[2] = {x[0] - P[0], x[1] - P[1]};
        double J[2][2] = {{dPdr[0], dPds[0]}, {dPdr[1], dPds[1]}};
        double delta[2];
        if (!Solve2(J, f, delta)) return false;
        r += delta[0];
        s += delta[1];
        if (std::sqrt(f[0] * f[0] + f[1] * f[1]) < 1.0e-12) break;
    }
    if (std::fabs(r) > 1.0 + 1.0e-6 || std::fabs(s) > 1.0 + 1.0e-6) return false;
    for (int i = 0; i < 4; ++i) w[i] = 0.25 * (1.0 + r * R[i]) * (1.0 + s * S[i]);
    return true;
}

// 六面体单元（三线性，Newton 迭代求局部坐标 r,s,t）
bool EvalHex(const double pts[][3], const double x[3], double w[8]) {
    const double R[8] = {-1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0};
    const double S[8] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
    const double T[8] = {-1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0};
    double r = 0.0, s = 0.0, t = 0.0;
    for (int iter = 0; iter < 100; ++iter) {
        double P[3] = {0.0, 0.0, 0.0};
        double dP[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
        for (int i = 0; i < 8; ++i) {
            const double N = 0.125 * (1.0 + r * R[i]) * (1.0 + s * S[i]) * (1.0 + t * T[i]);
            const double dNdr = 0.125 * R[i] * (1.0 + s * S[i]) * (1.0 + t * T[i]);
            const double dNds = 0.125 * S[i] * (1.0 + r * R[i]) * (1.0 + t * T[i]);
            const double dNdt = 0.125 * T[i] * (1.0 + r * R[i]) * (1.0 + s * S[i]);
            for (int c = 0; c < 3; ++c) {
                P[c] += N * pts[i][c];
                dP[0][c] += dNdr * pts[i][c];
                dP[1][c] += dNds * pts[i][c];
                dP[2][c] += dNdt * pts[i][c];
            }
        }
        double f[3] = {x[0] - P[0], x[1] - P[1], x[2] - P[2]};
        double delta[3];
        if (!Solve3(dP, f, delta)) return false;
        r += delta[0];
        s += delta[1];
        t += delta[2];
        if (std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]) < 1.0e-12) break;
    }
    if (std::fabs(r) > 1.0 + 1.0e-6 || std::fabs(s) > 1.0 + 1.0e-6 || std::fabs(t) > 1.0 + 1.0e-6) {
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        w[i] = 0.125 * (1.0 + r * R[i]) * (1.0 + s * S[i]) * (1.0 + t * T[i]);
    }
    return true;
}

// 单元定位与插值权重总入口。
// 返回值：true=点在单元内，weights[0..n-1] 为各节点权重。
// 注意：四边形/六面体假定节点顺序与 VTK 一致（0..3 / 0..7 规范顺序）。
bool EvaluateCell(IGenum cellType, const double pts[][3], int npts, const double x[3],
                  double weights[8]) {
    switch (static_cast<IGCellType>(cellType)) {
        case IG_VERTEX:
            return npts >= 1 && EvalVertex(pts, x, weights);
        case IG_LINE:
            return npts >= 2 && EvalLine(pts, x, weights);
        case IG_TRIANGLE:
            return npts >= 3 && EvalTriangle(pts, x, weights);
        case IG_QUAD:
            return npts >= 4 && EvalQuad(pts, x, weights);
        case IG_TETRA:
            return npts >= 4 && EvalTetra(pts, x, weights);
        case IG_HEXAHEDRON:
            return npts >= 8 && EvalHex(pts, x, weights);
        default:
            // IG_PRISM / IG_PYRAMID / IG_POLYGON / IG_POLYHEDRON / 二阶单元暂不支持，
            // 视作不在单元内。
            return false;
    }
}

} // namespace

//------------------------------------------------------------------------------
ResampleToImageFilter::ResampleToImageFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void ResampleToImageFilter::SetSamplingDimensions(int i, int j, int k) {
    SamplingDimensions[0] = i;
    SamplingDimensions[1] = j;
    SamplingDimensions[2] = k;
}

void ResampleToImageFilter::SetSamplingDimensions(int dims[3]) {
    SetSamplingDimensions(dims[0], dims[1], dims[2]);
}

void ResampleToImageFilter::GetSamplingDimensions(int dims[3]) const {
    dims[0] = SamplingDimensions[0];
    dims[1] = SamplingDimensions[1];
    dims[2] = SamplingDimensions[2];
}

void ResampleToImageFilter::SetSamplingBounds(const double bounds[6]) {
    for (int i = 0; i < 6; ++i) SamplingBounds[i] = bounds[i];
}

void ResampleToImageFilter::SetSamplingBounds(double x0, double x1, double y0, double y1, double z0,
                                              double z1) {
    SamplingBounds[0] = x0;
    SamplingBounds[1] = x1;
    SamplingBounds[2] = y0;
    SamplingBounds[3] = y1;
    SamplingBounds[4] = z0;
    SamplingBounds[5] = z1;
}

void ResampleToImageFilter::GetSamplingBounds(double bounds[6]) const {
    for (int i = 0; i < 6; ++i) bounds[i] = SamplingBounds[i];
}

//------------------------------------------------------------------------------
bool ResampleToImageFilter::Execute() {
    DataObject::Pointer input = GetInput(0);
    if (input == nullptr) return false;

    // 统一把输入当作 UnstructuredMesh 处理（等价 VTK 接受 vtkDataSet）。
    UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(input);
    if (mesh == nullptr) {
        mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    }
    if (mesh == nullptr) {
        igError("ResampleToImageFilter: input must be a mesh (PointSet subclass).");
        return false;
    }

    const IGsize numberOfPoints = mesh->GetNumberOfPoints();
    const IGsize numberOfCells = mesh->GetNumberOfCells();
    if (numberOfPoints == 0 || numberOfCells == 0) {
        igError("ResampleToImageFilter: empty input.");
        return false;
    }

    // 采样区域（等价 VTK vtkResampleToImage::RequestData）
    double samplingBounds[6];
    if (UseInputBounds) {
        double b[6] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                       std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),
                       std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        for (IGsize i = 0; i < numberOfPoints; ++i) {
            const Point& p = mesh->GetPoint(i);
            b[0] = std::min(b[0], static_cast<double>(p[0]));
            b[1] = std::max(b[1], static_cast<double>(p[0]));
            b[2] = std::min(b[2], static_cast<double>(p[1]));
            b[3] = std::max(b[3], static_cast<double>(p[1]));
            b[4] = std::min(b[4], static_cast<double>(p[2]));
            b[5] = std::max(b[5], static_cast<double>(p[2]));
        }
        // 向内收缩 epsilon，避免浮点舍入导致在数据集外采样。
        constexpr double epsilon = 1.0e-6;
        for (int i = 0; i < 3; ++i) {
            const double center = 0.5 * (b[2 * i] + b[2 * i + 1]);
            const double span = b[2 * i + 1] - b[2 * i];
            samplingBounds[2 * i] = center - 0.5 * span * (1.0 - epsilon);
            samplingBounds[2 * i + 1] = center + 0.5 * span * (1.0 - epsilon);
        }
    } else {
        for (int i = 0; i < 6; ++i) samplingBounds[i] = SamplingBounds[i];
    }

    int dims[3];
    GetSamplingDimensions(dims);
    if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0) {
        igError("ResampleToImageFilter: sampling dimensions must be positive.");
        return false;
    }

    const double origin[3] = {samplingBounds[0], samplingBounds[2], samplingBounds[4]};
    double spacing[3];
    for (int i = 0; i < 3; ++i) {
        spacing[i] = (dims[i] == 1) ? 0.0
                                    : (samplingBounds[2 * i + 1] - samplingBounds[2 * i]) /
                                          static_cast<double>(dims[i] - 1);
    }

    const IGsize numberOfGridPoints = static_cast<IGsize>(dims[0]) * dims[1] * dims[2];

    // 构建输出 StructuredMesh（等价 vtkImageData）
    StructuredMesh::Pointer output = StructuredMesh::New();
    output->SetName(input->GetName() + "_image");
    igIndex idims[3] = {static_cast<igIndex>(dims[0]), static_cast<igIndex>(dims[1]),
                        static_cast<igIndex>(dims[2])};
    output->SetDimensionSize(idims);

    Points::Pointer gridPoints = Points::New();
    gridPoints->Reserve(numberOfGridPoints);
    for (int k = 0; k < dims[2]; ++k) {
        for (int j = 0; j < dims[1]; ++j) {
            for (int i = 0; i < dims[0]; ++i) {
                gridPoints->AddPoint(static_cast<float>(origin[0] + i * spacing[0]),
                                     static_cast<float>(origin[1] + j * spacing[1]),
                                     static_cast<float>(origin[2] + k * spacing[2]));
            }
        }
    }
    output->SetPoints(gridPoints);
    output->GenStructuredCellConnectivities();

    // 收集待插值的输入点属性数组 + 待「快照」的输入单元属性数组。
    // 等价 vtkProbeFilter：源点数据插值到输出点数据；源单元数据在
    // (a) 与某点数组同名时被丢弃（点数据优先），否则作为输出点数组「快照」。
    struct SrcArray {
        ArrayObject::Pointer arr;
        IGenum type;
        int dim;
    };
    std::vector<SrcArray> srcPointArrays;
    std::vector<SrcArray> srcCellArrays;
    std::set<std::string> pointArrayNames;
    {
        auto all = mesh->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.attachmentType == IG_POINT) {
                if (a.pointer->GetNumberOfElements() != numberOfPoints) continue;
                srcPointArrays.push_back({a.pointer, a.type, a.pointer->GetDimension()});
                pointArrayNames.insert(a.pointer->GetName());
            } else if (a.attachmentType == IG_CELL) {
                if (a.pointer->GetNumberOfElements() != numberOfCells) continue;
                srcCellArrays.push_back({a.pointer, a.type, a.pointer->GetDimension()});
            }
        }
    }
    // 源单元数据 → 输出点数据（快照），跳过与点数组同名的。
    std::vector<size_t> snappedCellIndices;
    for (size_t c = 0; c < srcCellArrays.size(); ++c) {
        if (pointArrayNames.count(srcCellArrays[c].arr->GetName()) == 0) {
            snappedCellIndices.push_back(c);
        }
    }

    // 掩膜数组（等价 vtkValidPointMask）
    CharArray::Pointer mask = CharArray::New();
    mask->SetName(GetMaskArrayName());
    mask->SetDimension(1);
    mask->Resize(numberOfGridPoints);

    // 每一条输入点属性对应一条输出插值数组（默认值 0）。
    std::vector<FloatArray::Pointer> outPointArrays(srcPointArrays.size());
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        outPointArrays[s] = FloatArray::New();
        outPointArrays[s]->SetName(srcPointArrays[s].arr->GetName());
        outPointArrays[s]->SetDimension(srcPointArrays[s].dim);
        outPointArrays[s]->Resize(numberOfGridPoints);
    }
    // 每一条被快照的源单元属性 → 一条输出点数组（默认值 0）。
    std::vector<FloatArray::Pointer> outCellArrays(snappedCellIndices.size());
    for (size_t s = 0; s < snappedCellIndices.size(); ++s) {
        outCellArrays[s] = FloatArray::New();
        outCellArrays[s]->SetName(srcCellArrays[snappedCellIndices[s]].arr->GetName());
        outCellArrays[s]->SetDimension(srcCellArrays[snappedCellIndices[s]].dim);
        outCellArrays[s]->Resize(numberOfGridPoints);
    }

    // 插值/快照缓冲区按属性最大分量数动态分配，避免固定 16 分量的越界风险。
    int maxDim = 1;
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        maxDim = std::max(maxDim, srcPointArrays[s].dim);
    }
    for (size_t q = 0; q < snappedCellIndices.size(); ++q) {
        maxDim = std::max(maxDim, srcCellArrays[snappedCellIndices[q]].dim);
    }

    // 源单元 ghost 标记（若存在 "vtkGhostType" 单元属性），探测时跳过 ghost 单元。
    std::vector<unsigned char> srcGhostFlags(numberOfCells, 0);
    {
        auto all = mesh->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& a = all->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.attachmentType == IG_CELL && a.pointer->GetName() == "vtkGhostType" &&
                a.pointer->GetNumberOfElements() == numberOfCells) {
                for (IGsize c = 0; c < numberOfCells; ++c) {
                    srcGhostFlags[c] = static_cast<unsigned char>(a.pointer->GetElementValue(c, 0));
                }
            }
        }
    }

    // 预计算每个输入单元的包围盒，用于快速剔除。
    std::vector<std::array<double, 6>> cellBox(numberOfCells);
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        const igIndex* ids = nullptr;
        const int n = mesh->GetCellPointIds(cid, ids);
        double mn[3] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max()};
        double mx[3] = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
                        std::numeric_limits<double>::lowest()};
        for (int v = 0; v < n; ++v) {
            const Point& p = mesh->GetPoint(ids[v]);
            mn[0] = std::min(mn[0], static_cast<double>(p[0]));
            mx[0] = std::max(mx[0], static_cast<double>(p[0]));
            mn[1] = std::min(mn[1], static_cast<double>(p[1]));
            mx[1] = std::max(mx[1], static_cast<double>(p[1]));
            mn[2] = std::min(mn[2], static_cast<double>(p[2]));
            mx[2] = std::max(mx[2], static_cast<double>(p[2]));
        }
        cellBox[cid] = {mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]};
    }

    // 对每个源单元，在其包围盒覆盖的格点范围内做探针插值
    // （等价 vtkProbeFilter::ProbeImagePointsInCell：单元定向遍历，而非格点定向）。
    std::vector<double> vals(static_cast<size_t>(maxDim));
    double pts[8][3];
    double weights[8];
    for (IGsize cid = 0; cid < numberOfCells; ++cid) {
        if (srcGhostFlags[cid] != 0) continue; // 跳过 ghost 源单元
        IGenum cellType = mesh->GetCellType(cid);
        if (static_cast<IGCellType>(cellType) != IG_VERTEX &&
            static_cast<IGCellType>(cellType) != IG_LINE &&
            static_cast<IGCellType>(cellType) != IG_TRIANGLE &&
            static_cast<IGCellType>(cellType) != IG_QUAD &&
            static_cast<IGCellType>(cellType) != IG_TETRA &&
            static_cast<IGCellType>(cellType) != IG_HEXAHEDRON) {
            continue; // 不支持的单元类型
        }
        const igIndex* ids = nullptr;
        const int n = mesh->GetCellPointIds(cid, ids);
        for (int v = 0; v < n; ++v) {
            const Point& p = mesh->GetPoint(ids[v]);
            pts[v][0] = static_cast<double>(p[0]);
            pts[v][1] = static_cast<double>(p[1]);
            pts[v][2] = static_cast<double>(p[2]);
        }

        // 单元包围盒覆盖的格点 ijk 范围（floor/ceil 覆盖，避免边界漏点）
        const std::array<double, 6>& bx = cellBox[cid];
        int lo[3], hi[3];
        bool overlap = true;
        for (int ax = 0; ax < 3; ++ax) {
            if (spacing[ax] == 0.0) {
                lo[ax] = hi[ax] = 0;
                continue;
            }
            int a = static_cast<int>(std::floor((bx[ax] - origin[ax]) / spacing[ax]));
            int b = static_cast<int>(std::ceil((bx[ax + 3] - origin[ax]) / spacing[ax]));
            if (a < 0) a = 0;
            if (b > dims[ax] - 1) b = dims[ax] - 1;
            if (a > b) {
                overlap = false;
                break;
            }
            lo[ax] = a;
            hi[ax] = b;
        }
        if (!overlap) continue;

        for (int k = lo[2]; k <= hi[2]; ++k) {
            for (int j = lo[1]; j <= hi[1]; ++j) {
                for (int i = lo[0]; i <= hi[0]; ++i) {
                    const IGsize ptId = static_cast<IGsize>(i + dims[0] * (j + dims[1] * k));
                    if (mask->ValueAt(ptId) != 0) continue; // 已被前一个单元探测成功
                    const double x[3] = {origin[0] + i * spacing[0], origin[1] + j * spacing[1],
                                         origin[2] + k * spacing[2]};
                    if (!EvaluateCell(cellType, pts, n, x, weights)) continue;

                    mask->ValueAt(ptId) = 1;
                    // 插值源点属性
                    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
                        const int dim = srcPointArrays[s].dim;
                        for (int d = 0; d < dim; ++d) vals[d] = 0.0;
                        for (int v = 0; v < n; ++v) {
                            const double wv = weights[v];
                            for (int d = 0; d < dim; ++d) {
                                vals[d] += wv * srcPointArrays[s].arr->GetElementValue(ids[v], d);
                            }
                        }
                        outPointArrays[s]->SetElement(ptId, vals.data());
                    }
                    // 快照源单元属性（该点所在源单元的 cell data）
                    for (size_t q = 0; q < snappedCellIndices.size(); ++q) {
                        const SrcArray& ca = srcCellArrays[snappedCellIndices[q]];
                        for (int d = 0; d < ca.dim; ++d) {
                            vals[d] = ca.arr->GetElementValue(cid, d);
                        }
                        outCellArrays[q]->SetElement(ptId, vals.data());
                    }
                }
            }
        }
    }

    // ghost 标记（等价 vtkResampleToImage::SetBlankPointsAndCells）
    UnsignedCharArray::Pointer pointGhost = UnsignedCharArray::New();
    pointGhost->SetName("vtkGhostType");
    pointGhost->SetDimension(1);
    pointGhost->Resize(numberOfGridPoints);
    for (IGsize p = 0; p < numberOfGridPoints; ++p) {
        if (mask->ValueAt(p) == 0) {
            pointGhost->ValueAt(p) = kHiddenPoint;
        }
    }

    const int pointDim[3] = {dims[0], dims[1], dims[2]};
    const int cellDim[3] = {std::max(1, dims[0] - 1), std::max(1, dims[1] - 1),
                            std::max(1, dims[2] - 1)};
    const int span[3] = {(dims[0] > 1) ? 1 : 0, (dims[1] > 1) ? 1 : 0, (dims[2] > 1) ? 1 : 0};
    const IGsize pointSlice = static_cast<IGsize>(pointDim[0]) * pointDim[1];
    const IGsize cellSlice = static_cast<IGsize>(cellDim[0]) * cellDim[1];
    const IGsize numberOfOutCells = output->GetNumberOfCells();
    UnsignedCharArray::Pointer cellGhost = UnsignedCharArray::New();
    cellGhost->SetName("vtkGhostType");
    cellGhost->SetDimension(1);
    cellGhost->Resize(numberOfOutCells);
    for (IGsize cellId = 0; cellId < numberOfOutCells; ++cellId) {
        int ijk[3];
        ijk[2] = static_cast<int>(cellId / cellSlice);
        ijk[1] = static_cast<int>((cellId % cellSlice) / cellDim[0]);
        ijk[0] = static_cast<int>(cellId % cellDim[0]);
        const IGsize ptid = static_cast<IGsize>(ijk[0]) + pointDim[0] * ijk[1] + pointSlice * ijk[2];
        bool valid = true;
        for (int k = 0; k <= span[2]; ++k) {
            for (int j = 0; j <= span[1]; ++j) {
                for (int i = 0; i <= span[0]; ++i) {
                    valid = valid &&
                            (mask->ValueAt(ptid + static_cast<IGsize>(i) +
                                           static_cast<IGsize>(j) * pointDim[0] +
                                           static_cast<IGsize>(k) * pointSlice) != 0);
                }
            }
        }
        if (!valid) {
            cellGhost->ValueAt(cellId) = kHiddenCell;
        }
    }

    // 挂载输出属性（点数据）。
    AttributeSet* outAttrs = output->GetAttributeSet();
    for (size_t s = 0; s < srcPointArrays.size(); ++s) {
        outAttrs->AddAttribute(srcPointArrays[s].type, IG_POINT, outPointArrays[s]);
    }
    for (size_t k = 0; k < snappedCellIndices.size(); ++k) {
        outAttrs->AddAttribute(srcCellArrays[snappedCellIndices[k]].type, IG_POINT, outCellArrays[k]);
    }
    outAttrs->AddAttribute(IG_SCALAR, IG_POINT, mask);
    outAttrs->AddAttribute(IG_SCALAR, IG_POINT, pointGhost);
    outAttrs->AddAttribute(IG_SCALAR, IG_CELL, cellGhost);

    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END