//
// VTK XML ImageData(.vti) 读取器。
//

/**
 * @class   iGameVTIReader
 * @brief   iGameVTIReader's brief
 */

#include "iGameVTIReader.h"

#include <zlib.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <iGameStructuredMesh.h>
#include <iGameType.h>
#include <tinyxml2.h>

#undef max
#undef min

IGAME_NAMESPACE_BEGIN

namespace {
using ByteBuffer = std::vector<unsigned char>;

int Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

/**
 * @brief 流式 base64 解码器。
 *
 * VTK 的 appended/binary 数据是不带换行的 base64 文本流，同一个 <AppendedData> 里会有多段，
 * 所以不能整体解码后再按 offset 索引 —— 必须从 offset 对应的字符开始就地解码。本类为此服务：
 *  - 自动跳过空白；
 *  - '=' 只作为 quartet 内的补位字符（2 个补位出 1 字节，1 个补位出 2 字节），
 *    **不作为输入结束**：VTK 写压缩块时会把「压缩头 + 各块长度表」和「压缩数据」分别做一次
 *    base64 编码，因此补齐符会出现在同一数据块的中间（3DMatrix.vti 的 vtkValidPointMask 就是这样）。
 *    若把 '=' 当成结束，后面的压缩数据会被丢掉，导致该块 zlib 流被截断、解压失败；
 *  - 遇 '\0' 或 '<'（结束标签）才算输入结束。
 */
class Base64Stream {
public:
    explicit Base64Stream(const char* text) : m_Pos(text) {}

