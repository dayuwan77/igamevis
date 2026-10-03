#include <TensorPrincipalInvariants/iGameTensorPrincipalInvariantsFilter.h>

#include <Core/iGameScene.h>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iGameAttributeSet.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameUnstructuredMesh.h>
#include <iostream>
#include <string>
#include <vector>

// 中等任务配套测试用例：对称张量的主值（特征值）与主方向（特征向量）
// 运行：cd Examples && ./testTensorPrincipalInvariants
//
// 测试模型：TensorPrincipalInvariants_symmetric.vtk（4 点 / 1 四边形）
//   Point Data 'stress'（6 分量对称张量，顺序 XX,YY,ZZ,XY,YZ,XZ）：
//     tuple0 = diag(3,2,1)                     -> 主值 3, 2, 1
//     tuple1 = [[2,1,0],[1,2,0],[0,0,5]]       -> 主值 5, 3, 1
//     tuple2 = diag(2,2,2)（各向同性）          -> 主值 2, 2, 2
//     tuple3 = XX=10, YY=-2, ZZ=-8, XY=4       -> trace = 0，用不变量交叉验证
//   另有 'vec2'（2 分量）与 'vec3'（3 分量、类型 IG_SCALAR）用于验证"非张量数组被拒绝"；
//   Cell Data 'cstress'（IG_TENSOR，6 分量）用于验证单元数据的处理。
//
// 断言重点：
//   - 输出 6 个数组，命名逐字一致：'<名> - Sigma N' / '<名> - Sigma N (Vector)'，且写入顺序 3 向量 + 3 标量；
//   - 主值按从大到小排序；Σ 主值 == trace(T)；Π 主值 == det(T)（不变量交叉验证）；
//   - 主方向满足 T·v = λ·v（残差 < 1e-9）；默认单位长度，ScaleVectors 打开时长度 == |λ|。

namespace {

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    std::cerr << (ok ? "  [ ok ] " : " [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

iGame::UnstructuredMesh::Pointer LoadMesh(const std::string& fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::cerr << " [FAIL] model not found: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << " [FAIL] ReadFile returned null: " << fileName << "\n";
        ++g_failed;
        return nullptr;
    }
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh == nullptr) {
        std::cerr << " [FAIL] not an UnstructuredMesh: " << fileName << "\n";
        ++g_failed;
    }
    return mesh;
}

iGame::ArrayObject::Pointer FindArray(iGame::DataObject::Pointer obj, const std::string& name,
                                      IGenum attachment) {
    if (obj == nullptr) { return nullptr; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return nullptr; }
    auto all = attrs->GetAllAttributes();
    if (all == nullptr) { return nullptr; }
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (std::string(attr.pointer->GetName()) == name) { return attr.pointer; }
    }
    return nullptr;
}

int CountArrays(iGame::DataObject::Pointer obj, const std::string& name, IGenum attachment) {
    if (obj == nullptr) { return 0; }
    auto attrs = obj->GetAttributeSet();
    if (attrs == nullptr) { return 0; }
    auto all = attrs->GetAllAttributes();
    int n = 0;
    for (int i = 0; i < static_cast<int>(all->GetNumberOfElements()); ++i) {
        auto& attr = all->GetElement(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (attr.attachmentType != attachment) { continue; }
        if (std::string(attr.pointer->GetName()) == name) { ++n; }
    }
    return n;
}

double Comp(iGame::ArrayObject::Pointer array, IGsize i, int comp) {
    if (array == nullptr) { return -999.0; }
    return array->GetElementValue(i, comp);
}

bool Near(double a, double b, double tol = 1e-6) { return std::fabs(a - b) <= tol; }

/// 从 6 分量数组按固定分量顺序装配对称张量：XX, YY, ZZ, XY, YZ, XZ
void BuildTensor(iGame::ArrayObject::Pointer array, IGsize i, double m[3][3]) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { m[r][c] = 0.0; }
    }
    m[0][0] = Comp(array, i, 0);
    m[1][1] = Comp(array, i, 1);
    m[2][2] = Comp(array, i, 2);
    m[0][1] = m[1][0] = Comp(array, i, 3);
    m[1][2] = m[2][1] = Comp(array, i, 4);
    m[0][2] = m[2][0] = Comp(array, i, 5);
}

