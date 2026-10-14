// ============================================================================
// TemporalStatistics 回归测试
//
// 被测滤波器：iGameCore/Filters/Animation/iGameTemporalStatistics.{h,cpp}
//
// 测试数据：test/Animation Test/result-case1
//   - 401 帧 legacy VTK 序列（VTKfile-0.vtk ... VTKfile-400.vtk），
//     每帧 3212 点 / 2880 单元；
//   - 点数组：Displacement / Velocity / Acceleration / Force（3 分量）、Group / Contact（标量）
//   - 单元数组：Material、Stress-Sig_XY（标量）、Stress-Sig（3 分量）
//
// 检查内容：
//   1) 逐帧读入后能构造 MultiSubFiles 时间序列，滤波器执行成功且无非致命提示；
//   2) 每个源数组都产出 <名>_average / _minimum / _maximum / _stddev 四个统计数组，
//      隶属（点/单元）与元素数、维度与源一致；
//   3) 抽样元素上，平均 / 最小 / 最大 / 标准差与“测试自己按帧重算”的参考值一致；
//   4) 单调性检查：最小值 ≤ 平均值 ≤ 最大值，标准差 ≥ 0。
//
// 用法：
//   TestTemporalStatistics [数据目录] [--all | 帧数上限]
//   默认数据目录 test/Animation Test/result-case1，默认取前 48 帧（全量 401 帧较慢）。
//   数据目录不存在时打印跳过信息并返回 77（CMake 侧已配置 SKIP_RETURN_CODE）。
// ============================================================================

#include <Animation/iGameTemporalStatistics.h>

#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameStreamingData.h>
#include <iGameStringArray.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

int g_checked = 0;
int g_failed = 0;

void Check(bool ok, const std::string& what) {
    ++g_checked;
    if (ok) {
        std::cout << "[ok]   " << what << "\n";
    } else {
        std::cout << "[FAIL] " << what << "\n";
        ++g_failed;
    }
}

void CheckClose(double actual, double expected, const std::string& what) {
    const double tolerance = 1e-3 * std::max(1.0, std::abs(expected));
    const bool ok = std::abs(actual - expected) <= tolerance;
    ++g_checked;
    if (ok) {
        std::cout << "[ok]   " << what << "\n";
    } else {
        std::cout << "[FAIL] " << what << "（实际 " << actual << "，期望 " << expected << "）\n";
        ++g_failed;
    }
}

// 收集目录里的 VTKfile-<数字>.vtk，按编号升序；limit==0 表示全部
std::vector<std::string> CollectFrameFiles(const std::string& dir, size_t limit) {
    namespace fs = std::filesystem;
    std::vector<std::pair<int, std::string>> numbered;
    std::error_code ec;
    for (const auto& entry: fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) { continue; }
        const std::string name = entry.path().filename().string();
        if (name.rfind("VTKfile-", 0) != 0 || entry.path().extension() != ".vtk") { continue; }
        const std::string digits = name.substr(8, name.size() - 8 - 4);
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos) { continue; }
        numbered.emplace_back(std::stoi(digits), entry.path().string());
    }
    std::sort(numbered.begin(), numbered.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<std::string> files;
    for (const auto& item: numbered) {
        if (limit > 0 && files.size() >= limit) { break; }
        files.push_back(item.second);
    }
    return files;
}

// 在某一帧对象上按“名字 + 隶属”找数组
iGame::ArrayObject::Pointer FindFrameArray(const iGame::DataObject::Pointer& obj, const std::string& name,
                                           IGenum attachment) {
    using namespace iGame;
    if (obj.IsNull()) { return nullptr; }
    auto* attributes = obj->GetAttributeSet();
    if (attributes == nullptr) { return nullptr; }
    auto all = attributes->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attribute = all->GetElement(i);
        if (attribute.IsNone() || attribute.pointer.IsNull()) { continue; }
        if (attribute.pointer->GetName() == name && attribute.attachmentType == attachment) {
            return attribute.pointer;
        }
    }
    return nullptr;
}

std::string AttachmentName(IGenum attachment) {
    return attachment == IG_CELL ? "单元" : "点";
}

} // namespace