    /* 持续解码，直到 out 至少含 atLeastBytes 个字节，或输入结束。 */
    void DecodeTo(IGsize atLeastBytes, ByteBuffer& out) {
        if (m_Pos == nullptr || m_Finished) { return; }
        while (out.size() < atLeastBytes) {
            const char c = *m_Pos;
            if (c == '\0' || c == '<') {
                m_Finished = true;
                break;
            }
            if (c == '=') {
                m_Quartet[m_Count++] = 0;
                ++m_Padding;
            } else {
                const int value = Base64Value(c);
                if (value < 0) { // 空白等无关字符
                    ++m_Pos;
                    continue;
                }
                m_Quartet[m_Count++] = value;
            }
            ++m_Pos;

            if (m_Count == 4) {
                out.push_back(static_cast<unsigned char>((m_Quartet[0] << 2) | (m_Quartet[1] >> 4)));
                if (m_Padding < 2) {
                    out.push_back(static_cast<unsigned char>(((m_Quartet[1] & 0x0f) << 4) | (m_Quartet[2] >> 2)));
                }
                if (m_Padding < 1) {
                    out.push_back(static_cast<unsigned char>(((m_Quartet[2] & 0x03) << 6) | m_Quartet[3]));
                }
                m_Count = 0;
                m_Padding = 0;
            }
        }
    }

private:
    const char* m_Pos{nullptr};
    int m_Quartet[4]{0, 0, 0, 0};
    int m_Count{0};
    int m_Padding{0};
    bool m_Finished{false};
};

/* 翻转整数的字节序（BigEndian 数据 -> 本机字节序）。 */
template<typename T>
T SwapByteOrder(T value) {
    unsigned char bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
    std::reverse(bytes, bytes + sizeof(T));
    T swapped{};
    std::memcpy(&swapped, bytes, sizeof(T));
    return swapped;
}

/* 按文件字节序读取一个二进制值：byte_order="LittleEndian"（缺省）直接按本机解释，
   "BigEndian" 时翻转字节。头部字段与数组元素都用它，保证两者解释一致。 */
template<typename T>
T ReadBinary(const unsigned char* p, bool bigEndian) {
    T value{};
    std::memcpy(&value, p, sizeof(T));
    return bigEndian ? SwapByteOrder(value) : value;
}

/* 读取压缩块头部的第 index 个字段（字段宽度由 header_type 决定，字节序由 byte_order 决定）。 */
uint64_t ReadHeaderField(const ByteBuffer& buffer, size_t index, bool header8, bool bigEndian) {
    const size_t unit = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
    const unsigned char* p = buffer.data() + index * unit;
    return header8 ? ReadBinary<uint64_t>(p, bigEndian)
                   : static_cast<uint64_t>(ReadBinary<uint32_t>(p, bigEndian));
}

/* 压缩块头部字段个数：numBlocks、blockSize、lastBlockSize、compressedSize[numBlocks]。 */
size_t HeaderFieldCount(uint64_t numBlocks) { return static_cast<size_t>(3 + numBlocks); }

/**
 * @brief 解析 vtkZLibDataCompressor 的输出。
 *
 * 头部布局（字段宽度由 VTKFile/@header_type 决定，UInt32 → 4 字节，UInt64 → 8 字节；
 *          字节序由 VTKFile/@byte_order 决定，BigEndian 时逐个字段翻转）：
 *   [0]               numBlocks        压缩块个数
 *   [1]               blockSize        每块解压后的字节数
 *   [2]               lastBlockSize    最后一块解压后的字节数
 *   [3 .. 3+n-1]      各块压缩后的字节数（与数据部分顺序一致）
 * 数据部分：numBlocks 个独立 zlib 流，逐个解压（字节流本身与字节序无关，解压后仍是文件字节序）。
 */
bool DecompressVTKZLib(const ByteBuffer& encoded, bool header8, bool bigEndian, IGsize expectedBytes, ByteBuffer& out) {
    const size_t unit = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
    out.clear();
    if (encoded.size() < unit * 3) { return false; }

    const uint64_t numBlocks = ReadHeaderField(encoded, 0, header8, bigEndian);
    const uint64_t blockSize = ReadHeaderField(encoded, 1, header8, bigEndian);
    const uint64_t lastBlockSize = ReadHeaderField(encoded, 2, header8, bigEndian);
    // 防御损坏文件：块数不可能多于头部区本身能容纳的字段数
    if (numBlocks == 0 || blockSize == 0 || numBlocks > encoded.size() / unit) { return false; }

    const size_t headerBytes = unit * HeaderFieldCount(numBlocks);
    if (encoded.size() < headerBytes) { return false; }

    out.reserve(static_cast<size_t>((numBlocks - 1) * blockSize + lastBlockSize));
    size_t srcOffset = headerBytes;
    for (uint64_t block = 0; block < numBlocks; ++block) {
        const size_t compressedSize = static_cast<size_t>(ReadHeaderField(encoded, 3 + block, header8, bigEndian));
        const size_t expectedSize =
                (block + 1 == numBlocks) ? static_cast<size_t>(lastBlockSize) : static_cast<size_t>(blockSize);
        if (compressedSize == 0 || expectedSize == 0 || srcOffset + compressedSize > encoded.size()) {
            out.clear();
            return false;
        }

        const size_t dstOffset = out.size();
        out.resize(dstOffset + expectedSize);
        uLongf dstLen = static_cast<uLongf>(expectedSize);
        const int status = uncompress(out.data() + dstOffset, &dstLen, encoded.data() + srcOffset,
                                     static_cast<uLong>(compressedSize));
        if (status != Z_OK || dstLen != expectedSize) {
            out.clear();
            return false;
        }
        out.resize(dstOffset + dstLen);
        srcOffset += compressedSize;
    }
    return expectedBytes == 0 || out.size() == expectedBytes;
}

/**
 * @brief 从 base64 文本（内联 binary 或 appended+base64）读取一个数据块。
 *
 * 有压缩器时块内容就是压缩器输出（自描述，没有额外的字节数头）；无压缩器时块内容为
 * [字节数][数据]，字节数宽度由 header_type 决定。base64 只是文本编码层，
 * 解码出来的字节流仍按文件的 byte_order 解释，因此 bigEndian 要一路传下去。
 */
bool ReadBase64Block(const char* text, bool header8, bool compressed, bool bigEndian, IGsize expectedBytes,
                     ByteBuffer& payload) {
    payload.clear();
    if (text == nullptr) { return false; }

    const size_t unit = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
    Base64Stream stream(text);
    ByteBuffer buffer;

    if (compressed) {
        stream.DecodeTo(unit * 3, buffer);
        if (buffer.size() < unit * 3) { return false; }

        const uint64_t numBlocks = ReadHeaderField(buffer, 0, header8, bigEndian);
        if (numBlocks == 0) { return false; }

        const size_t headerBytes = unit * HeaderFieldCount(numBlocks);
        stream.DecodeTo(headerBytes, buffer);
        if (buffer.size() < headerBytes) { return false; }

        size_t totalBytes = headerBytes;
        for (uint64_t block = 0; block < numBlocks; ++block) {
            const size_t compressedSize = static_cast<size_t>(ReadHeaderField(buffer, 3 + block, header8, bigEndian));
            if (compressedSize == 0) { return false; }
            totalBytes += compressedSize;
        }
        stream.DecodeTo(totalBytes, buffer);
        if (buffer.size() < totalBytes) { return false; }

        return DecompressVTKZLib(buffer, header8, bigEndian, expectedBytes, payload);
    }

    stream.DecodeTo(unit, buffer);
    if (buffer.size() < unit) { return false; }
    const uint64_t byteNum = ReadHeaderField(buffer, 0, header8, bigEndian);
    // 字节数超过该数组应有的长度说明头部或 offset 有误，直接拒绝，避免越界访问
    if (byteNum == 0 || (expectedBytes != 0 && byteNum > expectedBytes)) { return false; }

    stream.DecodeTo(unit + byteNum, buffer);
    if (buffer.size() < unit + byteNum) { return false; }
    payload.assign(buffer.begin() + unit, buffer.begin() + unit + static_cast<size_t>(byteNum));
    return true;
}

/**
 * @brief 从 raw 文本（<AppendedData encoding="raw">）读取一个数据块。
 *
 * raw 数据直接嵌在 XML 文本里，长度必须由字节数头给出；"raw + 压缩"的组合暂不支持。
 */
bool ReadRawBlock(const char* text, bool header8, bool compressed, bool bigEndian, IGsize expectedBytes,
                  ByteBuffer& payload) {
    payload.clear();
    if (text == nullptr || compressed) { return false; }

    const size_t unit = header8 ? sizeof(uint64_t) : sizeof(uint32_t);
    const auto* bytes = reinterpret_cast<const unsigned char*>(text);
    const uint64_t byteNum = header8 ? ReadBinary<uint64_t>(bytes, bigEndian)
                                     : static_cast<uint64_t>(ReadBinary<uint32_t>(bytes, bigEndian));
    if (byteNum == 0 || expectedBytes == 0 || byteNum > expectedBytes) { return false; }

    payload.assign(bytes + unit, bytes + unit + static_cast<size_t>(byteNum));
    return true;
}

/* 把二进制数据按 T 追加到属性数组，最多追加 valueCount 个值（T 既是源元素也是目标元素类型）。 */
template<typename T>
void AppendBytesToFlatArray(const ByteBuffer& bytes, IGsize valueCount, bool bigEndian,
                            typename FlatArray<T>::Pointer arr) {
    if (arr == nullptr || bytes.empty()) { return; }
    IGsize count = bytes.size() / sizeof(T);
    if (valueCount != 0 && count > valueCount) { count = valueCount; }
    arr->Reserve(count / static_cast<IGsize>(std::max(1, arr->GetDimension())));
    for (IGsize i = 0; i < count; ++i) {
        arr->AddValue(ReadBinary<T>(bytes.data() + i * sizeof(T), bigEndian));
    }
}

/**
 * @brief 按「源元素宽度」读取整数并追加到目标整数数组。
 *
 * 目标容器按项目惯例选取（1/2/4 字节整数统一进 IntArray，8 字节进 LongLongArray），
 * 但**取值的宽度必须由 VTK 的 type 决定**，不能按目标容器宽度去读：Int8 的 1 若按 4 字节读，
 * 会变成 0x01010101 = 16843009，而且元素个数只有应有的 1/4。
 */
template<typename TDst>
void AppendRawIntegers(const ByteBuffer& bytes, IGsize valueCount, IGsize srcSize, bool srcUnsigned, bool bigEndian,
                       typename FlatArray<TDst>::Pointer arr) {
    if (arr == nullptr || srcSize == 0 || bytes.empty()) { return; }
    IGsize count = bytes.size() / srcSize;
    if (valueCount != 0 && count > valueCount) { count = valueCount; }
    arr->Reserve(count / static_cast<IGsize>(std::max(1, arr->GetDimension())));
    for (IGsize i = 0; i < count; ++i) {
        const unsigned char* p = bytes.data() + i * srcSize;
        long long value = 0;
        switch (srcSize) {
            case 1:
                value = srcUnsigned ? static_cast<long long>(p[0])
                                    : static_cast<long long>(static_cast<int8_t>(p[0]));
                break;
            case 2: {
                const uint16_t raw = ReadBinary<uint16_t>(p, bigEndian);
                value = srcUnsigned ? static_cast<long long>(raw)
                                    : static_cast<long long>(static_cast<int16_t>(raw));
                break;
            }
            case 4: {
                const uint32_t raw = ReadBinary<uint32_t>(p, bigEndian);
                value = srcUnsigned ? static_cast<long long>(raw)
                                    : static_cast<long long>(static_cast<int32_t>(raw));
                break;
            }
            default: {
                const uint64_t raw = ReadBinary<uint64_t>(p, bigEndian);
                value = static_cast<long long>(raw);
                break;
            }
        }
        arr->AddValue(static_cast<TDst>(value));
    }
}

/* 把 ascii 文本按浮点类型 T 追加到属性数组，最多追加 valueCount 个值。 */
template<typename T>
void AppendAsciiToFlatArray(const char* text, IGsize valueCount, typename FlatArray<T>::Pointer arr) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream stream(text);
    double value = 0.0;
    while ((valueCount == 0 || arr->GetNumberOfValues() < valueCount) && (stream >> value)) {
        arr->AddValue(static_cast<T>(value));
    }
}

