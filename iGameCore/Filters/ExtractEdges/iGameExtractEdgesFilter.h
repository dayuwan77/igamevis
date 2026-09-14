#ifndef iGameExtractEdgesFilter_h
#define iGameExtractEdgesFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

#include <set>      // 边去重用的红黑树集合
#include <utility>  // std::pair / std::minmax

IGAME_NAMESPACE_BEGIN


class ExtractEdgesFilter : public Filter {

public:
    I_OBJECT(ExtractEdgesFilter);
    static Pointer New() { return new ExtractEdgesFilter; }

    bool Execute() override;

    UnstructuredMesh::Pointer GetEdgesMesh() {
        return DynamicCast<UnstructuredMesh>(this->GetOutput());
    }

protected:
    ExtractEdgesFilter();


    bool ExecuteWithPointSet(DataObject::Pointer input);

    bool ExtractEdgesFromMesh(UnstructuredMesh::Pointer input,
                              UnstructuredMesh::Pointer output);
};

IGAME_NAMESPACE_END
#endif
