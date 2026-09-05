#include "iGameAppendLocationAttribute.h"
IGAME_NAMESPACE_BEGIN
bool AppendLocationAttribute::Execute() { 
	auto input = GetInput(0);
	if (input == nullptr) return false;
    //判断是否是面
    auto CheckType = [&]() -> bool {
        attributeSet = input->GetAttributeSet();
        if (curIndex == -1 && name == "") {
            return true;
        }
        else {
            m_Message = "please choose mesh";
            return false;
        }
    };
    if (!CheckType()) return false;
    //attributeSet = input->GetAttributeSet();
    return AppendLocation(input, attributeSet, curIndex);
    

    //根据面进行不同的操作
    //switch (input->GetDataObjectType()) {
    //    case IG_SURFACE_MESH: {
    //        surface_Mesh = DynamicCast<SurfaceMesh>(input);
    //        if (!CheckType()) return false;
    //        return AppendLocationSurface(surface_Mesh, attributeSet, curIndex);
    //    } break;
    //    case IG_VOLUME_MESH: {
    //        return false;
    //        // volume_Mesh = DynamicCast<VolumeMesh>(input);
    //        // if (!CheckType()) return false;
    //        // return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);

    //    } break;
    //    case IG_STRUCTURED_MESH: {
    //        return false;
    //        // volume_Mesh = DynamicCast<VolumeMesh>(input);
    //        // if (!CheckType()) return false;
    //        // return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);

    //    } break;
    //    case IG_UNSTRUCTURED_MESH: {
    //        auto mesh = DynamicCast<UnstructuredMesh>(input);
    //        surface_Mesh = mesh->TransferToSurfaceMesh();
    //        volume_Mesh = mesh->TransferToVolumeMesh();

    //        if (surface_Mesh) {
    //            if (!CheckType()) return false;
    //            return AppendLocationUnstructured(mesh, attributeSet, curIndex);
    //        }

    //        if (volume_Mesh) {
    //            return false;
    //            // if (!CheckType()) return false;
    //            // return ComputeGradientWithVolumeMesh(volume_Mesh, attributeSet, curIndex);
    //        }
    //    } break;
    //    default:
    //        return false;
    //}



}

bool AppendLocationAttribute::AppendLocation(DataObject::Pointer Mesh, AttributeSet* attributeSet, int Index) {
    auto Points = Mesh->GetPoints();
    auto NumPoints = Points->GetNumberOfPoints();
    //auto attributePosition = attributeSet->GetNumberOfAttributes();
    //std::cout << "yunxingdaozhe1" << std::endl;
    if (Points == nullptr) { m_Message = "Points为空"; }

    FloatArray::Pointer point = FloatArray::New();
    point->SetDimension(3);
    point->Resize(NumPoints);
    point->SetName("LocationAttribute");

    //将所有点转化为FloatArray类
    //遍历所有点
    //分别遍历x,y,z
    //添加进度条

    int blockNum = NumPoints / 100, progress = 0;
    for (int i = 0; i < NumPoints; i++) {
        float x = Points->GetPoint(i)[0];
        float y = Points->GetPoint(i)[1];
        float z = Points->GetPoint(i)[2];
        float value[3]{x, y, z};
        point->SetElement(i, value);

        if (i >= blockNum * progress) {
            UpdateProgress(progress * 0.01);
            progress++;
        }
    }
    ResetProgress();
    //通过attributeSet加属性
    attributeSet->AddVector(IG_POINT, point);
    //通过位置加属性
    /*auto attr = attributeSet->GetAttribute(attributePosition);
    attr.pointer = point;
    attr.attachmentType = IG_POINT;*/
    
    //std::cout << "yunxingdaozhe3" << std::endl;
    SetOutput(Mesh);
    return true;
}

