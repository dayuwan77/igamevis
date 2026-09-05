// 角度周期复制过滤器回归自检（无 GUI、无外部模型依赖）。
// 覆盖：多单元类型、PointData/CellData 属性、>16 点大单元、
//       VolumeMesh（不得按面类型错建）、StructuredMesh 显式拓扑。
#include <Periodic/iGameAngularPeriodicFilter.h>

#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGamePoints.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace iGame;

namespace {

int g_checks = 0;
int g_failures = 0;

void Expect(bool ok, const char* what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  -> %s : %s\n", ok ? "PASS" : "FAIL", what);
}

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3(double _x = 0, double _y = 0, double _z = 0) : x(_x), y(_y), z(_z) {}
};

Vec3 Rodrigues(const Vec3& p, const Vec3& axis, const Vec3& origin, double angleRad) {
    const double cosA = std::cos(angleRad), sinA = std::sin(angleRad);
    const double vx = p.x - origin.x, vy = p.y - origin.y, vz = p.z - origin.z;
    const double cx = axis.y * vz - axis.z * vy;
    const double cy = axis.z * vx - axis.x * vz;
    const double cz = axis.x * vy - axis.y * vx;
    const double dot = axis.x * vx + axis.y * vy + axis.z * vz;
    const double k = dot * (1.0 - cosA);
    return Vec3(origin.x + vx * cosA + cx * sinA + axis.x * k,
                origin.y + vy * cosA + cy * sinA + axis.y * k,
                origin.z + vz * cosA + cz * sinA + axis.z * k);
}

void CheckPointOnCopy(const Points* pts,
                      IGsize srcId, int copy, IGsize srcCount,
                      const Vec3& axisIn, const Vec3& origin, double angleStepRad) {
    double len = std::sqrt(axisIn.x * axisIn.x + axisIn.y * axisIn.y + axisIn.z * axisIn.z);
    const Vec3 axis = len > 1e-12 ? Vec3(axisIn.x / len, axisIn.y / len, axisIn.z / len) : axisIn;
    const auto& q0 = pts->GetPoint(srcId);
    const auto& qr = pts->GetPoint(srcId + copy * srcCount);
    const Vec3 p(q0[0], q0[1], q0[2]);
    const Vec3 q = Rodrigues(p, axis, origin, angleStepRad * copy);
    const double dx = qr[0] - q.x, dy = qr[1] - q.y, dz = qr[2] - q.z;
    Expect(dx * dx + dy * dy + dz * dz < 1e-5, "rotated coordinates match Rodrigues formula");
}

// ---------------- 输入网格构建 ----------------

FloatArray::Pointer FillPointAttr(const char* name, int dim, const std::vector<float>& values) {
    auto arr = FloatArray::New();
    arr->SetName(name);
    arr->SetDimension(dim);
    arr->Resize(values.size() / dim);
    for (size_t i = 0; i < values.size() / dim; ++i) {
        float* dst = arr->RawPointer(i);
        for (int d = 0; d < dim; ++d) { dst[d] = values[i * dim + d]; }
    }
    return arr;
}

UnstructuredMesh::Pointer MakeUnstructuredMixed() {
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 12; ++i) {
        pts->AddPoint(Point(float(i % 3), float((i / 3) % 2), float(i / 6)));
    }
    struct C { IGenum type; std::vector<igIndex> ids; };
    std::vector<C> cs;
    cs.push_back({IG_TETRA, {0, 1, 2, 3}});
    cs.push_back({IG_HEXAHEDRON, {4, 5, 6, 7, 8, 9, 10, 11}});
    cs.push_back({IG_PYRAMID, {0, 1, 2, 3, 4}});
    cs.push_back({IG_PRISM, {1, 2, 3, 4, 5, 6}});
    for (const C& c : cs) {
        mesh->AddCell(const_cast<igIndex*>(c.ids.data()), static_cast<int>(c.ids.size()), c.type);
    }

    auto attributes = AttributeSet::New();
    std::vector<float> ps;
    for (int i = 0; i < 12; ++i) { ps.push_back(float(i)); }
    attributes->AddAttribute(IG_SCALAR, IG_POINT, FillPointAttr("P", 1, ps));
    std::vector<float> pv;
    for (int i = 0; i < 12; ++i) { pv.push_back(float(i)); pv.push_back(float(i * 2)); pv.push_back(float(-i)); }
    attributes->AddAttribute(IG_VECTOR, IG_POINT, FillPointAttr("PV", 3, pv));
    std::vector<float> csd;
    for (size_t i = 0; i < cs.size(); ++i) { csd.push_back(float(i * 10)); }
    attributes->AddAttribute(IG_SCALAR, IG_CELL, FillPointAttr("C", 1, csd));
    std::vector<float> cvd;
    for (size_t i = 0; i < cs.size(); ++i) { cvd.push_back(float(i)); cvd.push_back(float(i + 1)); cvd.push_back(float(i + 2)); }
    attributes->AddAttribute(IG_VECTOR, IG_CELL, FillPointAttr("CV", 3, cvd));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