/**
 * @brief 把 ascii 文本按整数类型直接解析后追加到属性数组，最多 valueCount 个值。
 *
 * 【不能复用读 double 的版本】double 只有 53 位尾数，ASCII 里的 64 位整数先读成 double
 * 再转回整数会丢精度：9007199254740993 会被读成 9007199254740992，ID / 整数标量被悄悄改值。
 * 这里按 srcUnsigned 选择 strtoll / strtoull 直接解析成 64 位整数，再窄化到目标类型。
 *
 * 解析到非整数记号时就停止：剩下的位置留给长度校验，数组不完整会被判为错误而不是静默截断。
 */
template<typename T>
void AppendAsciiIntegersToFlatArray(const char* text, IGsize valueCount, bool srcUnsigned,
                                    typename FlatArray<T>::Pointer arr) {
    if (text == nullptr || arr == nullptr) { return; }
    std::istringstream stream(text);
    std::string token;
    while ((valueCount == 0 || arr->GetNumberOfValues() < valueCount) && (stream >> token)) {
        char* end = nullptr;
        long long value = 0;
        errno = 0;
        if (srcUnsigned) {
            const unsigned long long parsed = std::strtoull(token.c_str(), &end, 10);
            if (end == token.c_str() || errno == ERANGE) { return; }
            value = static_cast<long long>(parsed);
        } else {
            value = std::strtoll(token.c_str(), &end, 10);
            if (end == token.c_str() || errno == ERANGE) { return; }
        }
        if (*end != '\0') { return; } // 不是纯整数记号：停止解析，由长度校验判为不完整
        arr->AddValue(static_cast<T>(value));
    }
}