double Trace(const double m[3][3]) { return m[0][0] + m[1][1] + m[2][2]; }

double Det(const double m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

/// 张量 3x3 数组名（`<原名> - Sigma N` / `<原名> - Sigma N (Vector)`）
std::string ValName(const std::string& base, int i) { return base + " - Sigma " + std::to_string(i); }
std::string VecName(const std::string& base, int i) {
    return base + " - Sigma " + std::to_string(i) + " (Vector)";
}

/// 场景 1：点数据张量，逐元核对主值/主方向/不变量
void TestPointTensor() {
    std::cerr << "[case 1] point tensor: principal values, vectors and invariants\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }
    Check(mesh->GetNumberOfPoints() == 4 && mesh->GetNumberOfCells() == 1,
          "input: 4 points / 1 quad");

    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("stress");
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true (message: " + filter->GetMessage() + ")");

    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    Check(out != nullptr, "GetOutput() is an UnstructuredMesh");
    if (out == nullptr) { return; }
    Check(out.GetPointer() != mesh.GetPointer(), "output is independent from the input");
    Check(out->GetNumberOfPoints() == 4 && out->GetNumberOfCells() == 1,
          "geometry/topology preserved");

    // 6 个输出数组，命名固定
    std::vector<iGame::ArrayObject::Pointer> vecs, vals;
    for (int i = 1; i <= 3; ++i) {
        vecs.push_back(FindArray(out, VecName("stress", i), IG_POINT));
        vals.push_back(FindArray(out, ValName("stress", i), IG_POINT));
    }
    bool allExist = true;
    for (int i = 0; i < 3; ++i) {
        if (vecs[i] == nullptr || vals[i] == nullptr) { allExist = false; }
    }
    Check(allExist, "all 6 output arrays exist with the expected names");
    if (!allExist) { return; }
    Check(vecs[0]->GetDimension() == 3 && vecs[1]->GetDimension() == 3 &&
              vecs[2]->GetDimension() == 3,
          "principal vectors have 3 components");
    Check(vals[0]->GetDimension() == 1, "principal values are scalars");

    // 输入原有数组必须保留（含被拒绝的 vec2 / vec3）
    Check(FindArray(out, "stress", IG_POINT) != nullptr, "input tensor array is preserved");
    Check(FindArray(out, "vec2", IG_POINT) != nullptr, "unrelated point array 'vec2' preserved");
    Check(FindArray(out, "vec3", IG_POINT) != nullptr, "unrelated point array 'vec3' preserved");
    Check(FindArray(mesh, ValName("stress", 1), IG_POINT) == nullptr,
          "input mesh is NOT modified (no result arrays on it)");

    auto in = FindArray(mesh, "stress", IG_POINT);

    // 手算期望值：tuple0 = 3,2,1；tuple1 = 5,3,1；tuple2 = 2,2,2
    const double expect[3][3] = {{3.0, 2.0, 1.0}, {5.0, 3.0, 1.0}, {2.0, 2.0, 2.0}};
    for (int t = 0; t < 3; ++t) {
        bool ok = true;
        for (int k = 0; k < 3; ++k) {
            if (!Near(Comp(vals[k], t, 0), expect[t][k], 1e-9)) { ok = false; }
        }
        Check(ok, "tuple " + std::to_string(t) + ": principal values == " +
                      std::to_string(expect[t][0]) + ", " + std::to_string(expect[t][1]) + ", " +
                      std::to_string(expect[t][2]));
    }

    // 排序 + 不变量 + 残差 校验（4 个元组）
    bool sortedOK = true;
    bool traceOK = true;
    bool detOK = true;
    bool residualOK = true;
    bool unitOK = true;
    for (IGsize t = 0; t < 4; ++t) {
        const double l1 = Comp(vals[0], t, 0);
        const double l2 = Comp(vals[1], t, 0);
        const double l3 = Comp(vals[2], t, 0);
        if (!(l1 >= l2 && l2 >= l3)) { sortedOK = false; }

        double m[3][3];
        BuildTensor(in, t, m);
        if (!Near(l1 + l2 + l3, Trace(m), 1e-9)) { traceOK = false; }
        if (!Near(l1 * l2 * l3, Det(m), 1e-9)) { detOK = false; }

        for (int k = 0; k < 3; ++k) {
            const double vx = Comp(vecs[k], t, 0);
            const double vy = Comp(vecs[k], t, 1);
            const double vz = Comp(vecs[k], t, 2);
            const double len = std::sqrt(vx * vx + vy * vy + vz * vz);
            if (!Near(len, 1.0, 1e-9)) { unitOK = false; }
            // T·v 应当等于 λ·v
            double tv[3];
            for (int r = 0; r < 3; ++r) {
                tv[r] = m[r][0] * vx + m[r][1] * vy + m[r][2] * vz;
            }
            const double lam = Comp(vals[k], t, 0);
            if (std::fabs(tv[0] - lam * vx) > 1e-9 || std::fabs(tv[1] - lam * vy) > 1e-9 ||
                std::fabs(tv[2] - lam * vz) > 1e-9) {
                residualOK = false;
            }
        }
    }
    Check(sortedOK, "principal values are sorted from largest to smallest");
    Check(traceOK, "Sigma1+Sigma2+Sigma3 == trace(tensor) for every tuple");
    Check(detOK, "Sigma1*Sigma2*Sigma3 == det(tensor) for every tuple");
    Check(unitOK, "principal vectors are unit length (ScaleVectors off)");
    Check(residualOK, "T * v == lambda * v for every principal direction (residual < 1e-9)");

    // 写入顺序固定：3 个向量在前、3 个标量在后
    auto attrs = out->GetAttributeSet();
    int idxVec = attrs->GetAttributeIndex(VecName("stress", 1));
    int idxVal = attrs->GetAttributeIndex(ValName("stress", 1));
    Check(idxVec >= 0 && idxVal >= 0 && idxVec < idxVal,
          "vectors are added before values (fixed order)");
}

/// 场景 2：ScaleVectors 打开后主方向按主值缩放
void TestScaleVectors() {
    std::cerr << "[case 2] ScaleVectors: vector length == |principal value|\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("stress");
    filter->SetScaleVectors(true);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }

    bool ok = true;
    for (int k = 1; k <= 3; ++k) {
        auto v = FindArray(out, VecName("stress", k), IG_POINT);
        auto s = FindArray(out, ValName("stress", k), IG_POINT);
        if (v == nullptr || s == nullptr) {
            ok = false;
            break;
        }
        for (IGsize t = 0; t < 4; ++t) {
            const double vx = Comp(v, t, 0);
            const double vy = Comp(v, t, 1);
            const double vz = Comp(v, t, 2);
            const double len = std::sqrt(vx * vx + vy * vy + vz * vz);
            if (!Near(len, std::fabs(Comp(s, t, 0)), 1e-9)) { ok = false; }
        }
    }
    Check(ok, "|vector| == |value| for every tuple (ScaleVectors on)");
}