UnstructuredMesh::Pointer MakeUnstructuredWithBigPoly() {
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    const int bigN = 40; // >16 点的大单元
    for (int i = 0; i < bigN + 4; ++i) { pts->AddPoint(Point(float(i), 0.f, 0.f)); }
    std::vector<igIndex> big(bigN);
    for (int i = 0; i < bigN; ++i) { big[i] = igIndex(4 + i); }
    mesh->AddCell(big.data(), bigN, IG_POLYGON);
    igIndex quad[4] = {0, 1, 2, 3};
    mesh->AddCell(quad, 4, IG_QUAD);
    return mesh;
}

SurfaceMesh::Pointer MakeSurfaceMixed() {
    auto mesh = SurfaceMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 8; ++i) { pts->AddPoint(Point(float(i), 0.f, 0.f)); }
    auto faces = CellArray::New();
    igIndex tri[3] = {0, 1, 2};
    igIndex quad[4] = {1, 2, 3, 4};
    std::vector<igIndex> ngon(8);
    for (int i = 0; i < 8; ++i) { ngon[i] = igIndex(i); }
    faces->AddCellIds(tri, 3);
    faces->AddCellIds(quad, 4);
    faces->AddCellIds(ngon.data(), 8);
    mesh->SetFaces(faces);
    return mesh;
}

VolumeMesh::Pointer MakeVolumeTetHex() {
    auto mesh = VolumeMesh::New();
    auto pts = Points::New();
    for (int i = 0; i < 12; ++i) {
        pts->AddPoint(Point(float(i % 3), float((i / 3) % 2), float(i / 6)));
    }
    mesh->SetPoints(pts);
    auto vols = CellArray::New();
    igIndex tet[4] = {0, 1, 2, 3};
    igIndex hex[8] = {4, 5, 6, 7, 8, 9, 10, 11};
    vols->AddCellIds(tet, 4);
    vols->AddCellIds(hex, 8);
    mesh->SetVolumes(vols);

    auto attributes = AttributeSet::New();
    std::vector<float> pd;
    for (int i = 0; i < 12; ++i) { pd.push_back(float(i)); }
    attributes->AddAttribute(IG_SCALAR, IG_POINT, FillPointAttr("P", 1, pd));
    std::vector<float> cd = {5.f, 9.f};
    attributes->AddAttribute(IG_SCALAR, IG_CELL, FillPointAttr("C", 1, cd));
    mesh->SetAttributeSet(attributes);
    return mesh;
}

StructuredMesh::Pointer MakeStructured3D() {
    auto mesh = StructuredMesh::New();
    auto pts = Points::New();
    const igIndex nx = 2, ny = 2, nz = 2;
    for (igIndex k = 0; k < nz; ++k)
        for (igIndex j = 0; j < ny; ++j)
            for (igIndex i = 0; i < nx; ++i) { pts->AddPoint(Point(float(i), float(j), float(k))); }
    mesh->SetPoints(pts);
    igIndex dims[3] = {nx, ny, nz};
    mesh->SetDimensionSize(dims);
    return mesh;
}

StructuredMesh::Pointer MakeStructured2D() {
    auto mesh = StructuredMesh::New();
    auto pts = Points::New();
    const igIndex nx = 2, ny = 2;
    for (igIndex j = 0; j < ny; ++j)
        for (igIndex i = 0; i < nx; ++i) { pts->AddPoint(Point(float(i), float(j), 0.f)); }
    mesh->SetPoints(pts);
    igIndex dims[3] = {nx, ny, 1};
    mesh->SetDimensionSize(dims);
    return mesh;
}

