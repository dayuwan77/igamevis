#include "iGameResampleToLine.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

namespace {

// 形函数参数坐标容差
constexpr double kParamTol = 1e-6;
// 支持形函数插值的最大单元点数（超出的单元退化为最近点插值）
constexpr int kMaxShapePointNum = 32;

/** 3x3 行列式（等价于 c1 . (c2 x c3)） */
inline double Det3(const double c1[3], const double c2[3], const double c3[3]) {
    return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] + c3[0] * c1[1] * c2[2] -
           c1[0] * c3[1] * c2[2] - c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}

/** 把二次/拉格朗日单元退化成对应的线性单元（只取角点） */
IGenum LinearShapeType(IGenum cellType) {
    switch (cellType) {
        case IG_TRIANGLE:
        case IG_QUADRATIC_TRIANGLE:
        case IG_BIQUADRATIC_TRIANGLE:
        case IG_LAGRANGE_TRIANGLE:
            return IG_TRIANGLE;
        case IG_QUAD:
        case IG_QUADRATIC_QUAD:
        case IG_BIQUADRATIC_QUAD:
        case IG_QUADRATIC_LINEAR_QUAD:
        case IG_LAGRANGE_QUADRILATERAL:
            return IG_QUAD;
        case IG_POLYGON:
            return IG_POLYGON;
        case IG_TETRA:
        case IG_QUADRATIC_TETRA:
        case IG_LAGRANGE_TETRAHEDRON:
            return IG_TETRA;
        case IG_HEXAHEDRON:
        case IG_QUADRATIC_HEXAHEDRON:
        case IG_TRIQUADRATIC_HEXAHEDRON:
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case IG_LAGRANGE_HEXAHEDRON:
            return IG_HEXAHEDRON;
        case IG_PRISM:
        case IG_QUADRATIC_PRISM:
        case IG_QUADRATIC_LINEAR_WEDGE:
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:
        case IG_LAGRANGE_PRISM:
            return IG_PRISM;
        case IG_PYRAMID:
        case IG_QUADRATIC_PYRAMID:
        case IG_TRIQUADRATIC_PYRAMID:
        case IG_LAGRANGE_PYRAMID:
            return IG_PYRAMID;
        default:
            return IG_EMPTY_CELL;
    }
}

int ShapePointCount(IGenum shape) {
    switch (shape) {
        case IG_TRIANGLE:
            return 3;
        case IG_QUAD:
            return 4;
        case IG_TETRA:
            return 4;
        case IG_PYRAMID:
            return 5;
        case IG_PRISM:
            return 6;
        case IG_HEXAHEDRON:
            return 8;
        default:
            return 0;
    }
}

/* ---------------- 六面体形函数（VTK 节点顺序，参数坐标 r,s,t ∈ [0,1]） ---------------- */
void HexShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double rm = 1.0 - r, sm = 1.0 - s, tm = 1.0 - t;
    sf[0] = rm * sm * tm;
    sf[1] = r * sm * tm;
    sf[2] = r * s * tm;
    sf[3] = rm * s * tm;
    sf[4] = rm * sm * t;
    sf[5] = r * sm * t;
    sf[6] = r * s * t;
    sf[7] = rm * s * t;
}

void HexDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double rm = 1.0 - r, sm = 1.0 - s, tm = 1.0 - t;
    // d/dr
    d[0] = -sm * tm;
    d[1] = sm * tm;
    d[2] = s * tm;
    d[3] = -s * tm;
    d[4] = -sm * t;
    d[5] = sm * t;
    d[6] = s * t;
    d[7] = -s * t;
    // d/ds
    d[8] = -rm * tm;
    d[9] = -r * tm;
    d[10] = r * tm;
    d[11] = rm * tm;
    d[12] = -rm * t;
    d[13] = -r * t;
    d[14] = r * t;
    d[15] = rm * t;
    // d/dt
    d[16] = -rm * sm;
    d[17] = -r * sm;
    d[18] = -r * s;
    d[19] = -rm * s;
    d[20] = rm * sm;
    d[21] = r * sm;
    d[22] = r * s;
    d[23] = rm * s;
}

/* ---------------- 三棱柱（楔形）形函数：r,s ∈ [0,1], r+s <= 1, t ∈ [0,1] ---------------- */
void PrismShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double tm = 1.0 - t;
    sf[0] = (1.0 - r - s) * tm;
    sf[1] = r * tm;
    sf[2] = s * tm;
    sf[3] = (1.0 - r - s) * t;
    sf[4] = r * t;
    sf[5] = s * t;
}

void PrismDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double tm = 1.0 - t;
    // d/dr
    d[0] = -tm;
    d[1] = tm;
    d[2] = 0.0;
    d[3] = -t;
    d[4] = t;
    d[5] = 0.0;
    // d/ds
    d[6] = -tm;
    d[7] = 0.0;
    d[8] = tm;
    d[9] = -t;
    d[10] = 0.0;
    d[11] = t;
    // d/dt
    d[12] = -(1.0 - r - s);
    d[13] = -r;
    d[14] = -s;
    d[15] = (1.0 - r - s);
    d[16] = r;
    d[17] = s;
}

/* ---------------- 金字塔形函数：底面 0-1-2-3，顶点 4，r,s,t ∈ [0,1] ---------------- */
void PyramidShape(const double pc[3], double* sf) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double tm = 1.0 - t;
    sf[0] = (1.0 - r) * (1.0 - s) * tm;
    sf[1] = r * (1.0 - s) * tm;
    sf[2] = r * s * tm;
    sf[3] = (1.0 - r) * s * tm;
    sf[4] = t;
}