bool AppendLocationAttribute::AppendLocationSurface(SurfaceMesh::Pointer Mesh, AttributeSet* attributeSet, int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    auto Points = Mesh->GetPoints();

    if (Points == nullptr) { m_Message = "Points为空"; }
    FloatArray::Pointer point = FloatArray::New();

    point->SetDimension(3);
    point->Resize(NumPoints);
    point->SetName("LocationAttribute");


    //将所有点转化为FloatArray类
    //遍历所有点
    //分别遍历x,y,z
    //添加进度条

    int blockNum = NumPoints / 100, progress = 0;
    for (int i = 0; i < NumPoints; i++) {
        float x = Points->GetPoint(i)[0];
        float y = Points->GetPoint(i)[1];
        float z = Points->GetPoint(i)[2];
        float value[3]{x, y, z};
        point->SetElement(i, value);

        if (i >= blockNum * progress) {
            UpdateProgress(progress * 0.01);
            progress++;
        }
    }
   
    ResetProgress();
    //通过attributeSet加属性

    attributeSet->AddVector(IG_POINT, point);
    //通过位置加属性
    /*auto attr = attributeSet->GetAttribute(attributePosition);
    attr.pointer = point;
    attr.attachmentType = IG_POINT;*/

    //std::cout << "yunxingdaozhe3" << std::endl;
    SetOutput(Mesh);
    return true;

}
bool AppendLocationAttribute::AppendLocationVolume(VolumeMesh::Pointer Mesh, AttributeSet* attributeSet, int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    auto Points = Mesh->GetPoints();

    if (Points == nullptr) { m_Message = "Points为空"; }
    FloatArray::Pointer point = FloatArray::New();

    point->SetDimension(3);
    point->Resize(NumPoints);
    point->SetName("LocationAttribute");


    //将所有点转化为FloatArray类
    //遍历所有点
    //分别遍历x,y,z
    //添加进度条

    int blockNum = NumPoints / 100, progress = 0;
    for (int i = 0; i < NumPoints; i++) {
        float x = Points->GetPoint(i)[0];
        float y = Points->GetPoint(i)[1];
        float z = Points->GetPoint(i)[2];
        float value[3]{x, y, z};
        point->SetElement(i, value);

        if (i >= blockNum * progress) {
            UpdateProgress(progress * 0.01);
            progress++;
        }
    }
    
    ResetProgress();
    //通过attributeSet加属性

    attributeSet->AddVector(IG_POINT, point);
    //通过位置加属性
    /*auto attr = attributeSet->GetAttribute(attributePosition);
    attr.pointer = point;
    attr.attachmentType = IG_POINT;*/

    //std::cout << "yunxingdaozhe3" << std::endl;
    SetOutput(Mesh);
    return true;
}
bool AppendLocationAttribute::AppendLocationUnstructured(UnstructuredMesh::Pointer Mesh, AttributeSet* attributeSet, int Index) {
    int NumPoints = Mesh->GetNumberOfPoints();
    auto Points = Mesh->GetPoints();

    if (Points == nullptr) { m_Message = "Points为空"; }
    FloatArray::Pointer point = FloatArray::New();

    point->SetDimension(3);
    point->Resize(NumPoints);
    point->SetName("LocationAttribute");


    //将所有点转化为FloatArray类
    //遍历所有点
    //分别遍历x,y,z
    //添加进度条

    int blockNum = NumPoints / 100, progress = 0;
    for (int i = 0; i < NumPoints; i++) {
        float x = Points->GetPoint(i)[0];
        float y = Points->GetPoint(i)[1];
        float z = Points->GetPoint(i)[2];
        float value[3]{x, y, z};
        point->SetElement(i, value);

        if (i >= blockNum * progress) {
            UpdateProgress(progress * 0.01);
            progress++;
        }
    }
   
    ResetProgress();
    //通过attributeSet加属性

    attributeSet->AddVector(IG_POINT, point);
    //通过位置加属性
    /*auto attr = attributeSet->GetAttribute(attributePosition);
    attr.pointer = point;
    attr.attachmentType = IG_POINT;*/

    //std::cout << "yunxingdaozhe3" << std::endl;
    SetOutput(Mesh);
    return true;
}
IGAME_NAMESPACE_END