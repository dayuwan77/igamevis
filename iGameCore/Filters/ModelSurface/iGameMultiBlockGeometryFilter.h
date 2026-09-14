#ifndef iGameMultiBlockGeometryFilter_h
#define iGameMultiBlockGeometryFilter_h

#include "iGameFilter.h"
#include "iGameSurfaceMesh.h"
#include "iGameModelGeometryFilter.h"

IGAME_NAMESPACE_BEGIN

class MultiBlockGeometryFilter : public Filter{
public:
    I_OBJECT(MultiBlockGeometryFilter);
    static MultiBlockGeometryFilter::Pointer New() {return new MultiBlockGeometryFilter;}
    bool Execute() override;
    bool Execute(DataObject::Pointer);
    bool Execute(DataObject::Pointer, DataObject::Pointer&);

protected:
    DataObject::Pointer input;
    DataObject::Pointer output;
    
    MultiBlockGeometryFilter();
    ~MultiBlockGeometryFilter() override =default;

    bool ExtractRecursively(DataObject::Pointer, DataObject::Pointer&);
};



IGAME_NAMESPACE_END
#endif