/// 场景 3：单元数据上的张量（IG_TENSOR）
void TestCellTensor() {
    std::cerr << "[case 3] cell tensor (IG_TENSOR on cell data)\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("cstress");
    filter->SetArrayAttachment(IG_CELL);
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true (message: " + filter->GetMessage() + ")");
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    auto v1 = FindArray(out, VecName("cstress", 1), IG_CELL);
    auto s1 = FindArray(out, ValName("cstress", 1), IG_CELL);
    auto s2 = FindArray(out, ValName("cstress", 2), IG_CELL);
    auto s3 = FindArray(out, ValName("cstress", 3), IG_CELL);
    Check(v1 != nullptr && s1 != nullptr && s2 != nullptr && s3 != nullptr,
          "6 cell arrays are created on cell data");
    Check(s1 != nullptr && Near(Comp(s1, 0, 0), 1.0, 1e-9) && Near(Comp(s2, 0, 0), 0.0, 1e-9) &&
              Near(Comp(s3, 0, 0), 0.0, 1e-9),
          "diag(1,0,0) -> principal values 1, 0, 0");
    Check(FindArray(mesh, ValName("cstress", 1), IG_CELL) == nullptr,
          "input mesh is NOT modified");
}

/// 场景 4：张量分量含 NaN 时输出 NaN，且写入执行信息不静默
void TestNaNTensor() {
    std::cerr << "[case 4] NaN tensor component -> NaN outputs + message\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }
    auto stress = FindArray(mesh, "stress", IG_POINT);
    if (stress == nullptr) {
        Check(false, "input tensor array exists");
        return;
    }
    // 把 tuple3 的第一个分量改成 NaN（元素 3 的第 0 个分量 → 扁平下标 3*6 + 0）
    stress->SetValue(3 * 6 + 0, std::nan(""));

    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("stress");
    filter->SetInput(mesh);
    Check(filter->Execute(), "Execute() returns true");
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }
    auto s1 = FindArray(out, ValName("stress", 1), IG_POINT);
    Check(s1 != nullptr && std::isnan(Comp(s1, 3, 0)),
          "NaN tuple produces NaN result (not silently zero)");
    Check(!std::isnan(Comp(s1, 0, 0)), "other tuples are still computed normally");
    Check(filter->GetMessage().find("NaN") != std::string::npos,
          "execution message reports the NaN tuples: " + filter->GetMessage());
}

