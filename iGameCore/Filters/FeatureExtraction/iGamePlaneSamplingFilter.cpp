#include "iGamePlaneSamplingFilter.h"
#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameHexahedron.h"
#include "iGamePointFinder.h"
#include "iGamePoints.h"
#include "iGameTetra.h"
#include "iGameTriangle.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

IGAME_NAMESPACE_BEGIN

// 辅助函数：判断点是否在三角形内（重心坐标，带容差）
static bool PointInTriangle(const Vector3d& p, const Vector3d& a, const Vector3d& b, const Vector3d& c,
                            double eps = 1e-8) {
    Vector3d v0 = c - a;
    Vector3d v1 = b - a;
    Vector3d v2 = p - a;

    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);

    double denom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(denom) < 1e-20) return false;

    double u = (dot11 * dot02 - dot01 * dot12) / denom;
    double v = (dot00 * dot12 - dot01 * dot02) / denom;

    return (u >= -eps && v >= -eps && (u + v) <= 1.0 + eps);
}

// 辅助函数：判断点是否在四面体内（体积坐标，带容差）
static bool PointInTetra(const Vector3d& p, const Vector3d& a, const Vector3d& b, const Vector3d& c, const Vector3d& d,
                         double eps = 1e-6) {
    Vector3d da = a - d;
    Vector3d db = b - d;
    Vector3d dc = c - d;
    Vector3d dp = p - d;

    double det = da.dot(db.cross(dc));
    if (std::abs(det) < 1e-20) return false;

    double u = dp.dot(db.cross(dc)) / det;
    double v = da.dot(dp.cross(dc)) / det;
    double w = da.dot(db.cross(dp)) / det;
    double t = 1.0 - u - v - w;

    return (u >= -eps && v >= -eps && w >= -eps && t >= -eps);
}

// 辅助函数：三角形插值（重心坐标插值）
static double InterpolateTriangle(const Vector3d& p, const Vector3d& a, const Vector3d& b, const Vector3d& c, double va,
                                  double vb, double vc) {
    Vector3d v0 = c - a;
    Vector3d v1 = b - a;
    Vector3d v2 = p - a;

    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);

    double denom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(denom) < 1e-20) return va;

    double u = (dot11 * dot02 - dot01 * dot12) / denom;
    double v = (dot00 * dot12 - dot01 * dot02) / denom;

    return va + u * (vc - va) + v * (vb - va);
}

// 辅助函数：四面体插值（体积坐标插值）
static double InterpolateTetra(const Vector3d& p, const Vector3d& a, const Vector3d& b, const Vector3d& c,
                               const Vector3d& d, double va, double vb, double vc, double vd) {
    Vector3d da = a - d;
    Vector3d db = b - d;
    Vector3d dc = c - d;
    Vector3d dp = p - d;

    double det = da.dot(db.cross(dc));
    if (std::abs(det) < 1e-20) return va;

    double u = dp.dot(db.cross(dc)) / det;
    double v = da.dot(dp.cross(dc)) / det;
    double w = da.dot(db.cross(dp)) / det;
    double t = 1.0 - u - v - w;

    u = std::max(0.0, std::min(1.0, u));
    v = std::max(0.0, std::min(1.0, v));
    w = std::max(0.0, std::min(1.0, w));
    t = 1.0 - u - v - w;

    return u * va + v * vb + w * vc + t * vd;
}

// 辅助函数：读取顶点属性值（支持标量和矢量）
static void ReadPointAttribute(ArrayObject* attr, igIndex pointId, std::vector<double>& values) {
    if (!attr) {
        values.clear();
        return;
    }

    int dim = attr->GetDimension();
    values.resize(dim);

    if (auto floatArr = DynamicCast<FloatArray>(attr)) {
        for (int d = 0; d < dim; d++) { values[d] = floatArr->GetValue(pointId * dim + d); }
    } else if (auto doubleArr = DynamicCast<DoubleArray>(attr)) {
        for (int d = 0; d < dim; d++) { values[d] = doubleArr->GetValue(pointId * dim + d); }
    } else {
        double tmp[16] = {0};
        attr->GetElement(pointId, tmp);
        for (int d = 0; d < dim && d < 16; d++) { values[d] = tmp[d]; }
    }
}