/* VTK DataArray/@type 的字节宽度（同时兼容 XML 里的旧名字，如 char/short/unsigned_int/double）。 */
IGsize SizeOfVtkType(const char* type) {
    if (type == nullptr) { return 4; }
    if (std::strncmp(type, "Float64", 7) == 0 || std::strcmp(type, "double") == 0 ||
        std::strncmp(type, "Int64", 5) == 0 || std::strncmp(type, "UInt64", 6) == 0 ||
        std::strncmp(type, "long_long", 9) == 0 || std::strncmp(type, "unsigned_long_long", 18) == 0) {
        return 8;
    }
    if (std::strncmp(type, "Int16", 5) == 0 || std::strncmp(type, "UInt16", 6) == 0 ||
        std::strcmp(type, "short") == 0 || std::strcmp(type, "unsigned_short") == 0) {
        return 2;
    }
    if (std::strncmp(type, "Int8", 4) == 0 || std::strncmp(type, "UInt8", 5) == 0 ||
        std::strcmp(type, "char") == 0 || std::strcmp(type, "signed_char") == 0 ||
        std::strcmp(type, "unsigned_char") == 0) {
        return 1;
    }
    return 4; // Float32 / float / Int32 / UInt32 / int / unsigned_int
}

bool IsUnsignedVtkType(const char* type) {
    if (type == nullptr) { return false; }
    return std::strncmp(type, "UInt", 4) == 0 || std::strncmp(type, "unsigned", 8) == 0;
}

