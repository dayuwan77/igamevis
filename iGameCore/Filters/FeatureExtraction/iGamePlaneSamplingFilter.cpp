#include "iGamePlaneSamplingFilter.h"

#include <cmath>
#include <iostream>
#include <vector>

IGAME_NAMESPACE_BEGIN

bool PlaneSamplingFilter::Execute() {
    // 第1步：获取输入数据
    auto input = this->GetInput(0);
    if (!input) {
        std::cerr << "PlaneSamplingFilter: no input data" << std::endl;
        return false;
    }

    // 第2步：转换为 PointSet（支持所有网格类型）
    auto pointSet = DynamicCast<PointSet>(input);
    if (!pointSet) {
        std::cerr << "PlaneSamplingFilter: input is not a mesh" << std::endl;
        return false;
    }

    // 第3步：获取模型的点数据
    auto points = pointSet->GetPoints();
    auto numPoints = pointSet->GetNumberOfPoints();
    if (numPoints == 0) {
        std::cerr << "PlaneSamplingFilter: mesh has no points" << std::endl;
        return false;
    }

    std::cout << "Model has " << numPoints << " points" << std::endl;

    // 第4步：计算包围盒
    double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    if (numPoints > 0) {
        Point firstPoint = points->GetPoint(0);
        bounds[0] = bounds[1] = firstPoint[0];
        bounds[2] = bounds[3] = firstPoint[1];
        bounds[4] = bounds[5] = firstPoint[2];

        for (IGsize i = 1; i < numPoints; i++) {
            Point p = points->GetPoint(i);
            if (p[0] < bounds[0]) bounds[0] = p[0];
            if (p[0] > bounds[1]) bounds[1] = p[0];
            if (p[1] < bounds[2]) bounds[2] = p[1];
            if (p[1] > bounds[3]) bounds[3] = p[1];
            if (p[2] < bounds[4]) bounds[4] = p[2];
            if (p[2] > bounds[5]) bounds[5] = p[2];
        }
    } else {
        bounds[0] = -1.0;
        bounds[1] = 1.0;
        bounds[2] = -1.0;
        bounds[3] = 1.0;
        bounds[4] = -1.0;
        bounds[5] = 1.0;
    }

    double dx = bounds[1] - bounds[0];
    double dy = bounds[3] - bounds[2];
    double dz = bounds[5] - bounds[4];
    double diagonal = std::sqrt(dx * dx + dy * dy + dz * dz);

    m_HalfRange = diagonal * 0.6;
    if (m_HalfRange < 1e-10) { m_HalfRange = 1.0; }

    std::cout << "Plane range: " << m_HalfRange << std::endl;

    // 第5步：创建 PointFinder
    auto finder = PointFinder::New();
    finder->SetPoints(points);
    finder->Initialize();

    // 第6步：计算平面方向
    double u[3] = {1.0, 0.0, 0.0};
    double v[3] = {0.0, 1.0, 0.0};

    if (std::abs(m_Normal[0]) > 0.9) {
        u[0] = 0.0;
        u[1] = 1.0;
        u[2] = 0.0;
    }

    v[0] = m_Normal[1] * u[2] - m_Normal[2] * u[1];
    v[1] = m_Normal[2] * u[0] - m_Normal[0] * u[2];
    v[2] = m_Normal[0] * u[1] - m_Normal[1] * u[0];

    u[0] = v[1] * m_Normal[2] - v[2] * m_Normal[1];
    u[1] = v[2] * m_Normal[0] - v[0] * m_Normal[2];
    u[2] = v[0] * m_Normal[1] - v[1] * m_Normal[0];

    double uLen = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    double vLen = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (uLen < 1e-20 || vLen < 1e-20) {
        std::cerr << "PlaneSamplingFilter: cannot compute plane direction" << std::endl;
        return false;
    }
    u[0] /= uLen;
    u[1] /= uLen;
    u[2] /= uLen;
    v[0] /= vLen;
    v[1] /= vLen;
    v[2] /= vLen;

    // 第7步：查找属性
    ArrayObject::Pointer targetAttr = nullptr;
    auto attrSet = pointSet->GetAttributeSet();
    std::string selectedAttrName = "";

    if (!m_AttributeName.empty() && attrSet) {
        auto& attr = attrSet->GetAttribute(m_AttributeName);
        if (!attr.IsNone()) {
            targetAttr = attr.pointer;
            selectedAttrName = m_AttributeName;
            std::cout << "Using attribute: " << selectedAttrName << " (dimension: " << targetAttr->GetDimension() << ")"
                      << std::endl;
        }
    }

    if (!targetAttr && attrSet) {
        auto allAttrs = attrSet->GetAllAttributes();
        if (allAttrs && allAttrs->GetNumberOfElements() > 0) {
            for (igIndex i = 0; i < allAttrs->GetNumberOfElements(); i++) {
                auto& attr = allAttrs->GetElement(i);
                if (attr.type == IG_SCALAR && attr.pointer && !attr.isDeleted) {
                    targetAttr = attr.pointer;
                    selectedAttrName = targetAttr->GetName();
                    std::cout << "Auto-selected scalar: " << selectedAttrName << std::endl;
                    break;
                }
            }
            if (!targetAttr) {
                for (igIndex i = 0; i < allAttrs->GetNumberOfElements(); i++) {
                    auto& attr = allAttrs->GetElement(i);
                    if (attr.pointer && !attr.isDeleted) {
                        targetAttr = attr.pointer;
                        selectedAttrName = targetAttr->GetName();
                        std::cout << "Auto-selected attr: " << selectedAttrName << std::endl;
                        break;
                    }
                }
            }
        }
    }

    if (!targetAttr) { std::cout << "No attribute found, using Z coordinate as fallback" << std::endl; }

    // 第8步：判断是标量还是矢量
    bool isVector = (targetAttr && targetAttr->GetDimension() > 1);
    std::string baseName = selectedAttrName;
    if (baseName.empty()) { baseName = "Height"; }

    int totalSamples = m_Resolution * m_Resolution;

    auto outputMesh = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    outputPoints->Resize(totalSamples);

    // 标量：生成1个数组；矢量：生成4个数组（Magnitude + X + Y + Z） 
    FloatArray::Pointer scalarData = nullptr;
    FloatArray::Pointer magData = nullptr;
    FloatArray::Pointer xData = nullptr;
    FloatArray::Pointer yData = nullptr;
    FloatArray::Pointer zData = nullptr;

    if (isVector) {
        magData = FloatArray::New();
        magData->SetName(baseName + "_Magnitude");
        magData->Resize(totalSamples);

        xData = FloatArray::New();
        xData->SetName(baseName + "_X");
        xData->Resize(totalSamples);

        yData = FloatArray::New();
        yData->SetName(baseName + "_Y");
        yData->Resize(totalSamples);

        zData = FloatArray::New();
        zData->SetName(baseName + "_Z");
        zData->Resize(totalSamples);
    } else {
        scalarData = FloatArray::New();
        scalarData->SetName(baseName);
        scalarData->Resize(totalSamples);
    }

    auto validMask = IntArray::New();
    validMask->SetName("vtkValidPointMask");
    validMask->Resize(totalSamples);

    // 第9步：核心采样循环 
    std::cout << "Sampling " << totalSamples << " points..." << std::endl;
    int validCount = 0;

    for (int i = 0; i < m_Resolution; i++) {
        for (int j = 0; j < m_Resolution; j++) {
            double t_u = -m_HalfRange + 2.0 * m_HalfRange * i / (m_Resolution - 1);
            double t_v = -m_HalfRange + 2.0 * m_HalfRange * j / (m_Resolution - 1);

            double samplePoint[3] = {0.0, 0.0, 0.0};
            samplePoint[0] = m_Origin[0] + t_u * u[0] + t_v * v[0];
            samplePoint[1] = m_Origin[1] + t_u * u[1] + t_v * v[1];
            samplePoint[2] = m_Origin[2] + t_u * u[2] + t_v * v[2];

            int idx = i * m_Resolution + j;
            outputPoints->SetPoint(idx, (float) samplePoint[0], (float) samplePoint[1], (float) samplePoint[2]);

            Vector3d query(samplePoint[0], samplePoint[1], samplePoint[2]);
            double minDist2;
            igIndex closestId = finder->FindClosestPoint(query, minDist2);

            bool isValid = false;

            if (closestId != -1) {
                isValid = true;
                validCount++;

                if (targetAttr) {
                    int dim = targetAttr->GetDimension();

                    if (dim == 1) {
                        // 标量属性：只读一个值 
                        double val = 0.0;
                        targetAttr->GetElement(closestId, &val);
                        scalarData->SetValue(idx, (float) val);
                    } else {
                        // 矢量属性：读取所有分量
                        std::vector<double> values(dim);
                        targetAttr->GetElement(closestId, values);

                        double vx = (dim > 0) ? values[0] : 0.0;
                        double vy = (dim > 1) ? values[1] : 0.0;
                        double vz = (dim > 2) ? values[2] : 0.0;
                        double mag = std::sqrt(vx * vx + vy * vy + vz * vz);

                        magData->SetValue(idx, (float) mag);
                        xData->SetValue(idx, (float) vx);
                        yData->SetValue(idx, (float) vy);
                        zData->SetValue(idx, (float) vz);
                    }
                } else {
                    // 备用：使用 Z 坐标
                    Point closestPoint = points->GetPoint(closestId);
                    if (isVector) {
                        double vx = closestPoint[0];
                        double vy = closestPoint[1];
                        double vz = closestPoint[2];
                        double mag = std::sqrt(vx * vx + vy * vy + vz * vz);
                        magData->SetValue(idx, (float) mag);
                        xData->SetValue(idx, (float) vx);
                        yData->SetValue(idx, (float) vy);
                        zData->SetValue(idx, (float) vz);
                    } else {
                        scalarData->SetValue(idx, (float) closestPoint[2]);
                    }
                }
            } else {
                // 无效点：填充0
                if (isVector) {
                    magData->SetValue(idx, 0.0f);
                    xData->SetValue(idx, 0.0f);
                    yData->SetValue(idx, 0.0f);
                    zData->SetValue(idx, 0.0f);
                } else {
                    scalarData->SetValue(idx, 0.0f);
                }
            }
            validMask->SetValue(idx, isValid ? 1 : 0);
        }
        this->UpdateProgress(static_cast<double>(i + 1) / m_Resolution);
    }

    std::cout << "Valid points: " << validCount << " / " << totalSamples << std::endl;

    // 第10步：创建四边形网格
    auto cellArray = CellArray::New();
    int cellsPerRow = m_Resolution - 1;
    for (int i = 0; i < cellsPerRow; i++) {
        for (int j = 0; j < cellsPerRow; j++) {
            igIndex idx1 = i * m_Resolution + j;
            igIndex idx2 = i * m_Resolution + (j + 1);
            igIndex idx3 = (i + 1) * m_Resolution + (j + 1);
            igIndex idx4 = (i + 1) * m_Resolution + j;
            igIndex quad[4] = {idx1, idx2, idx3, idx4};
            cellArray->AddCellIds(quad, 4);
        }
    }

    auto cellTypes = UnsignedIntArray::New();
    int totalCells = cellsPerRow * cellsPerRow;
    for (int i = 0; i < totalCells; i++) { cellTypes->AddValue(IG_QUAD); }
    outputMesh->SetCells(cellArray, cellTypes);

    // 第11步：组装输出
    outputMesh->SetPoints(outputPoints);

    auto outputAttrSet = AttributeSet::New();
    if (isVector) {
        outputAttrSet->AddScalar(IG_POINT, magData);
        outputAttrSet->AddScalar(IG_POINT, xData);
        outputAttrSet->AddScalar(IG_POINT, yData);
        outputAttrSet->AddScalar(IG_POINT, zData);
    } else {
        outputAttrSet->AddScalar(IG_POINT, scalarData);
    }
    outputAttrSet->AddScalar(IG_POINT, validMask);
    outputMesh->SetAttributeSet(outputAttrSet);

    this->SetOutput(0, outputMesh);

    std::cout << "PlaneSamplingFilter: sampling complete" << std::endl;
    return true;
}

