#ifndef iGameAppendLocationAttribute_h
#define iGameAppendLocationAttribute_h
#include "iGameFilter.h"
#include <iGamePoints.h>
#include "iGameSurfaceMesh.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"
#include <vector>
IGAME_NAMESPACE_BEGIN

class AppendLocationAttribute : public Filter{
public:
    I_OBJECT(AppendLocationAttribute);
    static Pointer New() { return new AppendLocationAttribute; }
    std::vector<Point> AttributePoint; 
    

    void SetAttributeByIndex(int index) { curIndex = index; }
    void SetAttributeByName(const std::string& name) { this->name = name; }

    bool Execute() override;
    std::string GetMessage() const { return m_Message; }

private:

    bool AppendLocation(DataObject::Pointer Mesh, AttributeSet* attributeSet, int Index);
    bool AppendLocationSurface(SurfaceMesh::Pointer Mesh, AttributeSet* attributeSet, int Index);
    bool AppendLocationVolume(VolumeMesh::Pointer Mesh, AttributeSet* attributeSet, int Index);
    bool AppendLocationUnstructured(UnstructuredMesh::Pointer Mesh, AttributeSet* attributes, int Index);

protected:
    AppendLocationAttribute() { 
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~AppendLocationAttribute() override = default;
    SurfaceMesh::Pointer surface_Mesh{};
    VolumeMesh::Pointer volume_Mesh{};
    AttributeSet* attributeSet{nullptr};

    int curIndex{-1};
    std::string name;

    int dim{-1};
    int m_currentAttributeDimension{-1};

    std::string m_Message{"Not Surface Mesh!"};
};




IGAME_NAMESPACE_END
#endif