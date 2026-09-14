#pragma once

#ifndef EOIGAME_IGAMECORE_ATTRIBUTE_IGAMEEXTRACTCOMPONENTFILTER_H

#define EOIGAME_IGAMECORE_ATTRIBUTE_IGAMEEXTRACTCOMPONENTFILTER_H



#include <iGameDataObject.h>

#include <iGameFilter.h>

#include <string>



IGAME_NAMESPACE_BEGIN

class ExtractComponentFilter : public Filter {

public:

    I_OBJECT(ExtractComponentFilter)



    static Pointer New() { return new ExtractComponentFilter; }



    bool Execute() override;

    // 输入数组名称，为空时使用属性集中第一个向量属性（对应 DIME「可选」）

    void SetInputArrayName(const std::string& name) { m_InputArrayName = name; }

    const std::string& GetInputArrayName() const { return m_InputArrayName; }

    // Input array attachment restriction (IG_POINT / IG_CELL); IG_NONE means no restriction.
    // Used to disambiguate same-named arrays attached to both PointData and CellData.

    void SetInputAttachmentType(IGenum type) { m_InputAttachmentType = type; }

    IGenum GetInputAttachmentType() const { return m_InputAttachmentType; }
    // 输出标量数组名称，默认 "Result"

    void SetOutputArrayName(const std::string& name) { m_OutputArrayName = name; }

    const std::string& GetOutputArrayName() const { return m_OutputArrayName; }



    // 要提取的分量索引，0 = X, 1 = Y, 2 = Z

    void SetComponent(int comp) { m_Component = comp; }

    int GetComponent() const { return m_Component; }



    std::string GetMessage() const { return m_Message; }



protected:

    ExtractComponentFilter();

    ~ExtractComponentFilter() override = default;



    std::string m_InputArrayName{};

    std::string m_OutputArrayName{"Result"};

    int m_Component{0};

    IGenum m_InputAttachmentType{IG_NONE};

    std::string m_Message{"ExtractComponent 执行失败"};

};

IGAME_NAMESPACE_END

#endif

