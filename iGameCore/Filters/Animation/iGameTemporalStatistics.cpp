#include "iGameTemporalStatistics.h"

#include "iGameDrawObject.h"
#include "iGameFlatArray.h"
#include "iGamePointSet.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>

IGAME_NAMESPACE_BEGIN

iGameTemporalStatistics::iGameTemporalStatistics() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

/* ============================ 累加器 ============================ */

void iGameTemporalStatistics::Accumulator::Reset(IGsize elementCount, int channelCount) {
    count = elementCount;
    channels = (channelCount > 0) ? channelCount : 1;
    frameCount = 0;
    sampleCount = 0;
    // 累加器按"每个元素 × 每个通道"开槽：统计量是逐元素算的，
    // 只按通道开槽会把整个网格累加到同一个槽位，输出出来就是一片同色。
    const size_t total = static_cast<size_t>(count) * static_cast<size_t>(channels);
    sum.assign(total, 0.0);
    m2.assign(total, 0.0);
    minimum.assign(total, std::numeric_limits<double>::max());
    maximum.assign(total, std::numeric_limits<double>::lowest());
}

bool iGameTemporalStatistics::Accumulator::AddFrame(ArrayObject::Pointer array) {
    if (array.IsNull()) { return false; }
    if (array->GetNumberOfElements() != count) { return false; }
    const int dimension = array->GetDimension();
    if (dimension != channels) { return false; }

    // 均值 = 累加和 / 总帧数（缺数组的帧只跳过取值、不跳过计数，与 ParaView 一致）；
    // 标准差用 Welford 一趟算法，样本序号取"实际取到值的帧数"。
    const double prevCount = static_cast<double>(sampleCount);
    const double nextCount = prevCount + 1.0;
    for (IGsize i = 0; i < count; ++i) {
        for (int c = 0; c < dimension; ++c) {
            const size_t k = static_cast<size_t>(i) * static_cast<size_t>(channels) +
                             static_cast<size_t>(c);
            const double value = array->GetElementValue(i, c);
            const double prevMean = (prevCount > 0.0) ? (sum[k] / prevCount) : 0.0;
            const double delta = value - prevMean;
            sum[k] += value;
            const double nextMean = sum[k] / nextCount;
            m2[k] += delta * (value - nextMean);
            minimum[k] = std::min(minimum[k], value);
            maximum[k] = std::max(maximum[k], value);
        }
    }
    ++sampleCount;
    return true;
}

ArrayObject::Pointer iGameTemporalStatistics::NewArrayLike(IGenum arrayType, const std::string& name,
                                                           int channels, IGsize count) {
    // 与 vtkTemporalStatistics 一致：输出数组沿用源数组的数据类型
    // （整型数组的平均值同样在写回时按整型截断，和 ParaView 表现相同）
    ArrayObject::Pointer array;
    switch (arrayType) {
        case IG_DoubleArray: array = DoubleArray::New(); break;
        case IG_IntArray:
        case IG_INTARRAY: array = IntArray::New(); break;
        case IG_UnsignedIntArray: array = UnsignedIntArray::New(); break;
        case IG_CharArray: array = CharArray::New(); break;
        case IG_UnsignedCharArray: array = UnsignedCharArray::New(); break;
        case IG_ShortArray: array = ShortArray::New(); break;
        case IG_UnsignedShortArray: array = UnsignedShortArray::New(); break;
        case IG_LongLongArray: array = LongLongArray::New(); break;
        case IG_UnsignedLongLongArray: array = UnsignedLongLongArray::New(); break;
        case IG_FloatArray:
        default: array = FloatArray::New(); break;
    }
    array->SetName(name);
    array->SetDimension(channels);
    array->Resize(count);
    return array;
}