// ---------------- 运行与校验 ----------------

struct RunResult {
    UnstructuredMesh::Pointer out;
    bool ok = false;
};

RunResult RunFilter(PointSet::Pointer src, const Vec3& axis, const Vec3& origin,
                    int copies, float angleDeg) {
    RunResult r;
    auto filter = AngularPeriodicFilter::New();
    filter->SetInput(src);
    filter->SetRotationAxis(Point(float(origin.x), float(origin.y), float(origin.z)),
                            Vector3d(axis.x, axis.y, axis.z));
    filter->SetNumberOfCopies(copies);
    filter->SetAngle(angleDeg);
    if (!filter->Execute()) {
        std::printf("  filter Execute failed: %s\n", filter->GetMessage().c_str());
        return r;
    }
    r.out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
    r.ok = r.out != nullptr;
    return r;
}

IGsize GetCellSize(UnstructuredMesh* out, IGsize idx) {
    const igIndex* ids = nullptr;
    return out->GetCells()->GetCellIds(idx, ids);
}

IGenum GetCellTypeAt(UnstructuredMesh* out, IGsize idx) {
    auto types = out->GetCellTypes();
    return static_cast<IGenum>(types->GetValue(idx));
}

bool CheckCellCopies(UnstructuredMesh* out,
                     const std::vector<std::vector<igIndex>>& srcCells,
                     IGsize srcPointCount, int copies) {
    auto cellArray = out->GetCells();
    auto types = out->GetCellTypes();
    if (!cellArray || !types) return false;
    if (cellArray->GetNumberOfCells() != srcCells.size() * static_cast<size_t>(copies)) return false;
    for (int c = 0; c < copies; ++c) {
        for (size_t k = 0; k < srcCells.size(); ++k) {
            const IGsize idx = c * static_cast<IGsize>(srcCells.size()) + k;
            const igIndex* ids = nullptr;
            const int n = cellArray->GetCellIds(idx, ids);
            if (static_cast<size_t>(n) != srcCells[k].size()) return false;
            for (int j = 0; j < n; ++j) {
                if (ids[j] != srcCells[k][j] + c * srcPointCount) return false;
            }
        }
    }
    return true;
}

// ---------------- 用例 ----------------

