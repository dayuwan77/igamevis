/**
 * @class   iGameVTIReader
 * @brief   VTK XML ImageData(.vti) 读取器。
 *
 *          ImageData 的点坐标是隐含的（Origin + Spacing ×(i,j,k)），因此本读取器直接构造
 *          结构化网格 StructuredMesh：WholeExtent/Piece Extent → 维度，Origin/Spacing → 点坐标，
 *          PointData / CellData 中的每个 <DataArray> → 一条属性（IG_TENSOR / IG_VECTOR /
 *          IG_NORMAL / IG_SCALAR）。
 *
 *          支持的数据编码：ascii、binary(base64 内联)、appended + base64、appended + raw(未压缩)；
 *          compressor="vtkZLibDataCompressor" 时按 VTK 的压缩块格式解压。
 */
#ifndef iGameVTIReader_h
#define iGameVTIReader_h

#include "XML/iGameXMLFileReader.h"

#include <string>
#include <vector>

namespace tinyxml2 {
    class XMLElement;
}

IGAME_NAMESPACE_BEGIN

class iGameVTIReader : public iGameXMLFileReader {
public:
    I_OBJECT(iGameVTIReader)

    static Pointer New() { return new iGameVTIReader; }

    bool Parsing() override;

    bool CreateDataObject() override;

protected:
    iGameVTIReader() = default;
    ~iGameVTIReader() override = default;

private:
    /* 读取 <ImageData> 的 WholeExtent/Origin/Spacing 与 <Piece> 的 Extent。 */
    bool ReadImageDataInfo();

    /* 由 Origin/Spacing/Extent 生成隐含点坐标。 */
    bool BuildPoints();

    /* 读取一个 <PointData> 或 <CellData> 段下的所有 <DataArray>。 */
    bool ReadAttributes(tinyxml2::XMLElement* section, IGenum attachmentType, IGsize tupleNum);

    /* 读取单个 <DataArray>，按名字判定属性类型后加入 AttributeSet。 */
    bool ReadOneArray(tinyxml2::XMLElement* elem, IGenum attachmentType, IGsize tupleNum);

    /* <AppendedData> 中数据区起始指针（跳过 '_'）。 */
    const char* GetAppendedDataHead();

private:
    igIndex m_PointSize[3]{1, 1, 1};         // 各方向点数（Piece/WholeExtent）
    igIndex m_Extent[6]{0, 0, 0, 0, 0, 0};   // Piece Extent，用于隐含坐标的起点偏移
    double m_Origin[3]{0.0, 0.0, 0.0};
    double m_Spacing[3]{1.0, 1.0, 1.0};
    IGsize m_PointNum{0};
    IGsize m_CellNum{0};

    bool m_Header_8_byte_flag{false};   // VTKFile/@header_type == "UInt64"
    bool m_Compressed{false};           // 存在 VTKFile/@compressor
    bool m_BigEndian{false};            // VTKFile/@byte_order == "BigEndian"
    bool m_RawAppendedData{false};      // <AppendedData encoding="raw">
    const char* m_AppendedDataHead{nullptr};

    // <PointData>/<CellData> 上的 Tensors/Normals/Vectors 名字列表（决定属性类型）
    std::vector<std::string> m_TensorNames;
    std::vector<std::string> m_VectorNames;
    std::vector<std::string> m_NormalNames;
};

IGAME_NAMESPACE_END
#endif
