#pragma once
#include "iGameDataObject.h"
#include "iGameFilter.h"
#include "iGameStreamingData.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class iGameTemporalStatistics
 * @brief 时域统计过滤器（对标 ParaView 的 Temporal Statistics）。
 *
 *  - 逐元素统计：每个点 / 单元在时间维度上各自求平均、最小、最大；
 *  - 输入对象只读，全程不修改；输出是**新数据集**（几何沿用输入的 Points / CellArray，
 *    属性集全新），调用方拿 GetOutput(0) 交给
 *    igQtModelDialogWidget::addDataObjectToModelTree() 即得到一个新模型；
 *  - 结果属性命名固定：<数组名>_average / _minimum / _maximum；
 *  - 向量数组逐分量统计，输出与源同维的向量；标量数组输出标量。
 *
 *
 * 输入支持 StreamingData 的两种帧类型（见 StreamingType）：
 *  - MultiSubFiles（PVD 逐帧多文件 / 多子文件）：每帧是若干块对象，逐块统计；
 *    多块时输出为"容器根 + 每块一个子对象"，模型树结构与输入一致；
 *  - SingleFieldAttributes（单场属性帧，例如 ODB 场数据）：几何挂在宿主对象上，
 *    只有属性集逐帧切换。
 *
 */
class iGameTemporalStatistics : public Filter {
public:
    I_OBJECT(iGameTemporalStatistics);
    static Pointer New() { return new iGameTemporalStatistics; }

    bool Execute() override;

    std::string GetMessage() const { return m_Message; }

protected:
    iGameTemporalStatistics();
    ~iGameTemporalStatistics() override = default;

private:
    static constexpr const char* kAverageSuffix = "_average";
    static constexpr const char* kMinimumSuffix = "_minimum";
    static constexpr const char* kMaximumSuffix = "_maximum";

    // 一个输出块的统计累加器：单块输入只有 1 个，多文件帧每个子文件 1 个。
    struct Accumulator {
        IGsize count{0};          // 参与统计的元素数（点数或单元数）
        IGsize frameCount{0};     // 已累加的帧数（均值要除以它，不是元素数）
        int channels{1};          // 源数组维度：1 = 标量输出；N = N 分量向量输出
        std::vector<double> sum;  // 长度 = count * channels，按"元素 × 通道"排布
        std::vector<double> minimum;
        std::vector<double> maximum;

        void Reset(IGsize elementCount, int channelCount);
        // 累加一帧（逐元素、逐分量）。元素数与维度必须与 count / channels 一致。
        bool AddFrame(ArrayObject::Pointer array);
        // kind：0 = 均值，1 = 最小值，2 = 最大值。
        ArrayObject::Pointer BuildArray(int kind, const std::string& name) const;
    };

    // 第 index 帧的一块数据：geometry 提供点 / 单元，attributes 是该帧该块的属性集。
    struct FrameBlock {
        DataObject::Pointer geometry{};
        AttributeSet::Pointer attributes{};
    };

    // 一个待统计数组解析后的信息（维度 / 隶属 / 输出通道数）。
    struct SourceInfo {
        std::string name{};
        IGenum attachment{IG_POINT};
        int dimension{1};
        int channels{1};
    };

    // 步骤 1：把第 0 帧的全部数组解析成 m_Sources，并校验各块一致。
    bool ResolveSources(const std::vector<FrameBlock>& templateBlocks);
    // 步骤 2：取第 index 帧的所有块（只读，不改变宿主对象的当前帧）。
    static std::vector<FrameBlock> CollectFrameBlocks(DataObject::Pointer owner,
                                                      StreamingData::Pointer frames,
                                                      unsigned int index);
    // 步骤 3：建输出骨架 —— 几何沿用输入、属性集为空；多块输入建容器根 + 每块一个子对象。
    static DataObject::Pointer BuildOutputSkeleton(const std::vector<FrameBlock>& templateBlocks);
    // 按类型新建同构对象并共享几何（需要深拷贝时见 iGamePointAndCellIdsFilter.cpp 的做法）。
    static DataObject::Pointer MakeGeometryCopy(DataObject::Pointer geometry);
    // 步骤 4：逐帧 / 逐数组 / 逐块累加。accumulators 形状为 [源数组][块]。
    bool Accumulate(DataObject::Pointer owner, StreamingData::Pointer frames,
                    const std::vector<FrameBlock>& templateBlocks,
                    std::vector<std::vector<Accumulator>>& accumulators);
    // 步骤 5：把均值 / 最小值 / 最大值写进输出对象的属性集（每个块一份）。
    bool WriteStatistics(DataObject::Pointer output,
                         const std::vector<std::vector<Accumulator>>& accumulators);

    static IGsize ElementCount(DataObject::Pointer geometry, IGenum attachment);
    // attachment 为 IG_NONE 时按名字取第一个匹配，否则必须名字 + 隶属都一致。
    static ArrayObject::Pointer FindArray(AttributeSet* attributes, const std::string& name,
                                          IGenum attachment = IG_NONE);
    // 属性范围：dim + 1 个元素，[0] = 模长范围，[1 + c] = 第 c 分量范围
    // 与 AttributeSet::Attribute::GetDataRange 的口径一致，按 (dim+1)*2 取值。
    static DoubleArray::Pointer ComputeDataRange(ArrayObject::Pointer array);

    std::vector<SourceInfo> m_Sources{};
    std::string m_Message{};
};

IGAME_NAMESPACE_END