void TestUnstructuredMixed() {
    std::printf("[case] unstructured mixed cell types + attributes\n");
    auto src = MakeUnstructuredMixed();
    const int copies = 3;
    const Vec3 axis(0, 0, 1), origin(0, 0, 0);
    auto r = RunFilter(src, axis, origin, copies, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    const IGsize srcNpts = 12, srcNc = 4;
    Expect(r.out->GetNumberOfPoints() == srcNpts * copies, "point count = src*copies");
    Expect(r.out->GetCells()->GetNumberOfCells() == srcNc * copies, "cell count = src*copies");

    const IGenum expect[4] = {IG_TETRA, IG_HEXAHEDRON, IG_PYRAMID, IG_PRISM};
    bool typeOk = true;
    for (int c = 0; c < copies; ++c)
        for (IGsize k = 0; k < srcNc; ++k)
            if (GetCellTypeAt(r.out.get(), c * srcNc + k) != expect[k]) typeOk = false;
    Expect(typeOk, "per-copy cell types preserved (no face-type rebuild)");

    std::vector<std::vector<igIndex>> cells;
    auto cellArray = src->GetCells();
    for (IGsize k = 0; k < srcNc; ++k) {
        const igIndex* ids = nullptr;
        int n = cellArray->GetCellIds(k, ids);
        cells.emplace_back(ids, ids + n);
    }
    Expect(CheckCellCopies(r.out.get(), cells, srcNpts, copies), "cell id offsets per copy correct");

    auto outAttrs = r.out->GetAttributeSet();
    auto ps = DynamicCast<FloatArray>(outAttrs->GetAttribute("P").pointer);
    auto pv = DynamicCast<FloatArray>(outAttrs->GetAttribute("PV").pointer);
    auto csd = DynamicCast<FloatArray>(outAttrs->GetAttribute("C").pointer);
    auto cvd = DynamicCast<FloatArray>(outAttrs->GetAttribute("CV").pointer);
    Expect(ps && pv && csd && cvd, "all point/cell attributes present");
    if (!(ps && pv && csd && cvd)) return;

    bool attrOk = true;
    for (int c = 0; c < copies; ++c) {
        for (IGsize i = 0; i < srcNpts; ++i) {
            const int ival = static_cast<int>(i);
            const float* pp = ps->RawPointer(c * srcNpts + i);
            const float* pvp = pv->RawPointer(c * srcNpts + i);
            if (pp[0] != float(ival)) attrOk = false;
            if (pvp[0] != float(ival) || pvp[1] != float(ival * 2) || pvp[2] != float(-ival)) {
                attrOk = false;
            }
        }
        for (IGsize k = 0; k < srcNc; ++k) {
            const int kval = static_cast<int>(k);
            const float* cpp = csd->RawPointer(c * srcNc + k);
            const float* cvp = cvd->RawPointer(c * srcNc + k);
            if (cpp[0] != float(kval * 10)) attrOk = false;
            if (cvp[0] != float(kval) || cvp[1] != float(kval + 1) || cvp[2] != float(kval + 2)) {
                attrOk = false;
            }
        }
    }
    Expect(attrOk, "point/cell attribute values replicated identically per copy");

    const double step = 360.0 / copies * 3.141592653589793 / 180.0;
    for (int c = 0; c < copies; ++c) {
        CheckPointOnCopy(r.out->GetPoints(), 0, c, srcNpts, axis, origin, step);
        CheckPointOnCopy(r.out->GetPoints(), 11, c, srcNpts, axis, origin, step);
    }
}

void TestUnstructuredBigPoly() {
    std::printf("[case] unstructured with >16-point polygon cell\n");
    auto src = MakeUnstructuredWithBigPoly();
    const int copies = 2;
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), copies, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 4, "2 cells x 2 copies (none dropped)");
    Expect(GetCellSize(r.out.get(), 0) == 40, "big polygon cell preserved (40 pts)");
    Expect(GetCellSize(r.out.get(), 2) == 40, "big polygon preserved in 2nd copy");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_POLYGON, "big polygon type = IG_POLYGON");

    std::vector<std::vector<igIndex>> cells;
    auto cellArray = src->GetCells();
    for (IGsize k = 0; k < cellArray->GetNumberOfCells(); ++k) {
        const igIndex* ids = nullptr;
        int n = cellArray->GetCellIds(k, ids);
        cells.emplace_back(ids, ids + n);
    }
    Expect(CheckCellCopies(r.out.get(), cells, src->GetNumberOfPoints(), copies),
           "cell id offsets of big cell correct");
}

void TestSurfaceMixed() {
    std::printf("[case] surface faces -> correct mapped types\n");
    auto src = MakeSurfaceMixed();
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 6, "3 faces x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_TRIANGLE, "triangle");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_QUAD, "quad");
    Expect(GetCellTypeAt(r.out.get(), 2) == IG_POLYGON, "8-gon -> IG_POLYGON");
}