void PyramidDerivs(const double pc[3], double* d) {
    const double r = pc[0], s = pc[1], t = pc[2];
    const double tm = 1.0 - t;
    // d/dr
    d[0] = -(1.0 - s) * tm;
    d[1] = (1.0 - s) * tm;
    d[2] = s * tm;
    d[3] = -s * tm;
    d[4] = 0.0;
    // d/ds
    d[5] = -(1.0 - r) * tm;
    d[6] = -r * tm;
    d[7] = r * tm;
    d[8] = (1.0 - r) * tm;
    d[9] = 0.0;
    // d/dt
    d[10] = -(1.0 - r) * (1.0 - s);
    d[11] = -r * (1.0 - s);
    d[12] = -r * s;
    d[13] = -(1.0 - r) * s;
    d[14] = 1.0;
}

/* ---------------- 四边形（双线性）形函数：r,s ∈ [0,1] ---------------- */
void QuadShape(const double pc[2], double* sf) {
    const double r = pc[0], s = pc[1];
    sf[0] = (1.0 - r) * (1.0 - s);
    sf[1] = r * (1.0 - s);
    sf[2] = r * s;
    sf[3] = (1.0 - r) * s;
}

void QuadDerivs(const double pc[2], double* d) {
    const double r = pc[0], s = pc[1];
    // d/dr
    d[0] = -(1.0 - s);
    d[1] = (1.0 - s);
    d[2] = s;
    d[3] = -s;
    // d/ds
    d[4] = -(1.0 - r);
    d[5] = -r;
    d[6] = r;
    d[7] = (1.0 - r);
}

/** 统一的形函数求值入口 */
void ShapeFunctions(IGenum shape, const double pc[3], double* sf) {
    switch (shape) {
        case IG_HEXAHEDRON:
            HexShape(pc, sf);
            break;
        case IG_PRISM:
            PrismShape(pc, sf);
            break;
        case IG_PYRAMID:
            PyramidShape(pc, sf);
            break;
        default:
            break;
    }
}

/** 统一的形函数偏导求值入口（3 * npts，按 dr、ds、dt 分块） */
void ShapeDerivs(IGenum shape, const double pc[3], double* d) {
    switch (shape) {
        case IG_HEXAHEDRON:
            HexDerivs(pc, d);
            break;
        case IG_PRISM:
            PrismDerivs(pc, d);
            break;
        case IG_PYRAMID:
            PyramidDerivs(pc, d);
            break;
        default:
            break;
    }
}

/**
 * 3D 参数单元（六面体 / 三棱柱 / 金字塔）的逆映射：
 * 用 Newton 迭代求解参数坐标，返回形函数权重与残差（|ΣN·x - p|）
 */
bool NewtonParametric3D(IGenum shape, const std::vector<Point>& pts, const Point& target, double pc[3],
                        std::vector<double>& weights, double& residual) {
    const int npts = ShapePointCount(shape);
    if (npts <= 0 || static_cast<int>(pts.size()) < npts) { return false; }

    if (shape == IG_PRISM) {
        pc[0] = 1.0 / 3.0;
        pc[1] = 1.0 / 3.0;
        pc[2] = 0.5;
    } else {
        pc[0] = 0.5;
        pc[1] = 0.5;
        pc[2] = 0.5;
    }

    double sf[kMaxShapePointNum];
    double dN[3 * kMaxShapePointNum];

    for (int iter = 0; iter < 24; ++iter) {
        ShapeFunctions(shape, pc, sf);
        ShapeDerivs(shape, pc, dN);

        double rcol[3] = {0.0, 0.0, 0.0};
        double scol[3] = {0.0, 0.0, 0.0};
        double tcol[3] = {0.0, 0.0, 0.0};
        double fcol[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < npts; ++i) {
            const Point& x = pts[i];
            for (int j = 0; j < 3; ++j) {
                const double xv = static_cast<double>(x[j]);
                fcol[j] += sf[i] * xv;
                rcol[j] += dN[i] * xv;
                scol[j] += dN[i + npts] * xv;
                tcol[j] += dN[i + 2 * npts] * xv;
            }
        }
        for (int j = 0; j < 3; ++j) { fcol[j] -= static_cast<double>(target[j]); }

        const double detJ = Det3(rcol, scol, tcol);
        if (std::fabs(detJ) < 1e-30) { break; }

        const double negF[3] = {-fcol[0], -fcol[1], -fcol[2]};
        const double dp0 = Det3(negF, scol, tcol) / detJ;
        const double dp1 = Det3(rcol, negF, tcol) / detJ;
        const double dp2 = Det3(rcol, scol, negF) / detJ;

        pc[0] += dp0;
        pc[1] += dp1;
        pc[2] += dp2;

        if (std::fabs(dp0) + std::fabs(dp1) + std::fabs(dp2) < 1e-10) { break; }
        if (std::fabs(pc[0]) > 1e3 || std::fabs(pc[1]) > 1e3 || std::fabs(pc[2]) > 1e3) { return false; }
    }

    ShapeFunctions(shape, pc, sf);
    weights.assign(sf, sf + npts);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < npts; ++i) { x += pts[i] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - target).length());
    return true;
}

/**
 * 四边形（2D 单元）的逆映射：先把单元与目标点投影到单元平面上做 2D Newton，
 * 再回到 3D 计算残差作为"是否落在单元上"的判据
 */
