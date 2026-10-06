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


void iGameTemporalStatistics::Accumulator::Reset(IGsize elementCount, int channelCount) {
    count = elementCount;
    channels = (channelCount > 0) ? channelCount : 1;
    // 累加器按"每个元素 × 每个通道"开槽：统计量是逐元素算的，
    // 只按通道开槽会把整个网格累加到同一个槽位，输出出来就是一片同色。
    const size_t total = static_cast<size_t>(count) * static_cast<size_t>(channels);
    frameCount = 0;
    sum.assign(total, 0.0);
    minimum.assign(total, std::numeric_limits<double>::max());
    maximum.assign(total, std::numeric_limits<double>::lowest());
}

bool iGameTemporalStatistics::Accumulator::AddFrame(ArrayObject::Pointer array) {
    if (array.IsNull()) { return false; }
    if (array->GetNumberOfElements() != count) { return false; }
    const int dimension = array->GetDimension();
    // 输出通道数恒等于源数组维度：标量源 -> 1 个通道，向量源 -> 逐分量
    if (dimension != channels) { return false; }
    for (IGsize i = 0; i < count; ++i) {
        for (int c = 0; c < dimension; ++c) {
            const size_t k = static_cast<size_t>(i) * static_cast<size_t>(channels) + static_cast<size_t>(c);
            const double value = array->GetElementValue(i, c);
            sum[k] += value;
            minimum[k] = std::min(minimum[k], value);
            maximum[k] = std::max(maximum[k], value);
        }
    }
    ++frameCount;
    return true;
}

ArrayObject::Pointer iGameTemporalStatistics::Accumulator::BuildArray(int kind, const std::string& name) const {
    auto array = FloatArray::New();
    array->SetName(name);
    array->SetDimension(channels);
    array->Resize(count);
    if (count == 0 || channels <= 0) { return array; }

    std::vector<double> element(static_cast<size_t>(channels), 0.0);
    const double divisor = (frameCount > 0) ? static_cast<double>(frameCount) : 1.0;
    for (IGsize i = 0; i < count; ++i) {
        for (int c = 0; c < channels; ++c) {
            const size_t k = static_cast<size_t>(i) * static_cast<size_t>(channels) + static_cast<size_t>(c);
            switch (kind) {
                case 1:
                    element[static_cast<size_t>(c)] = minimum[k];
                    break;
                case 2:
                    element[static_cast<size_t>(c)] = maximum[k];
                    break;
                default:
                    element[static_cast<size_t>(c)] = sum[k] / divisor;
                    break;
            }
        }
        array->SetElement(i, element.data());
    }
    return array;
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
    if (baseAttributes.IsNull()) {
        m_Message = "Frame 0 has no attribute set.";
        return false;
    }

    // 以第 0 帧的块为基准，把所有数组都纳入统计（本过滤器没有属性选择参数）
    auto all = baseAttributes->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& baseAttribute = all->GetElement(i);
        if (baseAttribute.IsNone() || baseAttribute.pointer.IsNull()) { continue; }

        SourceInfo info;
        info.name = baseAttribute.pointer->GetName();
        info.attachment = baseAttribute.attachmentType;
        info.dimension = baseAttribute.pointer->GetDimension();
        // 逐分量统计：输出通道数 = 源维度（标量源自然退化为 1）
        info.channels = (info.dimension > 0) ? info.dimension : 1;

        // 多块输入：所有块都要有同名同隶属、维度一致的数组
        for (size_t b = 1; b < templateBlocks.size(); ++b) {
            auto other = FindArray(templateBlocks[b].attributes.get(), info.name, info.attachment);
            if (other.IsNull()) {
                m_Message = "Block " + std::to_string(b) + " has no array " + info.name + ".";
                return false;
            }
            if (other->GetDimension() != info.dimension) {
                m_Message = "Block " + std::to_string(b) + " array dimension mismatch: " + info.name + ".";
                return false;
            }
        }
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
            copy->SetPoints(source->GetPoints());                       // 共享点坐标（只读）
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
                                         const std::vector<FrameBlock>& templateBlocks,
                                         std::vector<std::vector<Accumulator>>& accumulators) {
    const unsigned int frameNum = static_cast<unsigned int>(frames->GetTimeNum());
    for (unsigned int f = 0; f < frameNum; ++f) {
        UpdateProgress(static_cast<double>(f) / static_cast<double>(frameNum));

        const auto frameBlocks = CollectFrameBlocks(owner, frames, f);
        if (frameBlocks.size() != templateBlocks.size()) {
            m_Message = "Frame " + std::to_string(f) + " block count mismatch (" + std::to_string(frameBlocks.size()) +
                        " vs " + std::to_string(templateBlocks.size()) + ").";
            return false;
        }
        for (size_t b = 0; b < frameBlocks.size(); ++b) {
            for (size_t s = 0; s < m_Sources.size(); ++s) {
                const auto& source = m_Sources[s];
                auto array = FindArray(frameBlocks[b].attributes.get(), source.name, source.attachment);
                if (array.IsNull()) {
                    m_Message = "Frame " + std::to_string(f) + " block " + std::to_string(b) + " has no array " +
                                source.name + ".";
                    return false;
                }
                if (!accumulators[s][b].AddFrame(array)) {
                    m_Message = "Frame " + std::to_string(f) + " block " + std::to_string(b) + " array " + source.name +
                                ": element count or dimension mismatch.";
                    return false;
                }
            }
        }
    }
    UpdateProgress(1.0);
    return true;
}


