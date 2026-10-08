// YieldCriteria 过滤器测试：主应力 / Tresca / von Mises 的数值、输出形态与边界行为。
//
// 参考值由 out/pv_yield_probe.py 采样得到（6 个测试张量上的三种准则结果），
// 张量分量顺序为 xx, yy, zz, xy, yz, zx（与项目 RotateTensor6 的约定一致）。
#include <TensorView/iGameYieldCriteriaFilter.h>
#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>
#include <iGameType.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failed = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "[ok]   " : "[FAIL] ") << what << "\n";
    if (!ok) ++g_failed;
}

// 6 个测试张量（xx, yy, zz, xy, yz, zx）
const double kTensors[6][6] = {
        {100.0, 0.0, 0.0, 0.0, 0.0, 0.0},   // 单轴 σxx=100
        {0.0, 0.0, 0.0, 50.0, 0.0, 0.0},    // 纯剪切 σxy=50
        {40.0, 40.0, 40.0, 0.0, 0.0, 0.0},  // 三轴等拉
        {200.0, 100.0, 50.0, 0.0, 0.0, 0.0},// 主应力已知
        {120.0, -30.0, 10.0, 45.0, -20.0, 15.0}, // 一般应力状态
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},     // 零应力
};
const int kNumTensors = 6;

// 参考值（ParaView 6.2.0 YieldCriteria）
const double kSigma1[kNumTensors] = {100.0, 50.0, 40.0, 200.0, 133.156541, 0.0};
const double kSigma2[kNumTensors] = {0.0, 0.0, 40.0, 100.0, 18.204323, 0.0};
const double kSigma3[kNumTensors] = {0.0, -50.0, 40.0, 50.0, -51.360864, 0.0};
const double kTresca[kNumTensors] = {100.0, 100.0, 0.0, 150.0, 184.517404, 0.0};
const double kVonMises[kNumTensors] = {100.0, 86.60254, 0.0, 132.287566, 161.400124, 0.0};

iGame::PointSet::Pointer MakeTensorPointSet(const char* name, int dimension /*6 或 9*/) {
    using namespace iGame;
    auto ps = PointSet::New();
    auto tensor = DoubleArray::New();
    tensor->SetName(name);
    tensor->SetDimension(dimension);
    for (int i = 0; i < kNumTensors; ++i) {
        ps->AddPoint(Point(static_cast<float>(i), 0.0f, 0.0f));
        const double* t = kTensors[i];
        if (dimension == 6) {
            // xx, yy, zz, xy, yz, zx
            for (int k = 0; k < 6; ++k) { tensor->AddValue(t[k]); }
        } else {
            // 行主序 3x3 对称矩阵
            const double m[9] = {t[0], t[3], t[5], t[3], t[1], t[4], t[5], t[4], t[2]};
            for (int k = 0; k < 9; ++k) { tensor->AddValue(m[k]); }
        }
    }
    ps->GetAttributeSet()->AddAttribute(IG_TENSOR, IG_POINT, tensor);
    return ps;
}

// 取输出里某个标量数组的第 i 个值
bool FetchScalar(iGame::DataObject::Pointer out, const std::string& name, double* values,
                 int count) {
    if (!out) return false;
    auto attrSet = out->GetAttributeSet();
    if (!attrSet) return false;
    const int idx = attrSet->GetAttributeIndex(name);
    if (idx < 0) return false;
    auto array = attrSet->GetAttribute(idx).pointer;
    if (!array || array->GetDimension() != 1) return false;
    if (static_cast<int>(array->GetNumberOfElements()) < count) return false;
    for (int i = 0; i < count; ++i) { values[i] = array->GetValue(i); }
    return true;
}

double MaxError(const double* got, const double* expected, int count) {
    double maxErr = 0.0;
    for (int i = 0; i < count; ++i) {
        maxErr = std::max(maxErr, std::fabs(got[i] - expected[i]));
    }
    return maxErr;
}

} // namespace

