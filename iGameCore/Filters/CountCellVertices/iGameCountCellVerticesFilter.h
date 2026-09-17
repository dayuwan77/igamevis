#ifndef iGameCountCellVerticesFilter_h
#define iGameCountCellVerticesFilter_h

#include "iGameFilter.h"
#include "iGamePointSet.h"

IGAME_NAMESPACE_BEGIN
class CountCellVerticesFilter : public Filter {

public:
    I_OBJECT(CountCellVerticesFilter);
    static Pointer New() { return new CountCellVerticesFilter; }
    bool Execute() override;

protected:
    CountCellVerticesFilter();
    ~CountCellVerticesFilter() override = default;
};

IGAME_NAMESPACE_END
#endif // iGameCountCellVerticesFilter_h