/// 场景 5：不能作为张量的输入要明确失败
void TestFailureCases() {
    std::cerr << "[case 5] failure reporting for invalid tensor inputs\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }

    // 2 分量数组
    {
        auto f = iGame::TensorPrincipalInvariantsFilter::New();
        f->SetTensorArrayName("vec2");
        f->SetInput(mesh);
        Check(!f->Execute(), "2-component array -> Execute() returns false");
        Check(f->GetMessage().find("components") != std::string::npos,
              "message explains the component requirement: " + f->GetMessage());
    }
    // 3 分量但不是 IG_TENSOR（普通三维向量不能被当成 2D 张量）
    {
        auto f = iGame::TensorPrincipalInvariantsFilter::New();
        f->SetTensorArrayName("vec3");
        f->SetInput(mesh);
        Check(!f->Execute(), "3-component non-IG_TENSOR array -> Execute() returns false");
        Check(f->GetMessage().find("IG_TENSOR") != std::string::npos,
              "message points out the IG_TENSOR requirement: " + f->GetMessage());
    }
    // 不存在的数组名
    {
        auto f = iGame::TensorPrincipalInvariantsFilter::New();
        f->SetTensorArrayName("no_such_array");
        f->SetInput(mesh);
        Check(!f->Execute(), "unknown array name -> Execute() returns false");
        Check(f->GetMessage().find("not found") != std::string::npos,
              "message says the array was not found: " + f->GetMessage());
    }
    // 空网格
    {
        auto empty = iGame::UnstructuredMesh::New();
        auto f = iGame::TensorPrincipalInvariantsFilter::New();
        f->SetInput(empty);
        Check(!f->Execute(), "mesh without data -> Execute() returns false");
        Check(!f->GetMessage().empty(), "message is provided: " + f->GetMessage());
    }
}

/// 场景 6：重复执行不堆积结果数组
void TestRepeatedExecution() {
    std::cerr << "[case 6] repeated execution keeps exactly 6 result arrays\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }
    auto f = iGame::TensorPrincipalInvariantsFilter::New();
    f->SetTensorArrayName("stress");
    f->SetInput(mesh);
    Check(f->Execute(), "first Execute() returns true");
    auto out1 = iGame::DynamicCast<iGame::UnstructuredMesh>(f->GetOutput());
    int total1 = 0;
    if (out1 != nullptr) {
        for (int k = 1; k <= 3; ++k) {
            total1 += CountArrays(out1, ValName("stress", k), IG_POINT);
            total1 += CountArrays(out1, VecName("stress", k), IG_POINT);
        }
    }
    Check(total1 == 6, "6 result arrays after the first run (got " + std::to_string(total1) + ")");

    Check(f->Execute(), "second Execute() returns true");
    auto out2 = iGame::DynamicCast<iGame::UnstructuredMesh>(f->GetOutput());
    int total2 = 0;
    if (out2 != nullptr) {
        for (int k = 1; k <= 3; ++k) {
            total2 += CountArrays(out2, ValName("stress", k), IG_POINT);
            total2 += CountArrays(out2, VecName("stress", k), IG_POINT);
        }
    }
    Check(total2 == 6, "still 6 result arrays after the second run (got " + std::to_string(total2) + ")");
}