void TestVolumeMesh() {
    std::printf("[case] VolumeMesh must NOT be rebuilt by face point count\n");
    auto src = MakeVolumeTetHex();
    auto r = RunFilter(src, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 4, "2 volumes x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_TETRA, "4-pt volume is TETRA (not QUAD)");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_HEXAHEDRON, "8-pt volume is HEXAHEDRON");
    Expect(GetCellTypeAt(r.out.get(), 2) == IG_TETRA, "2nd copy TETRA");

    auto outAttrs = r.out->GetAttributeSet();
    auto ps = DynamicCast<FloatArray>(outAttrs->GetAttribute("P").pointer);
    auto ca = DynamicCast<FloatArray>(outAttrs->GetAttribute("C").pointer);
    Expect(ps != nullptr && ca != nullptr, "volume point/cell attributes present");
    Expect(ps && ps->RawPointer(0)[0] == 0.f && ps->RawPointer(12)[0] == 0.f,
           "point attr length = npts*copies");
    Expect(ca && ca->RawPointer(0)[0] == 5.f && ca->RawPointer(2)[0] == 5.f,
           "cell attr values replicated per copy");
}

void TestStructured() {
    std::printf("[case] StructuredMesh 3D (single hexa) + 2D (quad)\n");
    auto s3 = MakeStructured3D();
    auto r3 = RunFilter(s3, Vec3(1, 0, 0), Vec3(0, 0, 0), 2, 360.f);
    Expect(r3.ok, "structured 3D Execute returns true");
    if (r3.ok) {
        Expect(r3.out->GetCells()->GetNumberOfCells() == 2, "1 hexa x 2 copies");
        Expect(GetCellTypeAt(r3.out.get(), 0) == IG_HEXAHEDRON, "structured 3D cell = HEXAHEDRON");
    }

    auto s2 = MakeStructured2D();
    auto r2 = RunFilter(s2, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 360.f);
    Expect(r2.ok, "structured 2D Execute returns true");
    if (r2.ok) {
        Expect(r2.out->GetCells()->GetNumberOfCells() == 2, "1 quad x 2 copies");
        Expect(GetCellTypeAt(r2.out.get(), 0) == IG_QUAD, "structured 2D cell = QUAD");
    }
}

void TestPolyhedronCell() {
    std::printf("[case] unstructured IG_POLYHEDRON encoded cell (face/vertex offset split)\n");
    // 手工构造多面体：编码 = [faceCount, n0, 顶点..., n1, 顶点...]
    auto mesh = UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    for (int i = 0; i < 10; ++i) { pts->AddPoint(Point(float(i), float(i * i % 3), 0.f)); }
    std::vector<igIndex> poly = {2,                     // 2 个面
                                 3, 0, 1, 2,            // 面 0：三角形 0-1-2
                                 4, 3, 4, 5, 6};        // 面 1：四边形 3-4-5-6
    mesh->AddCell(poly.data(), static_cast<int>(poly.size()), IG_POLYHEDRON);

    auto r = RunFilter(mesh, Vec3(0, 0, 1), Vec3(0, 0, 0), 2, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;

    Expect(r.out->GetCells()->GetNumberOfCells() == 2, "1 polyhedron x 2 copies");
    Expect(GetCellTypeAt(r.out.get(), 0) == IG_POLYHEDRON, "copy0 type = IG_POLYHEDRON");
    Expect(GetCellTypeAt(r.out.get(), 1) == IG_POLYHEDRON, "copy1 type = IG_POLYHEDRON");

    const igIndex* ids0 = nullptr;
    const igIndex* ids1 = nullptr;
    r.out->GetCells()->GetCellIds(0, ids0);
    r.out->GetCells()->GetCellIds(1, ids1);

    // copy0 应与输入编码一致；copy1 仅顶点号 +10，faceCount/每面顶点数不动
    const igIndex exp0[10] = {2, 3, 0, 1, 2, 4, 3, 4, 5, 6};
    const igIndex exp1[10] = {2, 3, 10, 11, 12, 4, 13, 14, 15, 16};
    bool ok0 = true, ok1 = true;
    for (int i = 0; i < 10; ++i) {
        if (ids0[i] != exp0[i]) ok0 = false;
        if (ids1[i] != exp1[i]) ok1 = false;
    }
    Expect(ok0, "copy0 polyhedron encoding unchanged");
    Expect(ok1, "copy1 polyhedron vertex ids offset, face counts preserved");
}

void TestNonAlignedAxis() {
    std::printf("[case] non-axis-aligned rotation axis\n");
    auto src = MakeUnstructuredMixed();
    auto r = RunFilter(src, Vec3(1, 1, 1), Vec3(0.2, -0.1, 0.4), 4, 360.f);
    Expect(r.ok, "Execute returns true");
    if (!r.ok) return;
    const double step = 360.0 / 4 * 3.141592653589793 / 180.0;
    for (int c = 0; c < 4; ++c) {
        CheckPointOnCopy(r.out->GetPoints(), 0, c, 12, Vec3(1, 1, 1), Vec3(0.2, -0.1, 0.4), step);
    }
}

} // namespace

int main() {
    TestUnstructuredMixed();
    TestUnstructuredBigPoly();
    TestSurfaceMixed();
    TestVolumeMesh();
    TestStructured();
    TestPolyhedronCell();
    TestNonAlignedAxis();

    std::printf("total checks: %d, failures: %d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