ArrayObject::Pointer iGameTemporalStatistics::Accumulator::BuildArray(int kind, const std::string& name,
                                                                      IGenum arrayType) const {
    auto array = NewArrayLike(arrayType, name, channels, count);
    if (count == 0 || channels <= 0) { return array; }

    const double divisor = (frameCount > 0) ? static_cast<double>(frameCount) : 1.0;
    std::vector<double> element(static_cast<size_t>(channels), 0.0);
    for (IGsize i = 0; i < count; ++i) {
        for (int c = 0; c < channels; ++c) {
            const size_t k = static_cast<size_t>(i) * static_cast<size_t>(channels) +
                             static_cast<size_t>(c);
            switch (kind) {
                case StatisticMinimum: element[static_cast<size_t>(c)] = minimum[k]; break;
                case StatisticMaximum: element[static_cast<size_t>(c)] = maximum[k]; break;
                case StatisticStandardDeviation: {
                    // 总体标准差（除以总帧数），与 vtkTemporalStatistics 一致
                    const double variance = (frameCount > 0) ? (m2[k] / divisor) : 0.0;
                    element[static_cast<size_t>(c)] = (variance > 0.0) ? std::sqrt(variance) : 0.0;
                    break;
                }
                default: {
                    // 平均值 = 累加和 / 总帧数
                    element[static_cast<size_t>(c)] = sum[k] / divisor;
                    break;
                }
            }
        }
        array->SetElement(i, element.data());
    }
    return array;
}

/* ============================== 工具 ============================== */

void iGameTemporalStatistics::AddWarning(const std::string& text) {
    if (text.empty()) { return; }
    // 去重 + 上限，避免逐帧刷屏
    constexpr size_t kMaxWarnings = 12;
    for (const auto& existing: m_Warnings) {
        if (existing == text) { return; }
    }
    if (m_Warnings.size() >= kMaxWarnings) {
        if (m_Warnings.size() == kMaxWarnings) {
            m_Warnings.push_back("…… 其余提示已省略。");
        }
        return;
    }
    m_Warnings.push_back(text);
}

std::string iGameTemporalStatistics::GetWarningsText() const {
    std::string text;
    for (const auto& warning: m_Warnings) {
        if (!text.empty()) { text += "\n"; }
        text += warning;
    }
    return text;
}

