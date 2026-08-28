#include "iGameDeflectNormalsFilter.h"

IGAME_NAMESPACE_BEGIN

DeflectNormalsFilter::DeflectNormalsFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    ifUseUserNormal = false;
    strength = 1.0f;
    userNormal = Vector3f(0.f, 0.f, 1.f);
}

DeflectNormalsFilter::~DeflectNormalsFilter() {}

ArrayObject::Pointer DeflectNormalsFilter::AttributeCell2Point(CellArray::Pointer cells, ArrayObject::Pointer ori, size_t pointNum) 
{        
    int dim = ori->GetDimension();                                 
    auto newArr = FloatArray::New();                               
    newArr->SetName(ori->GetName());                              
    newArr->SetDimension(dim);                                     
    newArr->Reserve(pointNum);                                     


    float s[16] = {0}, t[16] = {0};
    for (size_t i = 0; i < pointNum; ++i) newArr->AddElement(s);


    std::vector<int> adj(pointNum, 0); 
    igIndex cell[IGAME_CELL_MAX_SIZE];
    for (int i = 0; i < cells->GetNumberOfCells(); ++i) 
    {
        int size = cells->GetCellIds(i, cell); 
        ori->GetElement(i, s);                 
        for (int j = 0; j < size; ++j) 
        {
            adj[cell[j]]++;                             
            newArr->GetElement(cell[j], t);             
            for (int d = 0; d < dim; ++d) 
                t[d] += s[d]; 
            newArr->SetElement(cell[j], t);             
        }
    }
    
    for (size_t i = 0; i < pointNum; ++i) 
    {
        if (adj[i] > 0) 
        { // 防止除 0（孤立点没被任何单元包含）
            newArr->GetElement(i, t);
            for (int d = 0; d < dim; ++d) 
                t[d] /= adj[i];
            newArr->SetElement(i, t);
        }
    }
    return newArr;
}

bool DeflectNormalsFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) return false;


    // 拿到 SurfaceMesh
    SurfaceMesh::Pointer mesh;
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            mesh = DynamicCast<SurfaceMesh>(input);
            break;
        case IG_UNSTRUCTURED_MESH: 
        {
            auto um = DynamicCast<UnstructuredMesh>(input);
            // 先尝试 TransferToSurfaceMesh（纯面网格）
            mesh = um->TransferToSurfaceMesh();
            // 如果失败（体网格），尝试提取外表面
            if (mesh == nullptr) 
                mesh = um->ExtractSurfaceMesh();
            break;
        }
        default:
            errorMessage = "Not Surface Mesh!"; // 记录错误原因
            return false;
    }
    if (mesh == nullptr) return false;

    //构建"点→邻接面"索引。
    mesh->BuildFaceLinks();

    //为点法向量创建一个新的 FloatArray
    int numPts = mesh->GetNumberOfPoints(); 
    FloatArray::Pointer result = FloatArray::New();
    result->SetDimension(3);             
    result->Reserve(numPts);             
    result->SetName("DeflectedNormals"); 

    //计算点法向量
    igIndex faceIds[256];
    for (int ptId = 0; ptId < numPts; ++ptId)
    {
        Vector3f base(0.f, 0.f, 0.f);
        if (ifUseUserNormal)
        { 
            base = userNormal;
        } 
        else
        {
            int n = mesh->GetPointToNeighborFaces(ptId, faceIds); 
            for (int j = 0; j < n; ++j) 
            {
                Face* f = mesh->GetFace(faceIds[j]); 
                if (f) base += f->GetNormal();       
            }
        }
        
        base.normalize();
        float out[3] = {base[0], base[1], base[2]};
        result->AddElement(out); 
    }


    auto attrSet = mesh->GetAttributeSet();     // 取属性集
    if (attrSet == nullptr) 
    {
        errorMessage = "please choose a attribute";
        return false;
    }
    if (curIndex == -1 && name == "")           // 既没指定索引也没指定名字 → 用户还没选属性
    {
        errorMessage = "please choose a attribute";
        return false;
    }
    if (curIndex == -1)                         // 如果只给了名字，就按名字查索引
        curIndex = attrSet->GetAttributeIndex(name);    
    if (curIndex < 0 || curIndex >= attrSet->GetNumberOfAttributes()) // 索引越界 → 属性不存在
    {
        errorMessage = "please choose a attribute";
        return false;
    }


    auto& attr = attrSet->GetAttribute(curIndex); 
    ArrayObject::Pointer vecField = attr.pointer; 
    if (vecField->GetDimension() != 3)
    {
        errorMessage = "must be a vector (dim=3)";
        return false;
    }


    if (attr.attachmentType == IG_CELL)
        vecField = AttributeCell2Point(mesh->GetFaces(), vecField, mesh->GetNumberOfPoints());

    for (int ptId = 0; ptId < numPts; ++ptId)
    {
        //读取当前点的向量场向量 V
        float buf[3] = {0, 0, 0};
        result->GetElement(ptId, buf);
        Vector3f base(buf[0], buf[1], buf[2]);

        vecField->GetElement(ptId, buf); 
        Vector3f V(buf[0], buf[1], buf[2]); 

        Vector3f ND = base + V * float(strength);
        ND.normalize();

        float out[3] = {ND[0], ND[1], ND[2]};
        result->SetElement(ptId, out); 
    }


    // (8) 注册新向量属性并刷新渲染
    attrSet->AddVector(IG_POINT, result);    
    attrSet->ForceReConvertToDrawableData();
    SetOutput(mesh);                         
    UpdateProgress(1.0);

    return true;
}

IGAME_NAMESPACE_END