int main(int argc, char** argv) {
    using namespace iGame;

    std::string dataDir = (argc > 1) ? argv[1] : "test/Animation Test/result-case1";
    size_t frameLimit = 48;
    if (argc > 2) {
        const std::string arg = argv[2];
        frameLimit = (arg == "--all") ? 0 : static_cast<size_t>(std::max(0, std::atoi(arg.c_str())));
    }

    std::cout << "=== TemporalStatistics 回归测试 ===\n"
              << "数据目录: " << dataDir << "\n";

    if (!std::filesystem::exists(dataDir)) {
        std::cout << "[skip] 数据目录不存在（code-only checkout 未拉取 LFS 素材？），跳过测试。\n";
        return 77;
    }

    const auto files = CollectFrameFiles(dataDir, frameLimit);
    if (files.empty()) {
        std::cout << "[skip] 目录里没有 VTKfile-<数字>.vtk，跳过测试。\n";
        return 77;
    }
    std::cout << "使用帧数: " << files.size()
              << (frameLimit == 0 ? "（全部）" : "（默认前 N 帧，可用 --all 跑全量）") << "\n\n";

    // ---- 读入每一帧（只读一次：既作为时间序列缓存，也作为参考值来源）----
    std::vector<DataObject::Pointer> frameObjects;
    frameObjects.reserve(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        auto obj = FileIO::ReadFile(files[i]);
        if (obj.IsNull()) {
            std::cerr << "[FAIL] 读取失败: " << files[i] << "\n";
            return 1;
        }
        frameObjects.push_back(obj);
        if (i == 0 || (i + 1) % 16 == 0) {
            std::cout << "  已读入 " << (i + 1) << " / " << files.size() << " 帧\n";
        }
    }
    std::cout << "\n";

    // ---- 构造 MultiSubFiles 时间序列（每帧一个文件），并开启帧缓存 ----
    auto frames = StreamingData::New();
    for (size_t i = 0; i < frameObjects.size(); ++i) {
        auto names = StringArray::New();
        names->AddElement(files[i]);
        frames->AddTimeStep(static_cast<float>(i), names, StreamingType::MultiSubFiles);
    }
    frames->EnableCache(static_cast<unsigned int>(frameObjects.size()));
    for (size_t i = 0; i < frameObjects.size(); ++i) {
        frames->GetArrays()[i].SetCache(std::vector<Object::Pointer>{frameObjects[i]});
    }

    auto root = DataObject::New();
    root->SetName("AnimationTest");
    root->SetTimeFrames(frames);

    // ---- 执行滤波器（四项统计全开，与面板默认一致）----
    auto filter = iGameTemporalStatistics::New();
    filter->SetInput(root);
    filter->SetComputeAverage(true);
    filter->SetComputeMinimum(true);
    filter->SetComputeMaximum(true);
    filter->SetComputeStandardDeviation(true);
    if (!filter->Execute()) {
        std::cerr << "[FAIL] 滤波器执行失败: " << filter->GetMessage() << "\n";
        return 1;
    }
    auto output = filter->GetOutput(0);
    Check(output.IsNotNull(), "滤波器执行成功并产生输出");
    if (output.IsNull()) { return 1; }
    Check(filter->GetWarningsText().empty(),
          "该数据集各帧数组一致，执行过程无非致命提示" + std::string("（") + filter->GetWarningsText() + "）");

    auto* outputAttributes = output->GetAttributeSet();
    Check(outputAttributes != nullptr, "输出对象带属性集");
    if (outputAttributes == nullptr) { return 1; }

    // ---- 逐个源数组校验 ----
    auto* firstFrameAttributes = frameObjects.front()->GetAttributeSet();
    auto sourceAttributes = firstFrameAttributes->GetAllAttributes();
    size_t arrayCount = 0;
    int sampledValues = 0;

    for (IGsize a = 0; a < sourceAttributes->GetNumberOfElements(); ++a) {
        auto& source = sourceAttributes->GetElement(a);
        if (source.IsNone() || source.pointer.IsNull()) { continue; }
        ++arrayCount;

        const std::string name = source.pointer->GetName();
        const IGenum attachment = source.attachmentType;
        const int dimension = source.pointer->GetDimension();
        const IGsize count = source.pointer->GetNumberOfElements();

        // 四个统计数组是否都存在，且隶属 / 维度 / 元素数与源一致
        const std::vector<std::pair<std::string, const char*>> statistics = {
                {"_average", "平均"}, {"_minimum", "最小"}, {"_maximum", "最大"}, {"_stddev", "标准差"}};
        std::vector<ArrayObject::Pointer> outputs;
        for (const auto& item: statistics) {
            const int index = outputAttributes->GetAttributeIndex(name + item.first);
            if (index < 0) {
                Check(false, "输出缺少数组 " + name + item.first);
                outputs.push_back(nullptr);
                continue;
            }
            auto& attribute = outputAttributes->GetAttribute(static_cast<IGsize>(index));
            outputs.push_back(attribute.pointer);
            Check(attribute.pointer.IsNotNull() && attribute.attachmentType == attachment &&
                          attribute.pointer->GetDimension() == dimension &&
                          attribute.pointer->GetNumberOfElements() == count,
                  "输出数组 " + name + item.first + " 的隶属 / 维度 / 元素数与源一致");
        }

        // 抽样元素：拿滤波器结果与“测试自己按帧重算”的参考值对比
        const IGsize samples[5] = {0, count / 4, count / 2, count * 3 / 4, count > 0 ? count - 1 : 0};
        for (IGsize s = 0; s < 5; ++s) {
            const IGsize element = samples[s];
            if (element >= count) { continue; }
            for (int c = 0; c < dimension; ++c) {
                std::vector<double> values;
                values.reserve(frameObjects.size());
                bool missing = false;
                for (const auto& frame: frameObjects) {
                    auto array = FindFrameArray(frame, name, attachment);
                    if (array.IsNull()) { missing = true; break; }
                    values.push_back(array->GetElementValue(element, c));
                }
                if (missing || values.empty()) { continue; }

                double sum = 0.0;
                double minimum = values.front();
                double maximum = values.front();
                for (double value: values) {
                    sum += value;
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
                const double average = sum / static_cast<double>(values.size());
                double squared = 0.0;
                for (double value: values) {
                    const double delta = value - average;
                    squared += delta * delta;
                }
                const double stddev = std::sqrt(squared / static_cast<double>(values.size()));

                const std::string where = " (" + AttachmentName(attachment) + " " +
                                          std::to_string(element) + " 元素, 分量 " + std::to_string(c) + ")";
                if (outputs[0]) { CheckClose(outputs[0]->GetElementValue(element, c), average, name + "_average" + where); }
                if (outputs[1]) { CheckClose(outputs[1]->GetElementValue(element, c), minimum, name + "_minimum" + where); }
                if (outputs[2]) { CheckClose(outputs[2]->GetElementValue(element, c), maximum, name + "_maximum" + where); }
                if (outputs[3]) { CheckClose(outputs[3]->GetElementValue(element, c), stddev, name + "_stddev" + where); }
                ++sampledValues;

                // 单调性与非负
                if (outputs[0] && outputs[1] && outputs[2]) {
                    const double avgOut = outputs[0]->GetElementValue(element, c);
                    const double minOut = outputs[1]->GetElementValue(element, c);
                    const double maxOut = outputs[2]->GetElementValue(element, c);
                    Check(minOut <= avgOut + 1e-6 && avgOut <= maxOut + 1e-6,
                          name + " 满足 最小 ≤ 平均 ≤ 最大" + where);
                }
                if (outputs[3]) {
                    Check(outputs[3]->GetElementValue(element, c) >= -1e-6,
                          name + "_stddev 非负" + where);
                }
            }
        }
    }

    Check(arrayCount > 0, "源数据里至少有 1 个数组参与统计");

    // ---- 汇总 ----
    std::cout << "\n=== 汇总 ===\n"
              << "帧数: " << files.size() << "，源数组数: " << arrayCount
              << "，抽样比对值: " << sampledValues << "，断言数: " << g_checked
              << "，失败: " << g_failed << "\n";
    if (g_failed == 0) {
        std::cout << "全部通过。\n";
        return 0;
    }
    std::cout << "存在失败项，见上面 [FAIL] 行。\n";
    return 1;
}