bool IsFloat64VtkType(const char* type) {
    if (type == nullptr) { return false; }
    return std::strncmp(type, "Float64", 7) == 0 || std::strcmp(type, "double") == 0;
}

bool IsFloatVtkType(const char* type) {
    if (type == nullptr) { return true; } // 缺省按 Float32 处理
    return std::strncmp(type, "Float", 5) == 0 || std::strcmp(type, "float") == 0;
}

/* 解析 "a,b,c" 形式的属性名列表。 */
void SplitNameList(const char* text, std::vector<std::string>& names) {
    if (text == nullptr) { return; }
    std::istringstream stream(text);
    std::string name;
    while (std::getline(stream, name, ',')) {
        const auto begin = name.find_first_not_of(" \t\n\r");
        if (begin == std::string::npos) { continue; }
        const auto end = name.find_last_not_of(" \t\n\r");
        names.push_back(name.substr(begin, end - begin + 1));
    }
}

bool ContainsName(const std::vector<std::string>& names, const std::string& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

/* 解析 6 个整数的范围字符串（WholeExtent / Extent）。 */
bool ParseExtent(const char* text, igIndex* extent) {
    if (text == nullptr) { return false; }
    std::istringstream stream(text);
    for (int i = 0; i < 6; ++i) {
        if (!(stream >> extent[i])) { return false; }
    }
    return true;
}

/* 解析 3 个浮点数的字符串（Origin / Spacing）。 */
bool ParseTriple(const char* text, double* values) {
    if (text == nullptr) { return false; }
    std::istringstream stream(text);
    for (int i = 0; i < 3; ++i) {
        if (!(stream >> values[i])) { return false; }
    }
    return true;
}
} // namespace

const char* iGameVTIReader::GetAppendedDataHead() {
    if (m_AppendedDataHead != nullptr) { return m_AppendedDataHead; }

    auto* elem = FindTargetItem(root, "AppendedData");
    if (elem == nullptr) { return nullptr; }

    const char* encoding = elem->Attribute("encoding");
    if (encoding != nullptr && std::strncmp(encoding, "raw", 3) == 0) { m_RawAppendedData = true; }

    const char* data = elem->GetText();
    if (data == nullptr) { return nullptr; }
    while (*data == '\n' || *data == ' ' || *data == '\t' || *data == '\r') { ++data; }
    if (*data == '_') { ++data; } // VTK 在数据区起始处写一个下划线占位

    m_AppendedDataHead = data;
    return m_AppendedDataHead;
}