ArrayObject::Pointer iGameTemporalStatistics::FindArray(AttributeSet* attributes, const std::string& name,
                                                        IGenum attachment) {
    if (attributes == nullptr || name.empty()) { return nullptr; }
    if (attachment == IG_NONE) {
        const int index = attributes->GetAttributeIndex(name);
        if (index < 0) { return nullptr; }
        return attributes->GetAttribute(static_cast<IGsize>(index)).pointer;
    }
    // 名字 + 隶属同时匹配，避免同名 Point / Cell 数组取错
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

IGsize iGameTemporalStatistics::ElementCount(DataObject::Pointer geometry, IGenum attachment) {
    if (geometry.IsNull()) { return 0; }
    if (attachment == IG_POINT) {
        auto points = geometry->GetPoints();
        return points.IsNull() ? 0 : points->GetNumberOfPoints();
    }
    auto cells = geometry->GetCellArray();
    return cells.IsNull() ? 0 : cells->GetNumberOfCells();
}

DoubleArray::Pointer iGameTemporalStatistics::ComputeDataRange(ArrayObject::Pointer array) {
    // 口径与 AttributeSet::Attribute::GetDataRange 一致：
    // 元素 0 = 模长范围，元素 1 + c = 第 c 分量范围，共 dimension + 1 个元素。
    const int dimension = array.IsNull() ? 1 : array->GetDimension();
    const IGsize elementCount = array.IsNull() ? 0 : array->GetNumberOfElements();

    std::vector<double> minimum(static_cast<size_t>(dimension) + 1, std::numeric_limits<double>::max());
    std::vector<double> maximum(static_cast<size_t>(dimension) + 1, std::numeric_limits<double>::lowest());
    if (elementCount > 0 && dimension > 0) {
        std::vector<double> element(static_cast<size_t>(dimension), 0.0);
        for (IGsize i = 0; i < elementCount; ++i) {
            array->GetElement(i, element.data());
            double square = 0.0;
            for (int c = 0; c < dimension; ++c) {
                const size_t k = static_cast<size_t>(c);
                const double value = element[k];
                square += value * value;
                minimum[k + 1] = std::min(minimum[k + 1], value);
                maximum[k + 1] = std::max(maximum[k + 1], value);
            }
            const double magnitude = std::sqrt(square);
            minimum[0] = std::min(minimum[0], magnitude);
            maximum[0] = std::max(maximum[0], magnitude);
        }
    } else {
        std::fill(minimum.begin(), minimum.end(), 0.0);
        std::fill(maximum.begin(), maximum.end(), 0.0);
    }

    auto range = DoubleArray::New();
    range->SetDimension(2);
    range->Resize(static_cast<IGsize>(dimension) + 1);
    for (int i = 0; i <= dimension; ++i) {
        range->SetValue(static_cast<IGsize>(2 * i), minimum[static_cast<size_t>(i)]);
        range->SetValue(static_cast<IGsize>(2 * i + 1), maximum[static_cast<size_t>(i)]);
    }
    return range;
}

std::vector<iGameTemporalStatistics::FrameBlock>
iGameTemporalStatistics::CollectFrameBlocks(DataObject::Pointer owner, StreamingData::Pointer frames,
                                            unsigned int index) {
    std::vector<FrameBlock> blocks;
    if (owner.IsNull() || frames.IsNull() || index >= frames->GetTimeNum()) { return blocks; }

    // GetTargetTimeFrameData 只读帧数据（带缓存时直接复用），不会改宿主对象的当前帧
    const auto frameData = frames->GetTargetTimeFrameData(index);
    if (frames->GetTargetFrameType(index) == StreamingType::MultiSubFiles) {
        // 每帧若干子文件：每个子文件是自带几何与属性的对象
        for (auto& data: frameData) {
            auto object = DynamicCast<DataObject>(data);
            if (object.IsNull()) { continue; }
            blocks.push_back(FrameBlock{object, object->GetAttributeSet()});
        }
        return blocks;
    }

    // SingleFieldAttributes：几何挂在宿主对象上，只有属性集随帧切换
    for (auto& data: frameData) {
        auto attributeSet = DynamicCast<AttributeSet>(data);
        if (attributeSet.IsNull()) { continue; }
        blocks.push_back(FrameBlock{owner, attributeSet});
    }
    return blocks;
}

bool iGameTemporalStatistics::ResolveSources(const std::vector<FrameBlock>& templateBlocks) {
    m_Sources.clear();
    auto baseAttributes = templateBlocks.front().attributes;
    if (baseAttributes.IsNull()) { m_Message = "Frame 0 has no attribute set."; return false; }

    // 以模板帧的块为基准，把所有数组都纳入统计（本过滤器没有属性选择参数）
    auto all = baseAttributes->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& baseAttribute = all->GetElement(i);
        if (baseAttribute.IsNone() || baseAttribute.pointer.IsNull()) { continue; }

        SourceInfo info;
        info.name = baseAttribute.pointer->GetName();
        info.attachment = baseAttribute.attachmentType;
        info.arrayType = baseAttribute.pointer->GetArrayType();
        info.dimension = baseAttribute.pointer->GetDimension();
        // 逐分量统计：输出通道数 = 源维度（标量源自然退化为 1）
        info.channels = (info.dimension > 0) ? info.dimension : 1;
        m_Sources.push_back(info);
    }

    if (m_Sources.empty()) {
        m_Message = "Input has no array to compute statistics on.";
        return false;
    }
    return true;
}