bool QuadParametric(const std::vector<Point>& pts, const Point& p, double pc[2],
                    std::vector<double>& weights, double& residual) {
    if (pts.size() < 4) { return false; }

    // 用对角线构造单元平面的局部正交基
    Vector3f d1 = pts[2] - pts[0];
    Vector3f d2 = pts[3] - pts[1];
    Vector3f n = CrossProduct(d1, d2);
    const double nl = n.length();
    if (nl < 1e-20) { return false; }
    n = n / static_cast<float>(nl);

    Vector3f u = pts[1] - pts[0];
    u = u - n * DotProduct(u, n);
    double ul = u.length();
    if (ul < 1e-20) {
        u = pts[3] - pts[0];
        u = u - n * DotProduct(u, n);
        ul = u.length();
        if (ul < 1e-20) { return false; }
    }
    u = u / static_cast<float>(ul);
    Vector3f v = CrossProduct(n, u);

    double q[4][2];
    for (int i = 0; i < 4; ++i) {
        const Vector3f diff = pts[i] - pts[0];
        q[i][0] = static_cast<double>(DotProduct(diff, u));
        q[i][1] = static_cast<double>(DotProduct(diff, v));
    }
    const Vector3f pd = p - pts[0];
    const double tp[2] = {static_cast<double>(DotProduct(pd, u)), static_cast<double>(DotProduct(pd, v))};

    pc[0] = 0.5;
    pc[1] = 0.5;
    for (int iter = 0; iter < 20; ++iter) {
        double sf[4], dN[8];
        QuadShape(pc, sf);
        QuadDerivs(pc, dN);

        double F[2] = {0.0, 0.0};
        double J[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
        for (int i = 0; i < 4; ++i) {
            F[0] += sf[i] * q[i][0];
            F[1] += sf[i] * q[i][1];
            J[0][0] += dN[i] * q[i][0];
            J[1][0] += dN[i] * q[i][1];
            J[0][1] += dN[i + 4] * q[i][0];
            J[1][1] += dN[i + 4] * q[i][1];
        }
        F[0] -= tp[0];
        F[1] -= tp[1];

        const double detJ = J[0][0] * J[1][1] - J[0][1] * J[1][0];
        if (std::fabs(detJ) < 1e-30) { break; }

        const double dr = (-F[0] * J[1][1] + F[1] * J[0][1]) / detJ;
        const double ds = (-F[1] * J[0][0] + F[0] * J[1][0]) / detJ;
        pc[0] += dr;
        pc[1] += ds;

        if (std::fabs(dr) + std::fabs(ds) < 1e-12) { break; }
        if (pc[0] < -2.0 || pc[0] > 3.0 || pc[1] < -2.0 || pc[1] > 3.0) { break; }
    }

    double sf[4];
    QuadShape(pc, sf);
    weights.assign(sf, sf + 4);

    Point x(0.f, 0.f, 0.f);
    for (int i = 0; i < 4; ++i) { x += pts[i] * static_cast<float>(sf[i]); }
    residual = static_cast<double>((x - p).length());
    return true;
}

/** 点到线段的最近点（返回距离平方） */
void ClosestPointOnSegment(const Point& p, const Point& a, const Point& b, Point& closest, double& distSq) {
    const Vector3f ab = b - a;
    const double denom = static_cast<double>(DotProduct(ab, ab));
    if (denom < 1e-24) {
        closest = a;
        distSq = static_cast<double>((p - a).squaredLength());
        return;
    }
    double t = static_cast<double>(DotProduct(p - a, ab)) / denom;
    t = std::min(1.0, std::max(0.0, t));
    closest = a + ab * static_cast<float>(t);
    distSq = static_cast<double>((closest - p).squaredLength());
}

/** 点到三角形的最近点（返回距离平方） */
void ClosestPointOnTriangle(const Point& p, const Point& a, const Point& b, const Point& c, Point& closest,
                            double& distSq) {
    const Vector3f ab = b - a;
    const Vector3f ac = c - a;
    const Vector3f ap = p - a;

    const double d1 = static_cast<double>(DotProduct(ab, ap));
    const double d2 = static_cast<double>(DotProduct(ac, ap));
    if (d1 <= 0.0 && d2 <= 0.0) {
        closest = a;
        distSq = static_cast<double>(DotProduct(ap, ap));
        return;
    }

    const Vector3f bp = p - b;
    const double d3 = static_cast<double>(DotProduct(ab, bp));
    const double d4 = static_cast<double>(DotProduct(ac, bp));
    if (d3 >= 0.0 && d4 <= d3) {
        closest = b;
        distSq = static_cast<double>(DotProduct(bp, bp));
        return;
    }

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double t = (d1 - d3) != 0.0 ? d1 / (d1 - d3) : 0.0;
        closest = a + ab * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const Vector3f cp = p - c;
    const double d5 = static_cast<double>(DotProduct(ab, cp));
    const double d6 = static_cast<double>(DotProduct(ac, cp));
    if (d6 >= 0.0 && d5 <= d6) {
        closest = c;
        distSq = static_cast<double>(DotProduct(cp, cp));
        return;
    }

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double t = (d2 - d6) != 0.0 ? d2 / (d2 - d6) : 0.0;
        closest = a + ac * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double denom = (d4 - d3) + (d5 - d6);
        const double t = denom != 0.0 ? (d4 - d3) / denom : 0.0;
        closest = b + (c - b) * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    // 投影落在三角形内部
    const Vector3f n = CrossProduct(ab, ac);
    const double denom = static_cast<double>(DotProduct(n, n));
    if (denom > 1e-24) {
        const double t = static_cast<double>(DotProduct(n, ap)) / denom;
        closest = p - n * static_cast<float>(t);
        distSq = static_cast<double>((closest - p).squaredLength());
        return;
    }

    // 退化三角形
    closest = a;
    double minDistSq = static_cast<double>(DotProduct(ap, ap));
    const double distB = static_cast<double>(DotProduct(bp, bp));
    if (distB < minDistSq) {
        closest = b;
        minDistSq = distB;
    }
    const double distC = static_cast<double>(DotProduct(cp, cp));
    if (distC < minDistSq) {
        closest = c;
        minDistSq = distC;
    }
    distSq = minDistSq;
}

/** 三角形重心坐标（按投影点计算），同时返回点到三角面的距离 */
bool TriangleBarycentric(const Point& p, const Point& a, const Point& b, const Point& c, double w[3],
                         double& planeDist) {
    const Vector3f n = CrossProduct(b - a, c - a);
    const double nn = static_cast<double>(DotProduct(n, n));
    if (nn < 1e-24) { return false; }

    const Vector3f ap = p - a;
    const double t = static_cast<double>(DotProduct(n, ap)) / nn;
    planeDist = std::fabs(t) * std::sqrt(nn);
    const Vector3f proj = p - n * static_cast<float>(t);

    const Vector3f v0 = b - a;
    const Vector3f v1 = c - a;
    const Vector3f v2 = proj - a;
    const double d00 = static_cast<double>(DotProduct(v0, v0));
    const double d01 = static_cast<double>(DotProduct(v0, v1));
    const double d11 = static_cast<double>(DotProduct(v1, v1));
    const double d20 = static_cast<double>(DotProduct(v2, v0));
    const double d21 = static_cast<double>(DotProduct(v2, v1));
    const double denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-24) { return false; }

    const double l1 = (d11 * d20 - d01 * d21) / denom;
    const double l2 = (d00 * d21 - d01 * d20) / denom;
    w[0] = 1.0 - l1 - l2;
    w[1] = l1;
    w[2] = l2;
    return true;
}