bool iGameVTIReader::ReadImageDataInfo() {
    auto* elem = FindTargetItem(root, "ImageData");
    if (elem == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing ImageData node.");
        return false;
    }

    igIndex wholeExtent[6]{0, 0, 0, 0, 0, 0};
    if (!ParseExtent(elem->Attribute("WholeExtent"), wholeExtent)) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing or invalid WholeExtent attribute.");
        return false;
    }
    // Origin / Spacing 缺省值：原点 0、间距 1（VTK 默认）
    if (!ParseTriple(elem->Attribute("Origin"), m_Origin)) { m_Origin[0] = m_Origin[1] = m_Origin[2] = 0.0; }
    if (!ParseTriple(elem->Attribute("Spacing"), m_Spacing)) { m_Spacing[0] = m_Spacing[1] = m_Spacing[2] = 1.0; }

    // 只读取第一个 <Piece>；没有 Piece/Extent 时退化为 WholeExtent（单块文件）
    std::copy(wholeExtent, wholeExtent + 6, m_Extent);
    if (auto* piece = FindTargetItem(root, "Piece")) {
        igIndex pieceExtent[6]{0, 0, 0, 0, 0, 0};
        if (ParseExtent(piece->Attribute("Extent"), pieceExtent)) {
            std::copy(pieceExtent, pieceExtent + 6, m_Extent);
        }
    }

    for (int axis = 0; axis < 3; ++axis) {
        m_PointSize[axis] = m_Extent[2 * axis + 1] - m_Extent[2 * axis] + 1;
        if (m_PointSize[axis] < 1) { m_PointSize[axis] = 1; }
    }
    m_PointNum = static_cast<IGsize>(m_PointSize[0]) * static_cast<IGsize>(m_PointSize[1]) *
                 static_cast<IGsize>(m_PointSize[2]);

    // 单元数：2D（size[2] == 1）时是四边形面数，3D 时是六面体数
    const IGsize cellSize[3]{static_cast<IGsize>(std::max(0, m_PointSize[0] - 1)),
                             static_cast<IGsize>(std::max(0, m_PointSize[1] - 1)),
                             static_cast<IGsize>(std::max(0, m_PointSize[2] - 1))};
    m_CellNum = cellSize[0] * cellSize[1] * (m_PointSize[2] > 1 ? cellSize[2] : 1);

    m_Data.dimensionSize[0] = m_PointSize[0];
    m_Data.dimensionSize[1] = m_PointSize[1];
    m_Data.dimensionSize[2] = m_PointSize[2];
    return true;
}