DataObject::Pointer iGameTemporalStatistics::MakeGeometryCopy(DataObject::Pointer geometry) {
    if (geometry.IsNull()) { return nullptr; }

    switch (geometry->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH: {
            auto source = DynamicCast<UnstructuredMesh>(geometry);
            if (source.IsNull()) { return nullptr; }
            auto copy = UnstructuredMesh::New();
            copy->SetPoints(source->GetPoints());   // 共享点坐标（只读）
            copy->SetCells(source->GetCells(), source->GetCellTypes()); // 共享拓扑
            return copy;
        }
        case IG_VOLUME_MESH: {
            auto source = DynamicCast<VolumeMesh>(geometry);
            if (source.IsNull()) { return nullptr; }
            auto copy = VolumeMesh::New();
            copy->SetPoints(source->GetPoints());
            copy->SetFaces(source->GetFaces());
            copy->SetVolumes(source->GetVolumes());
            // TODO(多面体 / 结构化网格)：多面体还需要重建面-体索引
            // （见 iGamePointAndCellIdsFilter.cpp 的 DeepCopyDataObject 里 InitVolumesWithPolyhedron
            // 那段），StructuredMesh 需要走自己的路径。
            return copy;
        }
        case IG_SURFACE_MESH: {
            auto source = DynamicCast<SurfaceMesh>(geometry);
            if (source.IsNull()) { return nullptr; }
            auto copy = SurfaceMesh::New();
            copy->SetPoints(source->GetPoints());
            copy->SetFaces(source->GetFaces());
            if (source->GetEdges() != nullptr) { copy->SetEdges(source->GetEdges()); }
            return copy;
        }
        case IG_POINT_SET: {
            auto copy = PointSet::New();
            copy->SetPoints(geometry->GetPoints());
            return copy;
        }
        default:
            // TODO：DrawObject 容器根（DataObject::CreateDataObject(IG_DRAW_OBJECT) 返回 nullptr）、
            //       StructuredMesh、LagrangeUnstructuredMesh 等按需要补齐；
            //       完整版可参考 iGamePointAndCellIdsFilter.cpp 的 DeepCopyDataObject()。
            return nullptr;
    }
}

DataObject::Pointer iGameTemporalStatistics::BuildOutputSkeleton(const std::vector<FrameBlock>& templateBlocks) {
    if (templateBlocks.empty()) { return nullptr; }

    if (templateBlocks.size() == 1) { return MakeGeometryCopy(templateBlocks.front().geometry); }

    // 多块：容器根 + 每块一个子对象，模型树结构与输入一致
    DataObject::Pointer root = DrawObject::New();
    if (root.IsNull()) { return nullptr; }
    for (const auto& block: templateBlocks) {
        auto child = MakeGeometryCopy(block.geometry);
        if (child.IsNull()) { return nullptr; }
        root->AddSubDataObject(child);
    }
    return root;
}

bool iGameTemporalStatistics::Accumulate(DataObject::Pointer owner, StreamingData::Pointer frames,
                                         const std::vector<FrameBlock>& templateBlocks, unsigned int frameNum,
                                         std::vector<std::vector<Accumulator>>& accumulators) {
    auto markFrameProcessed = [&accumulators]() {
        for (auto& perBlock: accumulators) {
            for (auto& accumulator: perBlock) { accumulator.MarkFrameProcessed(); }
        }
    };

    for (unsigned int f = 0; f < frameNum; ++f) {
        UpdateProgress(static_cast<double>(f) / static_cast<double>(frameNum));

        const auto frameBlocks = CollectFrameBlocks(owner, frames, f);
        if (frameBlocks.size() != templateBlocks.size()) {
            // 与 ParaView 一致：不整体失败，跳过该帧并提示
            AddWarning("第 " + std::to_string(f + 1) + " 帧的数据块数量（" +
                       std::to_string(frameBlocks.size()) + "）与模板帧（" +
                       std::to_string(templateBlocks.size()) + "）不一致，已跳过该帧。");
            markFrameProcessed();
            continue;
        }

        for (size_t b = 0; b < frameBlocks.size(); ++b) {
            for (size_t s = 0; s < m_Sources.size(); ++s) {
                auto& source = m_Sources[s];
                if (!source.valid) { continue; }

                auto array = FindArray(frameBlocks[b].attributes.get(), source.name, source.attachment);
                if (array.IsNull()) {
                    // 该帧缺这个数组：跳过它的值，但帧数照常累加（均值按总帧数算）
                    ++source.missingFrames;
                    AddWarning("数组 " + source.name + " 在第 " + std::to_string(f + 1) +
                               " 帧缺失，已跳过该帧（均值 / 标准差仍按总帧数计算）。");
                    continue;
                }
                if (!accumulators[s][b].AddFrame(array)) {
                    // 规模随帧变化：丢弃该数组（与 VTK 的"移除该数组"行为一致）
                    source.valid = false;
                    AddWarning("数组 " + source.name + " 在第 " + std::to_string(f + 1) +
                               " 帧的元素数或维度与模板帧不一致（网格随时间变化），已丢弃该数组的统计结果。");
                    continue;
                }
            }
        }
        markFrameProcessed();
    }
    UpdateProgress(1.0);
    return true;
}

