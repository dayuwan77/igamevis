#ifndef iGamePassArraysFilter_h
#define iGamePassArraysFilter_h

#include "iGameDataObject.h"
#include "iGameFilter.h"
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class PassArrays : public Filter {
public:
    I_OBJECT(PassArrays);
    static Pointer New() { return new PassArrays; }

    // 设置需要过滤掉的属性名称列表（点/单元共用）
    void SetArrayNames(const std::vector<std::string>& names) { m_ArrayNames = names; }

    bool Execute() override;

protected:
    PassArrays() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~PassArrays() override = default;

private:
    std::vector<std::string> m_ArrayNames;
};

IGAME_NAMESPACE_END

#endif