bool iGameVTIReader::BuildPoints() {
    Points::Pointer points = m_Data.GetPoints();
    if (points == nullptr) { return false; }

    points->Reserve(m_PointNum);
    // 隐含坐标：coordinate = Origin + (Extent 起点 + i) * Spacing
    const double start[3]{m_Origin[0] + static_cast<double>(m_Extent[0]) * m_Spacing[0],
                          m_Origin[1] + static_cast<double>(m_Extent[2]) * m_Spacing[1],
                          m_Origin[2] + static_cast<double>(m_Extent[4]) * m_Spacing[2]};
    for (igIndex k = 0; k < m_PointSize[2]; ++k) {
        const double z = start[2] + static_cast<double>(k) * m_Spacing[2];
        for (igIndex j = 0; j < m_PointSize[1]; ++j) {
            const double y = start[1] + static_cast<double>(j) * m_Spacing[1];
            for (igIndex i = 0; i < m_PointSize[0]; ++i) {
                const double x = start[0] + static_cast<double>(i) * m_Spacing[0];
                points->AddPoint(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
            }
        }
    }
    return points->GetNumberOfPoints() == m_PointNum;
}

bool iGameVTIReader::ReadOneArray(tinyxml2::XMLElement* elem, IGenum attachmentType, IGsize tupleNum) {
    const char* data = elem->Attribute("Name");
    const std::string name = data != nullptr ? data : "Undefined Scalar";
    const char* type = elem->Attribute("type");
    const char* format = elem->Attribute("format");
    data = elem->Attribute("NumberOfComponents");
    const int components = data != nullptr ? std::max(1, std::atoi(data)) : 1;

    const bool ascii = (format == nullptr) || (std::strcmp(format, "ascii") == 0);
    const bool binary = (format != nullptr) && (std::strcmp(format, "binary") == 0);
    const bool appended = (format != nullptr) && (std::strcmp(format, "appended") == 0);
    if (!ascii && !binary && !appended) {
        IGAME_CORE_ERROR("[iGameVTIReader] Unsupported format '{}' of array '{}'.", format, name);
        return false;
    }

    // 该段没有元组时（例如无单元的网格同样带 <CellData> 节点）没有任何数据需要读取
    if (tupleNum == 0) { return true; }

    // 元组数 × 分量数 = 数组应有的值个数。先做溢出检查，避免 NumberOfComponents 异常时
    // 乘积回绕，把后面的长度校验绕过去。
    const IGsize maxSize = std::numeric_limits<IGsize>::max();
    if (static_cast<IGsize>(components) > maxSize / tupleNum) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' declares too many components ({}).", name, components);
        return false;
    }
    const IGsize valueNum = tupleNum * static_cast<IGsize>(components);

    // 源元素宽度/符号性由 VTK 的 type 决定（Int8 = 1 字节/元素）；容器类型另按项目惯例选，
    // 两者不能混为一谈，否则 Int8 的 1 会被按 4 字节读成 0x01010101 = 16843009。
    const IGsize srcSize = SizeOfVtkType(type);
    const bool srcUnsigned = IsUnsignedVtkType(type);

    if (valueNum > maxSize / srcSize) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' is too large to decode.", name);
        return false;
    }
    const IGsize byteNum = valueNum * srcSize;

    ByteBuffer bytes;
    if (!ascii) {
        bool ok = false;
        if (binary) {
            ok = ReadBase64Block(elem->GetText(), m_Header_8_byte_flag, m_Compressed, m_BigEndian, byteNum, bytes);
        } else {
            const char* offsetText = elem->Attribute("offset");
            const char* head = GetAppendedDataHead();
            if (offsetText != nullptr && head != nullptr) {
                const char* block = head + std::atoll(offsetText);
                ok = m_RawAppendedData
                             ? ReadRawBlock(block, m_Header_8_byte_flag, m_Compressed, m_BigEndian, byteNum, bytes)
                             : ReadBase64Block(block, m_Header_8_byte_flag, m_Compressed, m_BigEndian, byteNum, bytes);
            }
        }
        if (!ok) {
            IGAME_CORE_ERROR("[iGameVTIReader] Failed to decode array '{}' (format='{}', type='{}').", name,
                             format != nullptr ? format : "ascii", type != nullptr ? type : "Float32");
            return false;
        }
        // 解码出的字节数必须与 DataArray 声明的元组数完全一致：字节数偏少时数组不完整，
        // 若只告警继续，后续滤波会按点索引读到不存在的元素。
        if (bytes.size() != byteNum) {
            IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' decoded {} byte(s), expected {}; the data is incomplete.",
                             name, bytes.size(), byteNum);
            return false;
        }
    }

    // 容器类型按项目惯例选择：浮点 → Float/Double，8 字节整数 → LongLong，其余整数 → Int；
    // 取值时统一按上面的 srcSize/srcUnsigned 逐个元素读取。
    ArrayObject::Pointer array;
    if (IsFloat64VtkType(type)) {
        DoubleArray::Pointer typed = DoubleArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiToFlatArray<double>(elem->GetText(), valueNum, typed)
              : AppendBytesToFlatArray<double>(bytes, valueNum, m_BigEndian, typed);
        array = typed;
    } else if (IsFloatVtkType(type)) {
        FloatArray::Pointer typed = FloatArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiToFlatArray<float>(elem->GetText(), valueNum, typed)
              : AppendBytesToFlatArray<float>(bytes, valueNum, m_BigEndian, typed);
        array = typed;
    } else if (srcSize == 8) {
        LongLongArray::Pointer typed = LongLongArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiIntegersToFlatArray<long long>(elem->GetText(), valueNum, srcUnsigned, typed)
              : AppendRawIntegers<long long>(bytes, valueNum, srcSize, srcUnsigned, m_BigEndian, typed);
        array = typed;
    } else {
        IntArray::Pointer typed = IntArray::New();
        typed->SetDimension(components);
        ascii ? AppendAsciiIntegersToFlatArray<int>(elem->GetText(), valueNum, srcUnsigned, typed)
              : AppendRawIntegers<int>(bytes, valueNum, srcSize, srcUnsigned, m_BigEndian, typed);
        array = typed;
    }

    // 长度校验：值个数必须与元组数 × 分量数完全一致，不完整的数据一律拒绝，
    // 不允许“告警后仍然读取成功”。
    if (array == nullptr || array->GetNumberOfValues() != valueNum) {
        IGAME_CORE_ERROR("[iGameVTIReader] Array '{}' has {} value(s), expected {}; the data is incomplete.", name,
                         array != nullptr ? array->GetNumberOfValues() : 0, valueNum);
        return false;
    }

    array->SetName(name);
    // PointData/CellData 上的 Tensors/Vectors/Normals 决定属性类型，其余按标量处理
    IGenum attributeType = IG_SCALAR;
    if (ContainsName(m_TensorNames, name)) {
        attributeType = IG_TENSOR;
    } else if (ContainsName(m_VectorNames, name)) {
        attributeType = IG_VECTOR;
    } else if (ContainsName(m_NormalNames, name)) {
        attributeType = IG_NORMAL;
    }
    // 不传 dataRange：AttributeSet 内部按需懒计算（与 iGameVTPReader/iGameVTUReader 一致）
    m_Data.GetData()->AddAttribute(attributeType, attachmentType, array);
    return true;
}