bool iGameTemporalStatistics::WriteStatistics(
        DataObject::Pointer output,
        const std::vector<std::vector<Accumulator>>& accumulators) {
    // 输出对象与累加器的对应关系：多块是"容器根 + 子对象"，单块就是对象自己
    std::vector<AttributeSet*> destinations;
    if (output->HasSubDataObject()) {
        for (auto it = output->SubDataObjectIteratorBegin(); it != output->SubDataObjectIteratorEnd(); ++it) {
            if (it->second) { destinations.push_back(it->second->GetAttributeSet()); }
        }
    } else {
        destinations.push_back(output->GetAttributeSet());
    }
    const size_t expectedBlocks = accumulators.empty() ? 0 : accumulators.front().size();
    if (destinations.size() != expectedBlocks) {
        m_Message = "Output block count mismatch.";
        return false;
    }

    for (size_t s = 0; s < m_Sources.size(); ++s) {
        const auto& source = m_Sources[s];
        if (!source.valid) { continue; } // 规模随帧变化的数组：整体丢弃
        const IGenum attributeType = (source.channels > 1) ? IG_VECTOR : IG_SCALAR;

        for (size_t b = 0; b < destinations.size(); ++b) {
            auto* destination = destinations[b];
            if (destination == nullptr) {
                m_Message = "Output block " + std::to_string(b) + " has no attribute set.";
                return false;
            }
            const auto& accumulator = accumulators[s][b];

            if (m_ComputeAverage) {
                auto array = accumulator.BuildArray(StatisticAverage, source.name + kAverageSuffix,
                                                    source.arrayType);
                destination->AddAttribute(attributeType, source.attachment, array, ComputeDataRange(array));
            }
            if (m_ComputeMinimum) {
                auto array = accumulator.BuildArray(StatisticMinimum, source.name + kMinimumSuffix,
                                                    source.arrayType);
                destination->AddAttribute(attributeType, source.attachment, array, ComputeDataRange(array));
            }
            if (m_ComputeMaximum) {
                auto array = accumulator.BuildArray(StatisticMaximum, source.name + kMaximumSuffix,
                                                    source.arrayType);
                destination->AddAttribute(attributeType, source.attachment, array, ComputeDataRange(array));
            }
            if (m_ComputeStandardDeviation) {
                auto array = accumulator.BuildArray(StatisticStandardDeviation,
                                                    source.name + kStandardDeviationSuffix,
                                                    source.arrayType);
                destination->AddAttribute(attributeType, source.attachment, array, ComputeDataRange(array));
            }
        }
    }

    // 容器根：把子对象值域聚合到父级，供颜色条 / 标量面板读取
    if (output->HasSubDataObject()) { output->ReCollectSubDataObjectDataRange(); }
    return true;
}