/// 场景 7：主方向的符号惯例
/// 特征向量存在 v / -v 二义性，本实现把符号定死 —— 同一个输入永远给同一组主方向。
/// 期望值由规范化规则的参考实现给出（先按 |第一分量| 重排、令前两个方向的主对角元为正、
/// det<0 再翻第三个；各向同性给单位阵；两值相等时用叉乘重建），与下面的期望表逐位一致。
void TestVectorSignConvention() {
    std::cerr << "[case 7] principal-vector sign convention\n";
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_symmetric.vtk");
    if (mesh == nullptr) { return; }

    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("stress");
    filter->SetInput(mesh);
    if (!filter->Execute()) {
        Check(false, "Execute() returns true (message: " + filter->GetMessage() + ")");
        return;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) {
        Check(false, "GetOutput() is an UnstructuredMesh");
        return;
    }

    std::vector<iGame::ArrayObject::Pointer> vecs;
    for (int i = 1; i <= 3; ++i) {
        vecs.push_back(FindArray(out, VecName("stress", i), IG_POINT));
        if (vecs.back() == nullptr) {
            Check(false, "principal vector array " + std::to_string(i));
            return;
        }
    }

    // tuple0 = diag(3,2,1)           → 主轴恰好是 x / y / z
    // tuple1 = [[2,1,0],[1,2,0],[0,0,5]]
    // tuple2 = diag(2,2,2) 各向同性   → 惯例给单位阵，按降序取 (z, y, x)
    // tuple3 = [[10,4,0],[4,-2,0],[0,0,-8]]
    const double expect[4][3][3] = {
        {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
        {{0.0, 0.0, 1.0}, {0.7071068, 0.7071068, 0.0}, {0.7071068, -0.7071068, 0.0}},
        {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}},
        {{0.9570920, 0.2897841, 0.0}, {-0.2897841, 0.9570920, 0.0}, {0.0, 0.0, 1.0}},
    };
    bool ok = true;
    for (int t = 0; t < 4; ++t) {
        for (int k = 0; k < 3; ++k) {
            for (int c = 0; c < 3; ++c) {
                if (!Near(Comp(vecs[k], t, c), expect[t][k][c], 1e-6)) { ok = false; }
            }
        }
    }
    Check(ok, "principal direction signs follow the fixed convention on all 4 tuples");
}

/// 可视化演示：按第一主值伪彩着色（便于录屏对照）
void VisualizeResult() {
    if (std::getenv("IGV_TEST_NO_VIEW") != nullptr) { return; }
    auto mesh = LoadMesh("./Models/TensorPrincipalInvariants_field.vtk");
    if (mesh == nullptr) { return; }
    auto filter = iGame::TensorPrincipalInvariantsFilter::New();
    filter->SetTensorArrayName("stress");
    filter->SetInput(mesh);
    if (!filter->Execute()) { return; }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (out == nullptr) { return; }

    auto scene = iGame::Scene::New();
    auto draw = iGame::DynamicCast<iGame::DrawObject>(out);
    if (draw != nullptr) {
        draw->SetViewStyle(IG_SURFACE);
        int idx = -1;
        if (out->GetAttributeSet() != nullptr) {
            idx = out->GetAttributeSet()->GetAttributeIndex(ValName("stress", 1));
        }
        if (idx >= 0) { draw->ViewCloudPicture(scene.GetPointer(), idx); }
    }
    scene->AddModel(out);

    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}

}  // namespace

int main() {
    std::cerr << "==== testTensorPrincipalInvariants ====\n";
    TestPointTensor();
    TestScaleVectors();
    TestCellTensor();
    TestNaNTensor();
    TestFailureCases();
    TestRepeatedExecution();
    TestVectorSignConvention();

    if (g_failed == 0) {
        std::cerr << "[testTensorPrincipalInvariants] PASS: all checks passed\n";
    } else {
        std::cerr << "[testTensorPrincipalInvariants] FAIL: " << g_failed << " check(s) failed\n";
    }

    VisualizeResult();
    return (g_failed == 0) ? 0 : 1;
}