// 辅助函数：从矢量值计算幅值
static double ComputeMagnitude(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double v: vec) { sum += v * v; }
    return std::sqrt(sum);
}

// 核心执行函数
bool PlaneSamplingFilter::Execute() {
    // 第1步：获取输入数据
    auto input = this->GetInput(0);
    if (!input) {
        std::cerr << "PlaneSamplingFilter: no input data" << std::endl;
        return false;
    }

    // 第2步：转换为非结构网格
    auto inputMesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (!inputMesh) {
        std::cerr << "PlaneSamplingFilter: failed to convert to unstructured mesh" << std::endl;
        return false;
    }

    auto points = inputMesh->GetPoints();
    auto numPoints = inputMesh->GetNumberOfPoints();
    auto numCells = inputMesh->GetNumberOfCells();

    if (numPoints == 0 || numCells == 0) {
        std::cerr << "PlaneSamplingFilter: mesh has no points or cells" << std::endl;
        return false;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Input mesh: " << numPoints << " points, " << numCells << " cells" << std::endl;

    // 第3步：计算包围盒
    double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
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

    double dx = bounds[1] - bounds[0];
    double dy = bounds[3] - bounds[2];
    double dz = bounds[5] - bounds[4];

    std::cout << "Model bounds: X[" << bounds[0] << ", " << bounds[1] << "], Y[" << bounds[2] << ", " << bounds[3]
              << "], Z[" << bounds[4] << ", " << bounds[5] << "]" << std::endl;

    // 根据法向选择范围基准
    double nx = std::abs(m_Normal[0]);
    double ny = std::abs(m_Normal[1]);
    double nz = std::abs(m_Normal[2]);

    double scale = 0.5;

    if (nz > 0.9 && nx < 0.1 && ny < 0.1) {
        double xyRange = std::max(dx, dy);
        m_HalfRange = xyRange * scale;
    } else if (nx > 0.9 && ny < 0.1 && nz < 0.1) {
        double yzRange = std::max(dy, dz);
        m_HalfRange = yzRange * scale;
    } else if (ny > 0.9 && nx < 0.1 && nz < 0.1) {
        double xzRange = std::max(dx, dz);
        m_HalfRange = xzRange * scale;
    } else {
        double total = nx + ny + nz;
        if (total < 1e-10) {
            nx = 0.0;
            ny = 0.0;
            nz = 1.0;
            total = 1.0;
        }
        nx /= total;
        ny /= total;
        nz /= total;
        double weightedRange = nx * dx + ny * dy + nz * dz;
        m_HalfRange = weightedRange * scale;
    }

    if (m_HalfRange < 1e-10) { m_HalfRange = 1.0; }

    std::cout << "Plane half range: " << m_HalfRange << std::endl;

    // 第4步：计算平面方向
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

    m_U[0] = u[0];
    m_U[1] = u[1];
    m_U[2] = u[2];
    m_V[0] = v[0];
    m_V[1] = v[1];
    m_V[2] = v[2];

    // 第5步：查找属性
    ArrayObject::Pointer targetAttr = nullptr;
    auto attrSet = inputMesh->GetAttributeSet();
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

    // 第6步：判断是标量还是矢量
    bool isVector = (targetAttr && targetAttr->GetDimension() > 1);
    std::string baseName = selectedAttrName;
    if (baseName.empty()) { baseName = "Height"; }

    int totalSamples = m_Resolution * m_Resolution;

    // 第7步：创建输出网格
    auto outputMesh = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    outputPoints->Resize(totalSamples);

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

    // 第8步：获取单元类型数组
    auto typeArray = inputMesh->GetCellTypes();

    // ---- 诊断：打印单元类型分布 ----
    std::cout << "========== Cell Type Diagnostics ==========" << std::endl;
    std::map<IGenum, int> typeCount;
    for (IGsize i = 0; i < numCells; i++) {
        IGenum type = typeArray->GetValue(i);
        typeCount[type]++;
    }
    for (auto& pair: typeCount) {
        std::cout << "Cell type " << pair.first << ": " << pair.second << " cells" << std::endl;
    }
    std::cout << "============================================" << std::endl;

    // 收集所有可用的单元
    struct CellData {
        std::vector<Vector3d> coords;
        std::vector<igIndex> pointIds;
        int numPoints;
    };
    std::vector<CellData> cellList;

    for (IGsize cellId = 0; cellId < numCells; cellId++) {
        igIndex pointIds[IGAME_CELL_MAX_SIZE];
        int numCellPoints = inputMesh->GetCellPointIds(cellId, pointIds);

        if (numCellPoints < 3) continue;

        std::vector<Vector3d> cellCoords;
        for (int k = 0; k < numCellPoints; k++) {
            Point p = inputMesh->GetPoint(pointIds[k]);
            cellCoords.emplace_back(p[0], p[1], p[2]);
        }

        // 六面体拆分为四面体
        if (numCellPoints == 8) {
            auto hexa = Hexahedron::New();
            hexa->m_PointIds->Reset();
            hexa->m_Points->Reset();
            for (int k = 0; k < 8; k++) {
                hexa->m_PointIds->AddId(pointIds[k]);
                hexa->m_Points->AddPoint(cellCoords[k]);
            }
            auto tetras = hexa->clipCelltoTetra();
            for (auto& tetra: tetras) {
                CellData data;
                data.numPoints = 4;
                for (int k = 0; k < 4; k++) {
                    Point p = tetra->m_Points->GetPoint(k);
                    data.coords.emplace_back(p[0], p[1], p[2]);
                    data.pointIds.push_back(tetra->m_PointIds->GetId(k));
                }
                cellList.push_back(data);
            }
        }
        // 四面体直接加入
        else if (numCellPoints == 4) {
            CellData data;
            data.numPoints = 4;
            for (int k = 0; k < 4; k++) {
                data.coords.emplace_back(cellCoords[k]);
                data.pointIds.push_back(pointIds[k]);
            }
            cellList.push_back(data);
        }
        // 三角形直接加入
        else if (numCellPoints == 3) {
            CellData data;
            data.numPoints = 3;
            for (int k = 0; k < 3; k++) {
                data.coords.emplace_back(cellCoords[k]);
                data.pointIds.push_back(pointIds[k]);
            }
            cellList.push_back(data);
        }
    }

    std::cout << "Collected " << cellList.size() << " cells for interpolation" << std::endl;

    // ---- 创建 PointFinder（用于最近点后备） ----
    auto finder = PointFinder::New();
    finder->SetPoints(points);
    finder->Initialize();

    // 第9步：核心采样循环
    std::cout << "Sampling " << totalSamples << " points..." << std::endl;
    int validCount = 0;
    int tetraFoundCount = 0;
    int triangleFoundCount = 0;
    int fallbackCount = 0;

    // 吸附阈值：采样范围的 0.5%
    double snapThreshold = m_HalfRange * 0.005;

    // ---- 调试计数器 ----
    int debugSampleIdx = 0;

    for (int i = 0; i < m_Resolution; i++) {
        for (int j = 0; j < m_Resolution; j++) {
            double t_u = -m_HalfRange + 2.0 * m_HalfRange * i / (m_Resolution - 1);
            double t_v = -m_HalfRange + 2.0 * m_HalfRange * j / (m_Resolution - 1);

            double samplePoint[3] = {0.0, 0.0, 0.0};
            samplePoint[0] = m_Origin[0] + t_u * m_U[0] + t_v * m_V[0];
            samplePoint[1] = m_Origin[1] + t_u * m_U[1] + t_v * m_V[1];
            samplePoint[2] = m_Origin[2] + t_u * m_U[2] + t_v * m_V[2];

            int idx = i * m_Resolution + j;
            outputPoints->SetPoint(idx, (float) samplePoint[0], (float) samplePoint[1], (float) samplePoint[2]);

            Vector3d query(samplePoint[0], samplePoint[1], samplePoint[2]);

            bool foundCell = false;
            bool isValid = false;
            double resultScalar = 0.0;
            double resultMag = 0.0;
            double resultVx = 0.0, resultVy = 0.0, resultVz = 0.0;

            // ---- 第1步：尝试单元定位 + 插值 ----
            for (size_t cellIdx = 0; cellIdx < cellList.size() && !foundCell; cellIdx++) {
                auto& cell = cellList[cellIdx];
                const auto& coords = cell.coords;
                const auto& ids = cell.pointIds;
                bool inside = false;

                if (cell.numPoints == 4) {
                    inside = PointInTetra(query, coords[0], coords[1], coords[2], coords[3]);
                    if (inside) {
                        tetraFoundCount++;
                        if (targetAttr) {
                            int dim = targetAttr->GetDimension();
                            std::vector<double> v0, v1, v2, v3;
                            ReadPointAttribute(targetAttr, ids[0], v0);
                            ReadPointAttribute(targetAttr, ids[1], v1);
                            ReadPointAttribute(targetAttr, ids[2], v2);
                            ReadPointAttribute(targetAttr, ids[3], v3);

                            if (dim == 1) {
                                resultScalar = InterpolateTetra(query, coords[0], coords[1], coords[2], coords[3],
                                                                v0[0], v1[0], v2[0], v3[0]);
                            } else {
                                std::vector<double> interpVec(dim, 0.0);
                                for (int d = 0; d < dim; d++) {
                                    interpVec[d] = InterpolateTetra(query, coords[0], coords[1], coords[2], coords[3],
                                                                    v0[d], v1[d], v2[d], v3[d]);
                                }
                                resultMag = ComputeMagnitude(interpVec);
                                resultVx = (dim > 0) ? interpVec[0] : 0.0;
                                resultVy = (dim > 1) ? interpVec[1] : 0.0;
                                resultVz = (dim > 2) ? interpVec[2] : 0.0;
                            }
                        } else {
                            resultScalar = query[2];
                        }
                        isValid = true;
                        foundCell = true;
                        break;
                    }
                } else if (cell.numPoints == 3) {
                    inside = PointInTriangle(query, coords[0], coords[1], coords[2]);
                    if (inside) {
                        triangleFoundCount++;
                        if (targetAttr) {
                            int dim = targetAttr->GetDimension();
                            std::vector<double> v0, v1, v2;
                            ReadPointAttribute(targetAttr, ids[0], v0);
                            ReadPointAttribute(targetAttr, ids[1], v1);
                            ReadPointAttribute(targetAttr, ids[2], v2);

                            if (dim == 1) {
                                resultScalar = InterpolateTriangle(query, coords[0], coords[1], coords[2], v0[0], v1[0],
                                                                   v2[0]);
                            } else {
                                std::vector<double> interpVec(dim, 0.0);
                                for (int d = 0; d < dim; d++) {
                                    interpVec[d] = InterpolateTriangle(query, coords[0], coords[1], coords[2], v0[d],
                                                                       v1[d], v2[d]);
                                }
                                resultMag = ComputeMagnitude(interpVec);
                                resultVx = (dim > 0) ? interpVec[0] : 0.0;
                                resultVy = (dim > 1) ? interpVec[1] : 0.0;
                                resultVz = (dim > 2) ? interpVec[2] : 0.0;
                            }
                        } else {
                            resultScalar = query[2];
                        }
                        isValid = true;
                        foundCell = true;
                        break;
                    }
                }
            }

            // ---- 第2步：如果单元定位失败，使用最近点后备（带吸附阈值） ----
            if (!foundCell) {
                double minDist2;
                igIndex closestId = finder->FindClosestPoint(query, minDist2);
                double dist = std::sqrt(minDist2);

                // 如果距离小于吸附阈值，使用最近点的属性值
                if (closestId != -1 && dist < snapThreshold) {
                    fallbackCount++;
                    if (targetAttr) {
                        int dim = targetAttr->GetDimension();
                        if (dim == 1) {
                            double val = 0.0;
                            targetAttr->GetElement(closestId, &val);
                            resultScalar = val;
                        } else {
                            std::vector<double> vals;
                            ReadPointAttribute(targetAttr, closestId, vals);
                            resultMag = ComputeMagnitude(vals);
                            resultVx = (dim > 0) ? vals[0] : 0.0;
                            resultVy = (dim > 1) ? vals[1] : 0.0;
                            resultVz = (dim > 2) ? vals[2] : 0.0;
                        }
                    } else {
                        Point p = points->GetPoint(closestId);
                        resultScalar = p[2];
                    }
                    isValid = true;
                }
            }

            //// ---- 调试：前几个采样点的详细信息 ----
            //if (debugSampleIdx < 5) {
            //    std::cout << "\n[DEBUG Sample " << debugSampleIdx << "]" << std::endl;
            //    std::cout << "  Point: (" << samplePoint[0] << ", " << samplePoint[1] << ", " << samplePoint[2] << ")"
            //              << std::endl;
            //    std::cout << "  Found cell: " << (foundCell ? "YES" : "NO") << std::endl;
            //    if (foundCell) {
            //        std::cout << "  Method: INTERPOLATION" << std::endl;
            //    } else {
            //        std::cout << "  Method: FALLBACK (nearest point)" << std::endl;
            //    }
            //    std::cout << "  Valid: " << (isValid ? "YES" : "NO") << std::endl;
            //    if (isValid) { std::cout << "  Result: " << resultScalar << std::endl; }
            //    debugSampleIdx++;
            //}

            // ---- 填充输出数据 ----
            if (isValid) {
                validCount++;
                if (isVector) {
                    magData->SetValue(idx, (float) resultMag);
                    xData->SetValue(idx, (float) resultVx);
                    yData->SetValue(idx, (float) resultVy);
                    zData->SetValue(idx, (float) resultVz);
                } else {
                    scalarData->SetValue(idx, (float) resultScalar);
                }
            } else {
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

    // ---- 输出采样统计 ----
    std::cout << "\n========================================" << std::endl;
    std::cout << "========= Sampling Result =========" << std::endl;
    std::cout << "Valid points: " << validCount << " / " << totalSamples << std::endl;
    std::cout << "Tetra interpolation: " << tetraFoundCount << std::endl;
    std::cout << "Triangle interpolation: " << triangleFoundCount << std::endl;
    std::cout << "Fallback (nearest point, dist < threshold): " << fallbackCount << std::endl;
    std::cout << "Unsampled points: " << (totalSamples - validCount) << std::endl;
    std::cout << "Snap threshold: " << snapThreshold << std::endl;
    std::cout << "========================================" << std::endl;

    // 第10步：创建四边形网格
    auto cellArrayOut = CellArray::New();
    int cellsPerRow = m_Resolution - 1;
    for (int i = 0; i < cellsPerRow; i++) {
        for (int j = 0; j < cellsPerRow; j++) {
            igIndex idx1 = i * m_Resolution + j;
            igIndex idx2 = i * m_Resolution + (j + 1);
            igIndex idx3 = (i + 1) * m_Resolution + (j + 1);
            igIndex idx4 = (i + 1) * m_Resolution + j;
            igIndex quad[4] = {idx1, idx2, idx3, idx4};
            cellArrayOut->AddCellIds(quad, 4);
        }
    }

    auto cellTypes = UnsignedIntArray::New();
    int totalCellsOut = cellsPerRow * cellsPerRow;
    for (int i = 0; i < totalCellsOut; i++) { cellTypes->AddValue(IG_QUAD); }
    outputMesh->SetCells(cellArrayOut, cellTypes);

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