// 用户参数设置函数
void PlaneSamplingFilter::SetPlaneOrigin(double ox, double oy, double oz) {
    m_Origin[0] = ox;
    m_Origin[1] = oy;
    m_Origin[2] = oz;
}

void PlaneSamplingFilter::SetPlaneOrigin(const double origin[3]) {
    m_Origin[0] = origin[0];
    m_Origin[1] = origin[1];
    m_Origin[2] = origin[2];
}

void PlaneSamplingFilter::SetPlaneNormal(double nx, double ny, double nz) {
    double len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 1e-20) {
        m_Normal[0] = nx / len;
        m_Normal[1] = ny / len;
        m_Normal[2] = nz / len;
    }
}

void PlaneSamplingFilter::SetPlaneNormal(const double normal[3]) {
    double len = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (len > 1e-20) {
        m_Normal[0] = normal[0] / len;
        m_Normal[1] = normal[1] / len;
        m_Normal[2] = normal[2] / len;
    }
}

void PlaneSamplingFilter::SetResolution(int res) {
    if (res > 1) { m_Resolution = res; }
}

void PlaneSamplingFilter::GetPlaneOrigin(double origin[3]) const {
    origin[0] = m_Origin[0];
    origin[1] = m_Origin[1];
    origin[2] = m_Origin[2];
}

void PlaneSamplingFilter::GetPlaneNormal(double normal[3]) const {
    normal[0] = m_Normal[0];
    normal[1] = m_Normal[1];
    normal[2] = m_Normal[2];
}

IGAME_NAMESPACE_END