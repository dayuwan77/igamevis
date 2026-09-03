#pragma once
#include <iGameFilter.h>
#include <iGameDataObject.h>

IGAME_NAMESPACE_BEGIN

class iGameGenerateIdsFilter : public Filter {
public:
    I_OBJECT(iGameGenerateIdsFilter);
    static Pointer New(IGenum dataType) { return new iGameGenerateIdsFilter(dataType); }

    bool Execute() override;

    void SetArrayName(const std::string& name) { m_ArrayName = name; }
    void SetStartId(long long start) { m_StartId = start; }

private:
    bool Run();

protected:
    iGameGenerateIdsFilter(IGenum dataType);
    ~iGameGenerateIdsFilter() override = default;

private:
    IGenum m_DataType{}; // IG_POINT 或 IG_CELL

    std::string m_ArrayName{"Ids"};
    long long m_StartId{0};
};

IGAME_NAMESPACE_END