/** 按输入数组类型创建同类型的输出数组（保留数组数据类型） */
ArrayObject::Pointer CreateArrayObject(IGenum arrayType) {
    switch (arrayType) {
        case IG_FloatArray:
            return FloatArray::New();
        case IG_DoubleArray:
            return DoubleArray::New();
        case IG_IntArray:
            return IntArray::New();
        case IG_UnsignedIntArray:
            return UnsignedIntArray::New();
        case IG_CharArray:
            return CharArray::New();
        case IG_UnsignedCharArray:
            return UnsignedCharArray::New();
        case IG_ShortArray:
            return ShortArray::New();
        case IG_UnsignedShortArray:
            return UnsignedShortArray::New();
        case IG_LongLongArray:
            return LongLongArray::New();
        case IG_UnsignedLongLongArray:
            return UnsignedLongLongArray::New();
        default:
            return FloatArray::New();
    }
}

/** 权重规整：去掉微小负值并归一化，避免外插产生异常值 */
void NormalizeWeights(std::vector<double>& w) {
    double sum = 0.0;
    for (double& v : w) {
        if (v < 0.0) { v = 0.0; }
        sum += v;
    }
    if (sum <= 1e-30) {
        const double v = w.empty() ? 0.0 : 1.0 / static_cast<double>(w.size());
        for (double& x : w) { x = v; }
        return;
    }
    for (double& v : w) { v /= sum; }
}

} // namespace

/* ------------------------------------------------------------------ */
/* 参数设置                                                            */
/* ------------------------------------------------------------------ */
void ResampleToLine::setOrigTarget(const Vector3d& p0, const Vector3d& p1, const int& x) {
    orig = Point(static_cast<float>(p0[0]), static_cast<float>(p0[1]), static_cast<float>(p0[2]));
    target = Point(static_cast<float>(p1[0]), static_cast<float>(p1[1]), static_cast<float>(p1[2]));
    n = x;
}

