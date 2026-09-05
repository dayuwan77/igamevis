#include "iGameTransformFilter.h"

#include <cmath>

IGAME_NAMESPACE_BEGIN

TransformFilter::TransformFilter(){
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);

    // 默认使用单位矩阵
    for (int i = 0; i < 4; ++i){
        for (int j = 0; j < 4; ++j){
            m_Matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}


void TransformFilter::SetTranslation(float tx,float ty,float tz){
    m_TranslateX = tx;
    m_TranslateY = ty;
    m_TranslateZ = tz;
}

void TransformFilter::SetRotation(float rx,float ry,float rz){
    m_RotationX = rx;
    m_RotationY = ry;
    m_RotationZ = rz;
}


void TransformFilter::SetScale(float sx,float sy,float sz){
    m_ScaleX = sx;
    m_ScaleY = sy;
    m_ScaleZ = sz;
}

/** 
 * void TransformFilter::SetMatrix(const float matrix[4][4]){
    for (int i = 0; i < 4; ++i){
        for (int j = 0; j < 4; ++j){
            m_Matrix[i][j] = matrix[i][j];
        }
    }
}
 */



void TransformFilter::BuildMatrix()
{
    constexpr float PI = 3.14159265358979323846f;

    float rx = m_RotationX * PI / 180.0f;
    float ry = m_RotationY * PI / 180.0f;
    float rz = m_RotationZ * PI / 180.0f;
    float cx = std::cos(rx);
    float sx = std::sin(rx);
    float cy = std::cos(ry);
    float sy = std::sin(ry);
    float cz = std::cos(rz);
    float sz = std::sin(rz);

    float S[4][4] =
    {
        {m_ScaleX, 0.0f,     0.0f,     0.0f},
        {0.0f,     m_ScaleY, 0.0f,     0.0f},
        {0.0f,     0.0f,     m_ScaleZ, 0.0f},
        {0.0f,     0.0f,     0.0f,     1.0f}
    };

    float Rx[4][4] =
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, cx,   -sx,  0.0f},
        {0.0f, sx,    cx,  0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float Ry[4][4] =
    {
        {cy,   0.0f, sy,   0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {-sy,  0.0f, cy,   0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float Rz[4][4] =
    {
        {cz,   -sz,  0.0f, 0.0f},
        {sz,    cz,  0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float T[4][4] =
    {
        {1.0f, 0.0f, 0.0f, m_TranslateX},
        {0.0f, 1.0f, 0.0f, m_TranslateY},
        {0.0f, 0.0f, 1.0f, m_TranslateZ},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    auto MultiplyMatrix =[](const float A[4][4],const float B[4][4],float C[4][4])
    {
        for (int i = 0; i < 4; ++i){
            for (int j = 0; j < 4; ++j){
                C[i][j] = 0.0f;
            }
        }

        for (int i = 0; i < 4; ++i){
            for (int j = 0; j < 4; ++j){
                for (int k = 0; k < 4; ++k){
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    };

    float RxS[4][4]{};
    MultiplyMatrix(Rx, S, RxS);
    float RyRxS[4][4]{};
    MultiplyMatrix(Ry, RxS, RyRxS);
    float RzRyRxS[4][4]{};
    MultiplyMatrix(Rz, RyRxS, RzRyRxS);
    MultiplyMatrix(T, RzRyRxS, m_Matrix);
}


bool TransformFilter::Execute(){

    auto dataObject = this->GetInput(0);
    if (dataObject == nullptr){return false;}
    auto pointSet = DynamicCast<PointSet>(dataObject);
    if (pointSet == nullptr){return false;}

    BuildMatrix();

    PointSet::Pointer output = nullptr;

    switch(dataObject->GetDataObjectType()){
        case IG_SURFACE_MESH:{
            auto input = DynamicCast<SurfaceMesh>(dataObject);
            if (input == nullptr){return false;}

            auto surfaceOutput = SurfaceMesh::New();
            if (!surfaceOutput->DeepCopy(input)){
                return false;
            }

            output = surfaceOutput;
            break;
        }
        case IG_VOLUME_MESH:{
            auto input = DynamicCast<VolumeMesh>(dataObject);
            if (input == nullptr){return false;}

            auto volumeOutput = VolumeMesh::New();

            volumeOutput->SetVolumes(input->GetVolumes());
            volumeOutput->SetAttributeSet(input->GetAttributeSet());
            volumeOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            volumeOutput->SetPoints(newPoints);

            output = volumeOutput;
            break;
        }
        case IG_STRUCTURED_MESH:{
            auto input = DynamicCast<StructuredMesh>(dataObject);
            if (input == nullptr){return false;}

            auto structuredOutput = StructuredMesh::New();

            structuredOutput->SetDimensionSize(input->GetDimensionSize());
            structuredOutput->SetExtent(input->GetExtent());
            structuredOutput->SetAttributeSet(input->GetAttributeSet());
            structuredOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            structuredOutput->SetPoints(newPoints);

            output = structuredOutput;
            break;
        }
        case IG_UNSTRUCTURED_MESH:{
            auto input = DynamicCast<UnstructuredMesh>(dataObject);
            if (input == nullptr){return false;}

            auto unstructuredOutput = UnstructuredMesh::New();

            unstructuredOutput->SetCells(input->GetCells(),input->GetCellTypes());
            unstructuredOutput->SetAttributeSet(input->GetAttributeSet());
            unstructuredOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            unstructuredOutput->SetPoints(newPoints);

            output = unstructuredOutput;
            break;
        }

        default:
            return false;
    }

    if (output == nullptr){
        return false;
    }

    for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i){
        Point p = output->GetPoint(i);

        float x = p[0];
        float y = p[1];
        float z = p[2];
        float newX =
            m_Matrix[0][0] * x +
            m_Matrix[0][1] * y +
            m_Matrix[0][2] * z +
            m_Matrix[0][3];
        float newY =
            m_Matrix[1][0] * x +
            m_Matrix[1][1] * y +
            m_Matrix[1][2] * z +
            m_Matrix[1][3];
        float newZ =
            m_Matrix[2][0] * x +
            m_Matrix[2][1] * y +
            m_Matrix[2][2] * z +
            m_Matrix[2][3];
        float newW =
            m_Matrix[3][0] * x +
            m_Matrix[3][1] * y +
            m_Matrix[3][2] * z +
            m_Matrix[3][3];

        if (std::abs(newW) > 1e-6f){
            newX /= newW;
            newY /= newW;
            newZ /= newW;
        }

        Point newPoint(newX, newY, newZ);
        output->SetPoint(i, newPoint);
    }

    this->SetOutput(0, output);
    return true;
}


IGAME_NAMESPACE_END