int main() {
    using namespace iGame;

    /* ---- 1) 主应力：σ1 ≥ σ2 ≥ σ3 与参考值一致 ---- */
    {
        auto ps = MakeTensorPointSet("Stress", 6);
        auto filter = YieldCriteriaFilter::New();
        filter->SetCriterion(YieldCriteriaFilter::PRINCIPAL_STRESS);
        filter->SetInput(ps);
        const bool ok = filter->Execute();
        Check(ok, "principal stress: execute ok");
        auto out = filter->GetOutput();
        double s1[6], s2[6], s3[6];
        Check(FetchScalar(out, "Stress - Sigma 1", s1, kNumTensors) &&
                      FetchScalar(out, "Stress - Sigma 2", s2, kNumTensors) &&
                      FetchScalar(out, "Stress - Sigma 3", s3, kNumTensors),
              "principal stress: arrays named \"Stress - Sigma i\" exist");
        Check(MaxError(s1, kSigma1, kNumTensors) < 1e-5 &&
                      MaxError(s2, kSigma2, kNumTensors) < 1e-5 &&
                      MaxError(s3, kSigma3, kNumTensors) < 1e-5,
              "principal stress: values match reference (maxErr=" +
                      std::to_string(std::max({MaxError(s1, kSigma1, kNumTensors),
                                               MaxError(s2, kSigma2, kNumTensors),
                                               MaxError(s3, kSigma3, kNumTensors)})) + ")");

        // 主方向：单位矢量 + A·v = λ·v + 相互正交
        auto attrSet = out ? out->GetAttributeSet() : nullptr;
        const int dirIdx = attrSet ? attrSet->GetAttributeIndex("Stress - Sigma 1 (Vector)") : -1;
        Check(dirIdx >= 0, "principal stress: direction vectors are exported");
        if (dirIdx >= 0) {
            auto dirs = attrSet->GetAttribute(dirIdx).pointer;
            bool unit = (dirs != nullptr) && dirs->GetDimension() == 3;
            double worstResidual = 0.0;
            for (int i = 0; i < kNumTensors && unit; ++i) {
                const double vx = dirs->GetValue(i * 3 + 0);
                const double vy = dirs->GetValue(i * 3 + 1);
                const double vz = dirs->GetValue(i * 3 + 2);
                unit = std::fabs(std::sqrt(vx * vx + vy * vy + vz * vz) - 1.0) < 1e-9;
                const double* t = kTensors[i];
                const double m[9] = {t[0], t[3], t[5], t[3], t[1], t[4], t[5], t[4], t[2]};
                const double v[3] = {vx, vy, vz};
                for (int r = 0; r < 3; ++r) {
                    double av = 0.0;
                    for (int c = 0; c < 3; ++c) { av += m[r * 3 + c] * v[c]; }
                    worstResidual = std::max(worstResidual, std::fabs(av - s1[i] * v[r]));
                }
            }
            Check(unit && worstResidual < 1e-8,
                  "principal stress: directions are unit vectors satisfying A*v = sigma1*v (residual=" +
                          std::to_string(worstResidual) + ")");
        }
    }

    /* ---- 2) Tresca 与 von Mises 与参考值一致 ---- */
    {
        auto ps = MakeTensorPointSet("Stress", 6);

        auto tresca = YieldCriteriaFilter::New();
        tresca->SetCriterion(YieldCriteriaFilter::TRESCA);
        tresca->SetInput(ps);
        const bool okT = tresca->Execute();
        double tValues[6] = {0};
        Check(okT && FetchScalar(tresca->GetOutput(), "Stress - Tresca Criterion", tValues, kNumTensors),
              "tresca: array \"Stress - Tresca Criterion\" exists");
        Check(MaxError(tValues, kTresca, kNumTensors) < 1e-5,
              "tresca: values match reference (maxErr=" +
                      std::to_string(MaxError(tValues, kTresca, kNumTensors)) + ")");

        auto mises = YieldCriteriaFilter::New();
        mises->SetCriterion(YieldCriteriaFilter::VON_MISES);
        mises->SetInput(ps);
        const bool okM = mises->Execute();
        double mValues[6] = {0};
        Check(okM && FetchScalar(mises->GetOutput(), "Stress - Von Mises Criterion", mValues,
                                 kNumTensors),
              "von mises: array \"Stress - Von Mises Criterion\" exists");
        Check(MaxError(mValues, kVonMises, kNumTensors) < 1e-5,
              "von mises: values match reference (maxErr=" +
                      std::to_string(MaxError(mValues, kVonMises, kNumTensors)) + ")");
    }

    /* ---- 3) 几何/拓扑不变，原模型不被修改，输出命名 ---- */
    {
        auto ps = MakeTensorPointSet("Stress", 6);
        const IGsize numPointsBefore = ps->GetNumberOfPoints();
        const size_t attrsBefore = ps->GetAttributeSet()->GetNumberOfAttributes();

        auto filter = YieldCriteriaFilter::New();
        filter->SetCriterion(YieldCriteriaFilter::VON_MISES);
        filter->SetInput(ps);
        const bool ok = filter->Execute();
        auto out = filter->GetOutput();
        Check(ok && out != nullptr, "output produced");
        Check(out && out->GetName() == std::string(ps->GetName() + "_YieldCriteria"),
              "output model name is suffixed with _YieldCriteria");
        auto outPs = DynamicCast<PointSet>(out);
        Check(outPs && outPs->GetNumberOfPoints() == numPointsBefore,
              "output keeps input geometry (same point count)");
        Check(ps->GetAttributeSet()->GetNumberOfAttributes() == attrsBefore &&
                      ps->GetAttributeSet()->GetAttributeIndex("Stress - Von Mises Criterion") < 0,
              "input object is not modified (no result array added)");
        // 原张量属性仍然保留
        Check(out && out->GetAttributeSet()->GetAttributeIndex("Stress") >= 0,
              "output keeps the original tensor attribute");
    }

    /* ---- 4) 9 分量张量与 6 分量结果一致 ---- */
    {
        auto ps6 = MakeTensorPointSet("Stress", 6);
        auto ps9 = MakeTensorPointSet("Stress", 9);

        auto f6 = YieldCriteriaFilter::New();
        f6->SetCriterion(YieldCriteriaFilter::VON_MISES);
        f6->SetInput(ps6);
        f6->Execute();
        auto f9 = YieldCriteriaFilter::New();
        f9->SetCriterion(YieldCriteriaFilter::VON_MISES);
        f9->SetInput(ps9);
        const bool ok9 = f9->Execute();

        double v6[6] = {0}, v9[6] = {0};
        const bool got6 = FetchScalar(f6->GetOutput(), "Stress - Von Mises Criterion", v6, kNumTensors);
        const bool got9 = FetchScalar(f9->GetOutput(), "Stress - Von Mises Criterion", v9, kNumTensors);
        Check(ok9 && got6 && got9 && MaxError(v6, v9, kNumTensors) < 1e-9,
              "9-component tensor gives the same result as 6-component");
    }

    /* ---- 5) 指定张量属性名 / 自动选取 / 错误处理 ---- */
    {
        auto ps = MakeTensorPointSet("Stress", 6);

        auto named = YieldCriteriaFilter::New();
        named->SetTensorArrayName("Stress");
        named->SetCriterion(YieldCriteriaFilter::TRESCA);
        named->SetInput(ps);
        Check(named->Execute(), "explicit tensor array name works");

        auto missing = YieldCriteriaFilter::New();
        missing->SetTensorArrayName("NotExist");
        missing->SetInput(ps);
        Check(!missing->Execute() && !missing->GetStatusMessage().empty(),
              "missing tensor array name is rejected with a message");

        // 没有张量属性时应失败
        auto plain = PointSet::New();
        plain->AddPoint(Point(0.0f, 0.0f, 0.0f));
        auto noTensor = YieldCriteriaFilter::New();
        noTensor->SetInput(plain);
        Check(!noTensor->Execute() && noTensor->GetStatusMessage().find("张量") != std::string::npos,
              "input without a tensor attribute is rejected");

        auto noInput = YieldCriteriaFilter::New();
        Check(!noInput->Execute(), "missing input is rejected");
    }

    /* ---- 6) 单元数据上的张量同样支持 ---- */
    {
        auto ps = PointSet::New();
        for (int i = 0; i < kNumTensors; ++i) {
            ps->AddPoint(Point(static_cast<float>(i), 0.0f, 0.0f));
        }
        auto tensor = DoubleArray::New();
        tensor->SetName("CellStress");
        tensor->SetDimension(6);
        for (int i = 0; i < kNumTensors; ++i) {
            for (int k = 0; k < 6; ++k) { tensor->AddValue(kTensors[i][k]); }
        }
        ps->GetAttributeSet()->AddAttribute(IG_TENSOR, IG_CELL, tensor);

        auto filter = YieldCriteriaFilter::New();
        filter->SetCriterion(YieldCriteriaFilter::VON_MISES);
        filter->SetInput(ps);
        const bool ok = filter->Execute();
        auto attrSet = filter->GetOutput() ? filter->GetOutput()->GetAttributeSet() : nullptr;
        const int idx = attrSet ? attrSet->GetAttributeIndex("CellStress - Von Mises Criterion") : -1;
        Check(ok && idx >= 0 && attrSet->GetAttribute(idx).attachmentType == IG_CELL,
              "cell-data tensor is supported and result keeps cell attachment");
        double values[6] = {0};
        Check(idx >= 0 && FetchScalar(filter->GetOutput(), "CellStress - Von Mises Criterion",
                                      values, kNumTensors) &&
                      MaxError(values, kVonMises, kNumTensors) < 1e-5,
              "cell-data tensor: values match reference");
    }

    /* ---- 7) 测试模型文件：人造应力场（可与解析公式核验） ---- */
    // 模型的应力场为 sxx=10x, syy=5y, szz=-8z, sxy=2xy, syz=3yz, szx=4zx
    {
        auto mesh = FileIO::ReadFile("./Models/YieldCriteria_Test.vtk");
        auto loaded = DynamicCast<PointSet>(mesh);
        Check(loaded && loaded->GetNumberOfPoints() == 1331,
              "test model loaded (11x11x11 = 1331 points)");
        if (loaded) {
            auto attrSet = mesh->GetAttributeSet();
            const int tensorIdx = attrSet ? attrSet->GetAttributeIndex("Stress") : -1;
            Check(tensorIdx >= 0 && attrSet->GetAttribute(tensorIdx).type == IG_TENSOR,
                  "test model provides an IG_TENSOR point attribute \"Stress\"");

            auto principal = YieldCriteriaFilter::New();
            principal->SetCriterion(YieldCriteriaFilter::PRINCIPAL_STRESS);
            principal->SetInput(mesh);
            const bool okP = principal->Execute();

            auto mises = YieldCriteriaFilter::New();
            mises->SetCriterion(YieldCriteriaFilter::VON_MISES);
            mises->SetInput(mesh);
            const bool okM = mises->Execute();

            auto tresca = YieldCriteriaFilter::New();
            tresca->SetCriterion(YieldCriteriaFilter::TRESCA);
            tresca->SetInput(mesh);
            const bool okT = tresca->Execute();

            double s1[1331] = {0}, s2[1331] = {0}, s3[1331] = {0};
            double vm[1331] = {0}, tr[1331] = {0};
            const bool fetched =
                    FetchScalar(principal->GetOutput(), "Stress - Sigma 1", s1, 1331) &&
                    FetchScalar(principal->GetOutput(), "Stress - Sigma 2", s2, 1331) &&
                    FetchScalar(principal->GetOutput(), "Stress - Sigma 3", s3, 1331) &&
                    FetchScalar(mises->GetOutput(), "Stress - Von Mises Criterion", vm, 1331) &&
                    FetchScalar(tresca->GetOutput(), "Stress - Tresca Criterion", tr, 1331);
            Check(okP && okM && okT && fetched, "all three criteria run on the test model");

            if (fetched && loaded) {
                // 主值之和应等于张量迹（解析可算），von Mises 应与分量公式一致
                double maxTraceErr = 0.0;
                double maxMisesErr = 0.0;
                double maxTrescaErr = 0.0;
                for (int i = 0; i < 1331; ++i) {
                    const Point& p = loaded->GetPoint(i);
                    const double x = p[0];
                    const double y = p[1];
                    const double z = p[2];
                    const double sxx = 10.0 * x;
                    const double syy = 5.0 * y;
                    const double szz = -8.0 * z;
                    const double sxy = 2.0 * x * y;
                    const double syz = 3.0 * y * z;
                    const double szx = 4.0 * z * x;

                    const double trace = sxx + syy + szz;
                    maxTraceErr = std::max(maxTraceErr, std::fabs((s1[i] + s2[i] + s3[i]) - trace));

                    const double expectedVm = std::sqrt(
                            0.5 * ((sxx - syy) * (sxx - syy) + (syy - szz) * (syy - szz) +
                                   (szz - sxx) * (szz - sxx) +
                                   6.0 * (sxy * sxy + syz * syz + szx * szx)));
                    maxMisesErr = std::max(maxMisesErr, std::fabs(vm[i] - expectedVm));

                    maxTrescaErr = std::max(maxTrescaErr, std::fabs(tr[i] - (s1[i] - s3[i])));
                }
                Check(maxTraceErr < 1e-6,
                      "test model: sigma1+sigma2+sigma3 equals the tensor trace (maxErr=" +
                              std::to_string(maxTraceErr) + ")");
                Check(maxMisesErr < 1e-6,
                      "test model: von Mises matches the component formula (maxErr=" +
                              std::to_string(maxMisesErr) + ")");
                Check(maxTrescaErr < 1e-9,
                      "test model: Tresca equals sigma1 - sigma3 (maxErr=" +
                              std::to_string(maxTrescaErr) + ")");

                // 主值降序，且平均值介于 σ3 与 σ1 之间
                bool ordered = true;
                for (int i = 0; i < 1331 && ordered; ++i) {
                    if (!(s1[i] >= s2[i] && s2[i] >= s3[i])) { ordered = false; }
                    const double mean = (s1[i] + s2[i] + s3[i]) / 3.0;
                    if (mean > s1[i] + 1e-9 || mean < s3[i] - 1e-9) { ordered = false; }
                }
                Check(ordered, "test model: principal stresses are sorted descending");
            }
        }
    }

    /* ---- 8) 6 分量 vtu 模型（同一应力场，供 ParaView 对照） ---- */
    {
        auto mesh = FileIO::ReadFile("./Models/YieldCriteria_ParaView_Test.vtu");
        auto loaded = DynamicCast<PointSet>(mesh);
        Check(loaded && loaded->GetNumberOfPoints() == 1331,
              "6-component vtu model loaded (1331 points)");
        if (loaded) {
            auto attrSet = mesh->GetAttributeSet();
            const int idx = attrSet ? attrSet->GetAttributeIndex("Stress") : -1;
            auto tensor = idx >= 0 ? attrSet->GetAttribute(idx).pointer : nullptr;
            Check(tensor && tensor->GetDimension() == 6,
                  "vtu model provides a 6-component \"Stress\" tensor");

            auto mises = YieldCriteriaFilter::New();
            mises->SetCriterion(YieldCriteriaFilter::VON_MISES);
            mises->SetInput(mesh);
            const bool ok = mises->Execute();
            double vm[1331] = {0};
            const bool got =
                    FetchScalar(mises->GetOutput(), "Stress - Von Mises Criterion", vm, 1331);
            double maxErr = 0.0;
            if (got) {
                for (int i = 0; i < 1331; ++i) {
                    const Point& p = loaded->GetPoint(i);
                    const double x = p[0];
                    const double y = p[1];
                    const double z = p[2];
                    const double sxx = 10.0 * x;
                    const double syy = 5.0 * y;
                    const double szz = -8.0 * z;
                    const double sxy = 2.0 * x * y;
                    const double syz = 3.0 * y * z;
                    const double szx = 4.0 * z * x;
                    const double expected = std::sqrt(
                            0.5 * ((sxx - syy) * (sxx - syy) + (syy - szz) * (syy - szz) +
                                   (szz - sxx) * (szz - sxx) +
                                   6.0 * (sxy * sxy + syz * syz + szx * szx)));
                    maxErr = std::max(maxErr, std::fabs(vm[i] - expected));
                }
            }
            Check(ok && got && maxErr < 1e-6,
                  "vtu model (6-component): von Mises matches the analytic formula (maxErr=" +
                          std::to_string(maxErr) + ")");
            if (!ok || !got) {
                std::cout << "[info] execute ok=" << ok << " got=" << got
                          << " status=" << mises->GetStatusMessage() << std::endl;
                auto out = mises->GetOutput();
                auto outAttrs = out ? out->GetAttributeSet() : nullptr;
                if (outAttrs) {
                    const size_t n = outAttrs->GetNumberOfAttributes();
                    std::cout << "[info] output attributes (" << n << "):";
                    for (size_t i = 0; i < n; ++i) {
                        auto& a = outAttrs->GetAttribute(static_cast<IGsize>(i));
                        if (a.pointer) {
                            std::cout << " [" << a.pointer->GetName() << " d="
                                      << a.pointer->GetDimension() << " n="
                                      << a.pointer->GetNumberOfElements() << "]";
                        }
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "[info] no output attribute set" << std::endl;
                }
            }
        }
    }

    if (g_failed == 0) {
        std::cout << "\nResult: PASS\n";
        return 0;
    }
    std::cout << "\nResult: FAIL (" << g_failed << " checks failed)\n";
    return 1;
}