bool iGameTemporalStatistics::WriteStatistics(DataObject::Pointer output,
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
        const IGenum attributeType = (source.channels > 1) ? IG_VECTOR : IG_SCALAR;

        for (size_t b = 0; b < destinations.size(); ++b) {
            auto* destination = destinations[b];
            if (destination == nullptr) {
                m_Message = "Output block " + std::to_string(b) + " has no attribute set.";
                return false;
            }
            const auto& accumulator = accumulators[s][b];

            // 均值 / 最小值 / 最大值是并列属性，三个都挂到新对象的属性集上
            auto average = accumulator.BuildArray(0, source.name + kAverageSuffix);
            destination->AddAttribute(attributeType, source.attachment, average, ComputeDataRange(average));
            auto minimum = accumulator.BuildArray(1, source.name + kMinimumSuffix);
            destination->AddAttribute(attributeType, source.attachment, minimum, ComputeDataRange(minimum));
            auto maximum = accumulator.BuildArray(2, source.name + kMaximumSuffix);
            destination->AddAttribute(attributeType, source.attachment, maximum, ComputeDataRange(maximum));
        }
    }

    // 容器根：把子对象值域聚合到父级，供颜色条 / 标量面板读取
    if (output->HasSubDataObject()) { output->ReCollectSubDataObjectDataRange(); }
    return true;
}


bool iGameTemporalStatistics::Execute() {
    m_Message.clear();
    m_Sources.clear();

    auto input = GetInput(0);
    if (input.IsNull()) {
        m_Message = "Input is null.";
        return false;
    }

    auto frames = input->PeekTimeFrames();
    if (frames.IsNull() || frames->GetTimeNum() == 0) {
        m_Message = "Input has no time frames.";
        return false;
    }
    const unsigned int frameNum = static_cast<unsigned int>(frames->GetTimeNum());
    if (frameNum < 2) {
        m_Message = "Input has only one time frame.";
        return false;
    }

    // 整段统计会逐帧取数，先按需开帧缓存（与主窗口逐帧算属性差值时的做法一致）
    if (frames->GetMaxCacheSize() < frameNum) { frames->EnableCache(frameNum); }

    // 第 0 帧给出结构模板：块数、每块几何、每块属性
    const auto templateBlocks = CollectFrameBlocks(input, frames, 0);
    if (templateBlocks.empty()) {
        m_Message = "Frame 0 has no data block.";
        return false;
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

    if (!Accumulate(input, frames, templateBlocks, accumulators)) { return false; }

    // 新模型：几何沿用输入、属性集全新，输入对象全程未被修改
    auto output = BuildOutputSkeleton(templateBlocks);
    if (output.IsNull()) {
        m_Message = "Unsupported input data type: cannot build a geometry-sharing output object.";
        return false;
    }
    if (!WriteStatistics(output, accumulators)) { return false; }

    output->SetName(input->GetName() + "_temporal_statistics");
    SetOutput(0, output);
    UpdateProgress(1.0);
    return true;
}

IGAME_NAMESPACE_END