bool iGameVTIReader::ReadAttributes(tinyxml2::XMLElement* section, IGenum attachmentType, IGsize tupleNum) {
    if (section == nullptr) { return true; }

    m_TensorNames.clear();
    m_VectorNames.clear();
    m_NormalNames.clear();
    SplitNameList(section->Attribute("Tensors"), m_TensorNames);
    SplitNameList(section->Attribute("Vectors"), m_VectorNames);
    SplitNameList(section->Attribute("Normals"), m_NormalNames);

    for (auto* elem = section->FirstChildElement("DataArray"); elem != nullptr;
         elem = elem->NextSiblingElement("DataArray")) {
        // 解码失败 / 长度不足必须向上传递：静默跳过会让“读取成功但缺数据”的文件混过去
        if (!ReadOneArray(elem, attachmentType, tupleNum)) { return false; }
    }
    return true;
}

bool iGameVTIReader::Parsing() {
    const char* attribute = root->Attribute("header_type");
    if (attribute != nullptr && std::strcmp(attribute, "UInt64") == 0) { m_Header_8_byte_flag = true; }
    attribute = root->Attribute("compressor");
    if (attribute != nullptr && std::strncmp(attribute, "vtkZLibDataCompressor", 21) == 0) { m_Compressed = true; }
    // 字节序：缺省 LittleEndian；BigEndian 时头部字段与数组元素都要翻转字节，
    // 否则会被按本机字节序误读（头部解析出错 -> 数组被丢弃，却仍然报告读取成功）
    attribute = root->Attribute("byte_order");
    if (attribute != nullptr) {
        if (std::strcmp(attribute, "BigEndian") == 0) {
            m_BigEndian = true;
        } else if (std::strcmp(attribute, "LittleEndian") != 0) {
            IGAME_CORE_ERROR("[iGameVTIReader] Unsupported byte_order '{}'.", attribute);
            return false;
        }
    }

    if (!ReadImageDataInfo()) { return false; }
    UpdateProgress(0.1);

    if (!BuildPoints()) {
        IGAME_CORE_ERROR("[iGameVTIReader] Failed to build implicit ImageData points.");
        return false;
    }
    UpdateProgress(0.3);

    auto* piece = FindTargetItem(root, "Piece");
    if (piece == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Missing Piece node.");
        return false;
    }
    if (!ReadAttributes(FindTargetItem(piece, "PointData"), IG_POINT, m_PointNum)) { return false; }
    UpdateProgress(0.7);
    if (!ReadAttributes(FindTargetItem(piece, "CellData"), IG_CELL, m_CellNum)) { return false; }
    UpdateProgress(1.0);
    return true;
}

bool iGameVTIReader::CreateDataObject() {
    // ImageData 是规则网格：用 StructuredMesh 承载（隐含坐标已生成为 Points）
    m_Output = StructuredMesh::New();
    auto structuredMesh = DynamicCast<StructuredMesh>(m_Output);
    if (structuredMesh == nullptr) {
        IGAME_CORE_ERROR("[iGameVTIReader] Failed to create StructuredMesh.");
        return false;
    }

    structuredMesh->SetExtent(m_Extent);
    structuredMesh->SetDimensionSize(m_Data.dimensionSize);
    structuredMesh->SetPoints(m_Data.GetPoints());
    structuredMesh->SetAttributeSet(m_Data.GetData());
    structuredMesh->GenStructuredCellConnectivities();
    return true;
}

IGAME_NAMESPACE_END