/* ============================= 执行 ============================= */

bool iGameTemporalStatistics::Execute() {
    m_Message.clear();
    m_Warnings.clear();
    m_Sources.clear();

    auto input = GetInput(0);
    if (input.IsNull()) { m_Message = "Input is null."; return false; }

    if (!m_ComputeAverage && !m_ComputeMinimum && !m_ComputeMaximum && !m_ComputeStandardDeviation) {
        m_Message = "No statistic selected.";
        return false;
    }

    auto frames = input->PeekTimeFrames();
    if (frames.IsNull() || frames->GetTimeNum() == 0) {
        m_Message = "Input has no time frames.";
        return false;
    }
    const unsigned int frameNum = static_cast<unsigned int>(frames->GetTimeNum());

    // 整段统计会逐帧取数，先按需开帧缓存（与主窗口逐帧算法一致的做法）
    if (frames->GetMaxCacheSize() < frameNum) { frames->EnableCache(frameNum); }

    // 模板帧：优先用第 0 帧；若第 0 帧没有数据块，则退到第一个有数据的帧
    unsigned int templateIndex = 0;
    std::vector<FrameBlock> templateBlocks;
    for (; templateIndex < frameNum; ++templateIndex) {
        templateBlocks = CollectFrameBlocks(input, frames, templateIndex);
        if (!templateBlocks.empty()) { break; }
    }
    if (templateBlocks.empty()) {
        m_Message = "Input has no data block in any time frame.";
        return false;
    }
    if (templateIndex != 0) {
        AddWarning("第 1 帧没有数据块，已改用第 " + std::to_string(templateIndex + 1) + " 帧作为结构模板。");
    }

    if (!ResolveSources(templateBlocks)) { return false; }

    // accumulators[源数组][块]：元素数（点数 / 单元数）必须与输出几何一致
    std::vector<std::vector<Accumulator>> accumulators(m_Sources.size());
    for (size_t s = 0; s < m_Sources.size(); ++s) {
        accumulators[s].resize(templateBlocks.size());
        for (size_t b = 0; b < templateBlocks.size(); ++b) {
            const IGsize elementCount = ElementCount(templateBlocks[b].geometry, m_Sources[s].attachment);
            if (elementCount == 0) {
                m_Message = "Block " + std::to_string(b) + " has no " +
                            (m_Sources[s].attachment == IG_POINT ? std::string("point") : std::string("cell")) +
                            " data for array " + m_Sources[s].name + ".";
                return false;
            }
            accumulators[s][b].Reset(elementCount, m_Sources[s].channels);
        }
    }

    if (!Accumulate(input, frames, templateBlocks, frameNum, accumulators)) { return false; }

    // 新模型：几何沿用输入、属性集全新，输入对象全程未被修改
    auto output = BuildOutputSkeleton(templateBlocks);
    if (output.IsNull()) {
        m_Message = "Unsupported input data type: cannot build a geometry-sharing output object.";
        return false;
    }
    if (!WriteStatistics(output, accumulators)) { return false; }

    if (frameNum == 1) {
        AddWarning("输入只有 1 帧：平均值 = 该帧值，最小值 = 最大值 = 该帧值，标准差 = 0。");
    }

    // 与 ParaView 对应：记录本次处理的时间步。
    // iGame 没有 vtkFieldData 那样的场数据容器，等价地写进输出对象的 Metadata（名字沿用 time_steps）。
    if (auto* metadata = output->GetMetadata()) {
        auto timeSteps = DoubleArray::New();
        timeSteps->SetDimension(1);
        timeSteps->SetName("time_steps");
        for (unsigned int f = 0; f < frameNum; ++f) {
            timeSteps->AddValue(static_cast<double>(frames->GetTargetTimeValue(f)));
        }
        metadata->AddDoubleArray("time_steps", timeSteps);
    }

    output->SetName(input->GetName() + "_temporal_statistics");
    SetOutput(0, output);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