/* ------------------------------------------------------------------ */
/* 主流程                                                              */
/* ------------------------------------------------------------------ */
bool ResampleToLine::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "未选择输入数据";
        return false;
    }
    if (n < 2) {
        m_Message = "n must be greater than 1";
        return false;
    }

    // 优先使用 UnstructuredMesh；SurfaceMesh / VolumeMesh 先转换为 UnstructuredMesh，
    // 这样体单元（四面体/六面体/…）与面单元（三角形/四边形/…）都能参与单元定位与插值
    auto mesh = DynamicCast<UnstructuredMesh>(input);
    if (mesh == nullptr) {
        mesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    }
    if (mesh == nullptr) {
        m_Message = "please input UnstructuredMesh / SurfaceMesh / VolumeMesh";
        return false;
    }
    if (mesh->GetNumberOfPoints() == 0 || mesh->GetNumberOfCells() == 0) {
        m_Message = "Mesh has no cells or points.";
        return false;
    }

    const BoundingBox& bbox = mesh->GetBoundingBox();
    const double diag = std::max(bbox.diag(), 1e-12);
    const double distTol = std::max(1e-9, m_Tolerance * diag);

    // 1. 沿线均匀生成采样点
    Points::Pointer samples = Points::New();
    std::vector<SampleLocation> locations(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(n - 1);
        const Point p(static_cast<float>((1.0 - t) * orig[0] + t * target[0]),
                      static_cast<float>((1.0 - t) * orig[1] + t * target[1]),
                      static_cast<float>((1.0 - t) * orig[2] + t * target[2]));
        samples->AddPoint(p);
        locations[static_cast<size_t>(i)].point = p;
    }

    // 2. 构建均匀网格加速结构（网格分辨率随单元数自适应）
    const double cellNum = static_cast<double>(mesh->GetNumberOfCells());
    igIndex gridDim = static_cast<igIndex>(std::cbrt(cellNum) + 0.5);
    gridDim = std::max<igIndex>(4, std::min<igIndex>(64, gridDim));
    const igIndex nx = gridDim, ny = gridDim, nz = gridDim;
    auto grid = BuildUniformGrid(mesh, bbox, nx, ny, nz);

    // 3. 逐采样点定位单元并计算形函数权重
    m_SampleCellIds.assign(static_cast<size_t>(n), -1);
    m_SampleCellInside.assign(static_cast<size_t>(n), 0);
    int hitCount = 0;
    int insideCount = 0;
    for (int i = 0; i < n; ++i) {
        SampleLocation loc;
        if (LocateSample(mesh, samples->GetPoint(i), bbox, grid, nx, ny, nz, distTol, loc) && loc.cellId >= 0) {
            locations[static_cast<size_t>(i)] = loc;
            m_SampleCellIds[static_cast<size_t>(i)] = loc.cellId;
            m_SampleCellInside[static_cast<size_t>(i)] = loc.inside ? 1 : 0;
            ++hitCount;
            if (loc.inside) { ++insideCount; }
        }
    }

    // 4. 插值 Point Data / 复制 Cell Data
    AttributeSet* inAttr = mesh->GetAttributeSet();
    AttributeSet::Pointer outAttr = AttributeSet::New();
    InterpolatePointData(inAttr, outAttr, locations, n);
    CopyCellData(inAttr, outAttr, locations, n);

    // 5. 生成折线输出
    BuildPolyLineOutputs(samples, outAttr, n);

    m_Message = "ResampleToLine: " + std::to_string(n) + " samples, " + std::to_string(insideCount) +
                " inside cells, " + std::to_string(hitCount - insideCount) + " snapped to nearest cell";
    return true;
}

/* ------------------------------------------------------------------ */
/* 均匀网格                                                            */
/* ------------------------------------------------------------------ */
std::vector<std::vector<igIndex>> ResampleToLine::BuildUniformGrid(const UnstructuredMesh::Pointer& mesh,
                                                                  const BoundingBox& bbox, igIndex nx, igIndex ny,
                                                                  igIndex nz) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    std::vector<std::vector<igIndex>> grid(static_cast<size_t>(nx) * ny * nz);

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    for (igIndex cellId = 0; cellId < static_cast<igIndex>(mesh->GetNumberOfCells()); ++cellId) {
        const int npts = mesh->GetCellPointIds(cellId, ptIds);
        if (npts <= 0) { continue; }

        double minv[3] = {DBL_MAX, DBL_MAX, DBL_MAX};
        double maxv[3] = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
        for (int k = 0; k < npts; ++k) {
            // 点单元索引可能被删除标记影响，这里直接按点坐标取包围盒
            const Point& p = mesh->GetPoint(ptIds[k]);
            for (int j = 0; j < 3; ++j) {
                const double v = static_cast<double>(p[j]);
                minv[j] = std::min(minv[j], v);
                maxv[j] = std::max(maxv[j], v);
            }
        }

        const igIndex ixMin = std::max<igIndex>(0, static_cast<igIndex>((minv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMin = std::max<igIndex>(0, static_cast<igIndex>((minv[1] - bbox.min[1]) / voxelY));
        const igIndex izMin = std::max<igIndex>(0, static_cast<igIndex>((minv[2] - bbox.min[2]) / voxelZ));
        const igIndex ixMax = std::min<igIndex>(nx - 1, static_cast<igIndex>((maxv[0] - bbox.min[0]) / voxelX));
        const igIndex iyMax = std::min<igIndex>(ny - 1, static_cast<igIndex>((maxv[1] - bbox.min[1]) / voxelY));
        const igIndex izMax = std::min<igIndex>(nz - 1, static_cast<igIndex>((maxv[2] - bbox.min[2]) / voxelZ));

        for (igIndex ix = ixMin; ix <= ixMax; ++ix) {
            for (igIndex iy = iyMin; iy <= iyMax; ++iy) {
                for (igIndex iz = izMin; iz <= izMax; ++iz) {
                    grid[static_cast<size_t>(ix) + static_cast<size_t>(iy) * nx + static_cast<size_t>(iz) * nx * ny]
                            .push_back(cellId);
                }
            }
        }
    }

    return grid;
}

/* ------------------------------------------------------------------ */
/* 采样点定位                                                          */
/* ------------------------------------------------------------------ */
bool ResampleToLine::LocateSample(const UnstructuredMesh::Pointer& mesh, const Point& p, const BoundingBox& bbox,
                                  const std::vector<std::vector<igIndex>>& grid, igIndex nx, igIndex ny, igIndex nz,
                                  double distTol, SampleLocation& out) {
    double voxelX = (bbox.max[0] - bbox.min[0]) / static_cast<double>(nx);
    double voxelY = (bbox.max[1] - bbox.min[1]) / static_cast<double>(ny);
    double voxelZ = (bbox.max[2] - bbox.min[2]) / static_cast<double>(nz);
    if (voxelX < 1e-12) { voxelX = 1.0; }
    if (voxelY < 1e-12) { voxelY = 1.0; }
    if (voxelZ < 1e-12) { voxelZ = 1.0; }

    const igIndex ix = std::max<igIndex>(0, std::min<igIndex>(nx - 1, static_cast<igIndex>((p[0] - bbox.min[0]) / voxelX)));
    const igIndex iy = std::max<igIndex>(0, std::min<igIndex>(ny - 1, static_cast<igIndex>((p[1] - bbox.min[1]) / voxelY)));
    const igIndex iz = std::max<igIndex>(0, std::min<igIndex>(nz - 1, static_cast<igIndex>((p[2] - bbox.min[2]) / voxelZ)));

    // 从所在体素开始逐层向外扩展，直到找到候选单元
    std::vector<igIndex> candidates;
    const igIndex maxRadius = std::max<igIndex>(nx, std::max<igIndex>(ny, nz));
    for (igIndex r = 0; r <= maxRadius && candidates.empty(); ++r) {
        for (igIndex dx = -r; dx <= r; ++dx) {
            for (igIndex dy = -r; dy <= r; ++dy) {
                for (igIndex dz = -r; dz <= r; ++dz) {
                    if (std::max(std::abs(dx), std::max(std::abs(dy), std::abs(dz))) != r) { continue; }
                    const igIndex cx = ix + dx, cy = iy + dy, cz = iz + dz;
                    if (cx < 0 || cx >= nx || cy < 0 || cy >= ny || cz < 0 || cz >= nz) { continue; }
                    const auto& list = grid[static_cast<size_t>(cx) + static_cast<size_t>(cy) * nx +
                                            static_cast<size_t>(cz) * nx * ny];
                    candidates.insert(candidates.end(), list.begin(), list.end());
                }
            }
        }
    }
    if (candidates.empty()) { return false; }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    // 优先命中"真正包含采样点"的单元
    double bestDistSq = DBL_MAX;
    igIndex bestCell = -1;
    Point bestClosest(0.f, 0.f, 0.f);

    for (const igIndex cellId : candidates) {
        std::vector<double> weights;
        bool inside = false;
        if (ComputeCellWeights(mesh, cellId, p, distTol, weights, inside) && inside) {
            out.cellId = cellId;
            out.inside = true;
            out.point = p;
            out.weights = std::move(weights);
            igIndex ptIds[IGAME_CELL_MAX_SIZE];
            const int npts = mesh->GetCellPointIds(cellId, ptIds);
            out.pointIds.assign(ptIds, ptIds + npts);
            return true;
        }

        if (m_SnapToNearest) {
            Point closest(0.f, 0.f, 0.f);
            const double d2 = DistanceToCell(mesh, cellId, p, closest);
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                bestCell = cellId;
                bestClosest = closest;
            }
        }
    }

    if (!m_SnapToNearest || bestCell < 0) { return false; }

    // 没有单元包含采样点：吸附到最近单元，并用最近点处的形函数权重插值
    out.cellId = bestCell;
    out.inside = false;
    out.point = bestClosest;
    {
        igIndex ptIds[IGAME_CELL_MAX_SIZE];
        const int npts = mesh->GetCellPointIds(bestCell, ptIds);
        out.pointIds.assign(ptIds, ptIds + npts);
    }
    {
        std::vector<double> weights;
        bool inside = false;
        if (!ComputeCellWeights(mesh, bestCell, bestClosest, distTol, weights, inside) || weights.empty()) {
            // 极端退化单元：退化为按距离反比加权
            const int npts = static_cast<int>(out.pointIds.size());
            weights.assign(static_cast<size_t>(npts), 0.0);
            double sum = 0.0;
            for (int i = 0; i < npts; ++i) {
                const double d2 = static_cast<double>((mesh->GetPoint(out.pointIds[static_cast<size_t>(i)]) - bestClosest)
                                                              .squaredLength());
                const double w = 1.0 / (d2 + 1e-30);
                weights[static_cast<size_t>(i)] = w;
                sum += w;
            }
            if (sum > 0.0) {
                for (double& w : weights) { w /= sum; }
            }
        }
        out.weights = std::move(weights);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* 形函数权重                                                          */
/* ------------------------------------------------------------------ */
bool ResampleToLine::ComputeCellWeights(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                        double distTol, std::vector<double>& weights, bool& inside) {
    weights.clear();
    inside = false;

    auto cells = mesh->GetCells();
    if (cells == nullptr) { return false; }
    const IGuint cellSize = cells->GetCellSize(cellId);
    if (cellSize == 0 || cellSize > static_cast<IGuint>(kMaxShapePointNum)) { return false; }

    igIndex ptIds[IGAME_CELL_MAX_SIZE];
    const int npts = mesh->GetCellPointIds(cellId, ptIds);
    if (npts <= 0) { return false; }

    std::vector<Point> pts(static_cast<size_t>(npts));
    for (int i = 0; i < npts; ++i) { pts[static_cast<size_t>(i)] = mesh->GetPoint(ptIds[i]); }

    const IGenum shape = LinearShapeType(mesh->GetCellType(cellId));
    const int shapeNum = ShapePointCount(shape);
    if (shapeNum == 0 || npts < shapeNum) { return false; }

    if (shape == IG_TETRA) {
        // 四面体：直接求解重心坐标（线性形函数，精确解）
        const Point& a = pts[0];
        const Point& b = pts[1];
        const Point& c = pts[2];
        const Point& d = pts[3];
        double p10[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        double p20[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        double p30[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
        double rhs[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};

        const double det = Det3(p10, p20, p30);
        if (std::fabs(det) < 1e-30) { return false; }

        double w[4];
        w[1] = Det3(rhs, p20, p30) / det;
        w[2] = Det3(p10, rhs, p30) / det;
        w[3] = Det3(p10, p20, rhs) / det;
        w[0] = 1.0 - w[1] - w[2] - w[3];

        inside = w[0] >= -kParamTol && w[1] >= -kParamTol && w[2] >= -kParamTol && w[3] >= -kParamTol;
        weights.assign(w, w + 4);
        NormalizeWeights(weights);
        return true;
    }

    if (shape == IG_TRIANGLE) {
        double w[3];
        double planeDist = 0.0;
        if (!TriangleBarycentric(p, pts[0], pts[1], pts[2], w, planeDist)) { return false; }
        inside = planeDist <= distTol && w[0] >= -kParamTol && w[1] >= -kParamTol && w[2] >= -kParamTol;
        weights.assign(w, w + 3);
        NormalizeWeights(weights);
        return true;
    }

    if (shape == IG_POLYGON) {
        // 多边形：以 0 号点为扇心做扇形三角剖分，取包含投影点的三角形
        double bestPlaneDist = DBL_MAX;
        std::vector<double> bestWeights;
        for (int k = 1; k + 1 < npts; ++k) {
            double w[3];
            double planeDist = 0.0;
            if (!TriangleBarycentric(p, pts[0], pts[k], pts[k + 1], w, planeDist)) { continue; }
            if (w[0] >= -kParamTol && w[1] >= -kParamTol && w[2] >= -kParamTol && planeDist <= distTol) {
                std::vector<double> cand(static_cast<size_t>(npts), 0.0);
                cand[0] = w[0];
                cand[static_cast<size_t>(k)] = w[1];
                cand[static_cast<size_t>(k + 1)] = w[2];
                weights = std::move(cand);
                NormalizeWeights(weights);
                inside = true;
                return true;
            }
            if (planeDist < bestPlaneDist) {
                bestPlaneDist = planeDist;
                bestWeights.assign(static_cast<size_t>(npts), 0.0);
                bestWeights[0] = w[0];
                bestWeights[static_cast<size_t>(k)] = w[1];
                bestWeights[static_cast<size_t>(k + 1)] = w[2];
            }
        }
        if (!bestWeights.empty()) {
            weights = std::move(bestWeights);
            NormalizeWeights(weights);
            return true;
        }
        return false;
    }

    if (shape == IG_QUAD) {
        double pc[2] = {0.5, 0.5};
        double residual = 0.0;
        std::vector<double> w;
        if (!QuadParametric(pts, p, pc, w, residual)) { return false; }
        inside = residual <= distTol && pc[0] >= -kParamTol && pc[0] <= 1.0 + kParamTol &&
                 pc[1] >= -kParamTol && pc[1] <= 1.0 + kParamTol;
        weights = std::move(w);
        NormalizeWeights(weights);
        return true;
    }

    // 六面体 / 三棱柱 / 金字塔：Newton 求解参数坐标
    double pc[3] = {0.5, 0.5, 0.5};
    double residual = 0.0;
    std::vector<double> w;
    if (!NewtonParametric3D(shape, pts, p, pc, w, residual)) { return false; }

    bool inDomain = pc[0] >= -kParamTol && pc[0] <= 1.0 + kParamTol && pc[1] >= -kParamTol &&
                    pc[1] <= 1.0 + kParamTol && pc[2] >= -kParamTol && pc[2] <= 1.0 + kParamTol;
    if (shape == IG_PRISM) {
        inDomain = inDomain && (pc[0] + pc[1]) <= 1.0 + kParamTol;
    }
    inside = inDomain && residual <= distTol;

    weights = std::move(w);
    NormalizeWeights(weights);
    return true;
}

/* ------------------------------------------------------------------ */
/* 点到单元的距离（取所有面片上最近距离的最小值）                       */
/* ------------------------------------------------------------------ */
double ResampleToLine::DistanceToCell(const UnstructuredMesh::Pointer& mesh, igIndex cellId, const Point& p,
                                      Point& closest) {
    double best = DBL_MAX;
    closest = p;

    auto cell = mesh->GetCell(cellId);
    if (cell == nullptr) { return best; }

    const int nfaces = cell->GetNumberOfFaces();
    for (int f = 0; f < nfaces; ++f) {
        auto face = cell->GetFace(f);
        if (face == nullptr) { continue; }
        const int fn = face->GetNumberOfPoints();
        if (fn < 3) {
            for (int k = 0; k + 1 < fn; ++k) {
                Point c(0.f, 0.f, 0.f);
                double d2 = 0.0;
                ClosestPointOnSegment(p, face->GetPoint(k), face->GetPoint(k + 1), c, d2);
                if (d2 < best) {
                    best = d2;
                    closest = c;
                }
            }
            continue;
        }
        // 多边形面按扇形三角剖分
        for (int k = 1; k + 1 < fn; ++k) {
            Point c(0.f, 0.f, 0.f);
            double d2 = 0.0;
            ClosestPointOnTriangle(p, face->GetPoint(0), face->GetPoint(k), face->GetPoint(k + 1), c, d2);
            if (d2 < best) {
                best = d2;
                closest = c;
            }
        }
    }

    // 退化单元（没有面）：退化为点距
    if (best == DBL_MAX) {
        const int npts = cell->GetNumberOfPoints();
        for (int i = 0; i < npts; ++i) {
            const double d2 = static_cast<double>((cell->GetPoint(i) - p).squaredLength());
            if (d2 < best) {
                best = d2;
                closest = cell->GetPoint(i);
            }
        }
    }
    return best;
}

/* ------------------------------------------------------------------ */
/* 属性插值                                                            */
/* ------------------------------------------------------------------ */
void ResampleToLine::InterpolatePointData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                          const std::vector<SampleLocation>& locations, int sampleNum) {
    if (inSet == nullptr || outSet == nullptr) { return; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_POINT) { continue; }

        auto src = attr.pointer;
        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // 保留合理的数据类型：浮点数组沿用原类型，整型等插值后使用 float
        IGenum outArrayType = src->GetArrayType();
        if (outArrayType != IG_FloatArray && outArrayType != IG_DoubleArray) { outArrayType = IG_FloatArray; }

        auto dst = CreateArrayObject(outArrayType);
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0);
            const SampleLocation& loc = locations[static_cast<size_t>(j)];
            const size_t npts = loc.pointIds.size();
            if (loc.cellId >= 0 && npts > 0 && loc.weights.size() == npts) {
                for (size_t k = 0; k < npts; ++k) {
                    const double w = loc.weights[k];
                    if (w == 0.0) { continue; }
                    const igIndex ptId = loc.pointIds[k];
                    for (int c = 0; c < dim; ++c) {
                        values[static_cast<size_t>(c)] += w * src->GetElementValue(ptId, c);
                    }
                }
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
}

void ResampleToLine::CopyCellData(AttributeSet* inSet, AttributeSet::Pointer outSet,
                                  const std::vector<SampleLocation>& locations, int sampleNum) {
    if (inSet == nullptr || outSet == nullptr) { return; }
    auto all = inSet->GetAllAttributes();
    if (all == nullptr) { return; }

    std::vector<double> values;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != IG_CELL) { continue; }

        auto src = attr.pointer;
        const int dim = src->GetDimension();
        if (dim <= 0) { continue; }

        // Cell Data 是复制关系，完整保留原数组数据类型与分量数
        auto dst = CreateArrayObject(src->GetArrayType());
        if (dst == nullptr) { continue; }
        dst->SetName(src->GetName());
        dst->SetDimension(dim);
        dst->Resize(static_cast<IGsize>(sampleNum));

        const IGsize cellNum = src->GetNumberOfElements();
        values.assign(static_cast<size_t>(dim), 0.0);
        for (int j = 0; j < sampleNum; ++j) {
            std::fill(values.begin(), values.end(), 0.0);
            const igIndex cellId = locations[static_cast<size_t>(j)].cellId;
            if (cellId >= 0 && static_cast<IGsize>(cellId) < cellNum) {
                src->GetElement(static_cast<IGsize>(cellId), values.data());
            }
            dst->SetElement(static_cast<IGsize>(j), values.data());
        }

        // 复制到采样点上，因此附着类型为 IG_POINT
        outSet->AddAttribute(attr.type, IG_POINT, dst, attr.GetDataRange());
    }
}

/* ------------------------------------------------------------------ */
/* 折线输出                                                            */
/* ------------------------------------------------------------------ */
/** 复制一份属性条目（数组共享，属性条目独立），避免共享 AttributeSet 时宿主对象混乱 */
namespace {
AttributeSet::Pointer CloneAttributeSetEntries(const AttributeSet::Pointer& srcSet) {
    auto dstSet = AttributeSet::New();
    if (srcSet == nullptr) { return dstSet; }

    auto all = srcSet->GetAllAttributes();
    if (all == nullptr) { return dstSet; }

    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        dstSet->AddAttribute(attr.type, attr.attachmentType, attr.pointer, attr.dataRange);
    }
    return dstSet;
}
} // namespace

void ResampleToLine::BuildPolyLineOutputs(const Points::Pointer& samples, AttributeSet::Pointer attrSet,
                                          int sampleNum) {
    // (1) UnstructuredMesh 折线：IG_LINE 单元，保持与既有流程（菜单/示例）兼容
    auto lineMesh = UnstructuredMesh::New();
    lineMesh->SetName("resample_to_line");
    lineMesh->SetPoints(samples);

    auto cells = CellArray::New();
    auto types = UnsignedIntArray::New();
    for (int i = 0; i + 1 < sampleNum; ++i) {
        cells->AddCellId2(i, i + 1);
        types->AddValue(IG_LINE);
    }
    lineMesh->SetCells(cells, types);
    lineMesh->SetAttributeSet(CloneAttributeSetEntries(attrSet));
    lineMesh->SetViewStyle(IG_WIREFRAME);

    m_LineMesh = lineMesh;
    SetOutput(0, lineMesh);

    // (2) SurfaceMesh 折线：真正的折线数据（点 + 边），可直接渲染为折线
    auto poly = SurfaceMesh::New();
    poly->SetName("resample_to_line");
    poly->SetPoints(samples);

    // 先给出空的面片数组：SurfaceMesh 的拓扑/可绘制数据接口要求面片数组非空，
    // 且其时间戳要早于边数组，避免后续 RequestEditStatus() 依据面片重建边而丢掉折线
    poly->SetFaces(CellArray::New());

    auto edges = CellArray::New();
    for (int i = 0; i + 1 < sampleNum; ++i) { edges->AddCellId2(i, i + 1); }
    poly->SetEdges(edges);
    poly->SetAttributeSet(attrSet);
    poly->SetViewStyle(IG_WIREFRAME);
    // 折线没有面片，关闭抽壳/简化渲染，避免空面片网格参与简化
    poly->SetShellRenderingOption(false);

    m_PolyLine = poly;
    SetOutput(1, poly);
}

IGAME_NAMESPACE_END
