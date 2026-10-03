#include "iGameIntegrateVariablesFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCell.h"
#include "iGameCellType.h"
#include "iGameStructuredMesh.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"
#include "iGameVolumeMesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

struct AttributeAccumulator {
    ArrayObject::Pointer input;
    IGenum type{IG_NONE};
    IGenum attachment{IG_NONE};
    std::vector<double> values;
}; // 用于累加点属性或单元属性的积分结果

struct GeometryView {
    Points::Pointer points;
    CellArray::Pointer cells;
    UnstructuredMesh::Pointer unstructured;
    int fixedDimension{0};
}; // 统一保存输入网格的几何数据和维度信息

// 计算三个空间点构成的三角形面积。
double TriangleArea(const Point& p0, const Point& p1, const Point& p2) {
    return (p1 - p0).cross(p2 - p0).norm() * 0.5;
}

// 计算四个空间点构成的四面体有符号体积。
double SignedTetraVolume(const Point& p0, const Point& p1,
                         const Point& p2, const Point& p3) {
    return (p1 - p0).cross(p2 - p0).dot(p3 - p0) / 6.0;
}

// 累加折线各线段的长度，并将每段长度的一半分配给两个端点作为积分权重。
double AddLineWeights(const std::vector<Point>& points, std::vector<double>& weights) {
    double measure = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const double length = (points[i + 1] - points[i]).norm();
        weights[i] += length * 0.5;
        weights[i + 1] += length * 0.5;
        measure += length;
    }
    return measure;
}

// 累加指定三角形的面积，并将面积均分给三个顶点作为积分权重。
double AddTriangleWeights(const std::vector<Point>& points,
                          size_t i0, size_t i1, size_t i2,
                          std::vector<double>& weights) {
    const double area = TriangleArea(points[i0], points[i1], points[i2]);
    const double share = area / 3.0;
    for (const size_t id : {i0, i1, i2}) weights[id] += share;
    return area;
}

// 累加指定四面体的有符号体积，并将体积均分给四个顶点作为积分权重。
double AddTetraWeights(const std::vector<Point>& points,
                       size_t i0, size_t i1, size_t i2, size_t i3,
                       std::vector<double>& weights) {
    const double volume = SignedTetraVolume(
            points[i0], points[i1], points[i2], points[i3]);
    const double share = volume * 0.25;
    for (const size_t id : {i0, i1, i2, i3}) weights[id] += share;
    return volume;
}

// 按照 ParaView 的线性策略计算单元测度及各顶点的积分权重。
bool ComputeIntegrationWeights(int dimension, const std::vector<Point>& points,
                               std::vector<double>& weights, double& measure) {
    weights.assign(points.size(), 0.0);
    measure = 0.0;
    if (points.empty()) return false;

    const auto addTriangle = [&](size_t i0, size_t i1, size_t i2) {
        measure += AddTriangleWeights(points, i0, i1, i2, weights);
    };
    const auto addTetra = [&](size_t i0, size_t i1, size_t i2, size_t i3) {
        measure += AddTetraWeights(points, i0, i1, i2, i3, weights);
    };

    if (dimension == 1 && points.size() >= 2) {
        measure = AddLineWeights(points, weights);
    } else if (dimension == 2 && points.size() >= 3) {
        for (size_t i = 1; i + 1 < points.size(); ++i)
            addTriangle(0, i, i + 1);
    } else if (dimension == 3) {
        switch (points.size()) {
            case 4:
                addTetra(0, 1, 2, 3);
                break;
            case 5: { // 金字塔：ParaView 沿底面较短的对角线进行拆分
                const double diagonal02 = points[0].distance2(points[2]);
                const double diagonal13 = points[1].distance2(points[3]);
                if (diagonal02 < diagonal13) {
                    addTetra(0, 1, 2, 4);
                    addTetra(0, 2, 3, 4);
                } else {
                    addTetra(0, 1, 3, 4);
                    addTetra(1, 2, 3, 4);
                }
                break;
            }
            case 6: // 三棱柱：底面为 0、1、2，顶面为 3、4、5
                addTetra(0, 1, 2, 3);
                addTetra(1, 4, 5, 3);
                addTetra(1, 3, 5, 2);
                break;
            case 8: // 六面体
                addTetra(0, 1, 3, 4);
                addTetra(1, 4, 5, 6);
                addTetra(1, 4, 6, 3);
                addTetra(1, 3, 6, 2);
                addTetra(3, 6, 7, 4);
                break;
            default:
                return false;
        }
    } else {
        return false;
    }

    if (!std::isfinite(measure)) return false;
    return std::all_of(weights.begin(), weights.end(),
                       [](double value) { return std::isfinite(value); });
}

// 从受支持的数据对象中取得点、单元连接关系和维度信息。
bool ResolveGeometry(const DataObject::Pointer& input, GeometryView& geometry) {
    geometry = {};
    geometry.points = input->GetPoints();
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(input);
            if (!mesh) return false;
            geometry.cells = mesh->GetCellArray();
            if (geometry.cells && geometry.cells->GetNumberOfCells() > 0) {
                geometry.fixedDimension = 2;
            } else {
                // 旧版 VTK POLYDATA LINES 在 iGame 中保存为 SurfaceMesh 的边。
                geometry.cells = mesh->GetEdges();
                geometry.fixedDimension = 1;
            }
            break;
        }
        case IG_VOLUME_MESH:
            geometry.cells = input->GetCellArray();
            geometry.fixedDimension = 3;
            break;
        case IG_UNSTRUCTURED_MESH:
            geometry.unstructured = DynamicCast<UnstructuredMesh>(input);
            if (!geometry.unstructured) return false;
            geometry.cells = geometry.unstructured->GetCells();
            break;
        case IG_STRUCTURED_MESH: {
            auto mesh = DynamicCast<StructuredMesh>(input);
            if (!mesh) return false;
            mesh->GenStructuredCellConnectivities();
            geometry.cells = mesh->GetCells();
            geometry.fixedDimension = static_cast<int>(mesh->GetDimension());
            break;
        }
        default:
            return false;
    }
    return geometry.points && geometry.cells;
}

// 读取指定单元的顶点编号和坐标。
int LoadCellPoints(const GeometryView& geometry, IGsize cellId,
                   const igIndex*& pointIds, std::vector<Point>& cellPoints) {
    const int pointCount = geometry.cells->GetCellIds(cellId, pointIds);
    if (pointCount <= 0 || !pointIds) return 0;

    cellPoints.clear();
    cellPoints.reserve(static_cast<size_t>(pointCount));
    const IGsize totalPointCount = geometry.points->GetNumberOfPoints();
    for (int i = 0; i < pointCount; ++i) {
        if (pointIds[i] < 0 ||
            static_cast<IGsize>(pointIds[i]) >= totalPointCount) return 0;
        cellPoints.push_back(geometry.points->GetPoint(pointIds[i]));
    }
    return pointCount;
}

// 根据网格类型、单元类型和顶点数确定可支持的积分维度。
int ResolveIntegrationDimension(const GeometryView& geometry, IGsize cellId,
                                const std::vector<Point>& cellPoints) {
    int dimension = geometry.fixedDimension;
    if (geometry.unstructured) {
        const IGenum cellType = geometry.unstructured->GetCellType(cellId);
        dimension = cellType == IG_POLY_LINE
                ? 1 : static_cast<int>(Cell::GetCellDimension(cellType));
    }
    const size_t pointCount = cellPoints.size();
    if (dimension == 1 && pointCount >= 2) return 1;
    if (dimension == 2 && pointCount >= 3) return 2;
    if (dimension == 3 &&
        (pointCount == 4 || pointCount == 5 ||
         pointCount == 6 || pointCount == 8)) return 3;
    return 0;
}

// 收集可参与积分的点属性和单元属性，并为每个属性创建累加器。
void CollectAttributes(AttributeSet* attributes, IGsize pointCount, IGsize cellCount,
                       const std::string& measureName,
                       std::vector<AttributeAccumulator>& pointAttributes,
                       std::vector<AttributeAccumulator>& cellAttributes) {
    if (!attributes) return;
    for (IGsize i = 0; i < attributes->GetNumberOfAttributes(); ++i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.IsNone() || !attr.pointer || attr.isDeleted) continue;

        const int components = attr.pointer->GetDimension();
        if (components <= 0) continue;

        AttributeAccumulator accumulator;
        accumulator.input = attr.pointer;
        accumulator.type = attr.type;
        accumulator.attachment = attr.attachmentType;
        accumulator.values.assign(static_cast<size_t>(components), 0.0);

        if (attr.attachmentType == IG_POINT &&
            attr.pointer->GetNumberOfElements() >= pointCount) {
            pointAttributes.push_back(std::move(accumulator));
        } else if (attr.attachmentType == IG_CELL &&
                   attr.pointer->GetNumberOfElements() >= cellCount &&
                   attr.pointer->GetName() != measureName) {
            // ParaView 会用新生成的几何测度替换输出中同名的单元属性数组。
            cellAttributes.push_back(std::move(accumulator));
        }
    }
}

// 将属性累加结果发布到输出数据，并按需将单元属性除以总测度。
void PublishAttributes(const std::vector<AttributeAccumulator>& accumulators,
                       AttributeSet* outputAttributes, bool divide,
                       double measure) {
    if (!outputAttributes) return;
    for (const auto& accumulator : accumulators) {
        auto output = DoubleArray::New();
        output->SetName(accumulator.input->GetName());
        output->SetDimension(static_cast<int>(accumulator.values.size()));
        std::vector<double> values = accumulator.values;
        if (divide && measure != 0.0) {
            for (double& value : values) value /= measure;
        }
        output->AddElement(values.data());
        outputAttributes->AddAttribute(
                accumulator.type, accumulator.attachment, output);
    }
}

} // 匿名命名空间

// 初始化过滤器的输入和输出端口数量。
IntegrateVariablesFilter::IntegrateVariablesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

// 根据当前最高积分维度返回对应的几何测度名称。
std::string IntegrateVariablesFilter::GetMeasureName() const {
    switch (m_IntegrationDimension) {
        case 1: return "Length";
        case 2: return "Area";
        case 3: return "Volume";
        default: return "";
    }
}

// 执行变量积分，生成包含积分属性、总测度和积分中心的单点输出。
bool IntegrateVariablesFilter::Execute() {
    m_Message.clear();
    m_IntegrationDimension = 0;
    m_IntegratedCellCount = 0;
    m_IntegratedMeasure = 0.0;

    auto input = GetInput(0);
    if (!input) {
        m_Message = "No input data, please load and select a mesh first.";
        return false;
    }

    GeometryView geometry;
    if (!ResolveGeometry(input, geometry)) {
        m_Message = "Unsupported input. Integrate Variables requires a surface, volume, unstructured, or structured mesh.";
        return false;
    }
    const IGsize pointCount = geometry.points->GetNumberOfPoints();
    const IGsize cellCount = geometry.cells->GetNumberOfCells();
    // 与 ParaView 一致，混合维度网格只积分其中最高维度的单元。
    std::vector<int> cellDimensions(static_cast<size_t>(cellCount), 0);
    const igIndex* pointIds = nullptr;
    std::vector<Point> cellPoints;
    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        if (LoadCellPoints(geometry, cellId, pointIds, cellPoints) == 0) continue;
        const int dimension = ResolveIntegrationDimension(geometry, cellId, cellPoints);
        cellDimensions[static_cast<size_t>(cellId)] = dimension;
        m_IntegrationDimension = std::max(m_IntegrationDimension, dimension);
    }
    if (m_IntegrationDimension == 0) {
        m_Message = "No supported 1D, 2D, or 3D cells were found.";
        return false;
    }

    const std::string measureName = GetMeasureName();
    std::vector<AttributeAccumulator> pointAttributes;
    std::vector<AttributeAccumulator> cellAttributes;
    CollectAttributes(input->GetAttributeSet(), pointCount, cellCount, measureName,
                      pointAttributes, cellAttributes);

    Vector3d weightedCenter(0.0, 0.0, 0.0); // 累加积分中心的分子部分，最后除以总测度得到积分中心坐标
    std::vector<double> pointWeights;

    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        const int dimension = cellDimensions[static_cast<size_t>(cellId)];
        if (dimension != m_IntegrationDimension) {
            continue;
        }

        const int cellPointCount = LoadCellPoints(geometry, cellId, pointIds, cellPoints);
        if (cellPointCount == 0) continue;
        double measure = 0.0;
        if (!ComputeIntegrationWeights(dimension, cellPoints, pointWeights, measure)) {
            continue;
        }

        ++m_IntegratedCellCount;
        m_IntegratedMeasure += measure;
        for (int localId = 0; localId < cellPointCount; ++localId) {
            const double weight = pointWeights[static_cast<size_t>(localId)];
            const Point& point = cellPoints[static_cast<size_t>(localId)];
            weightedCenter += Vector3d(point) * weight;

            for (auto& accumulator : pointAttributes) {
                for (size_t component = 0; component < accumulator.values.size(); ++component) {
                    accumulator.values[component] +=
                            accumulator.input->GetElementValue(pointIds[localId], static_cast<int>(component)) * weight;
                }
            }
        }

        for (auto& accumulator : cellAttributes) {
            for (size_t component = 0; component < accumulator.values.size(); ++component) {
                accumulator.values[component] +=
                        accumulator.input->GetElementValue(cellId, static_cast<int>(component)) * measure;
            }
        }
    }

    if (m_IntegratedCellCount == 0) {
        m_Message = "No cells of the highest integration dimension could be integrated.";
        return false;
    }

    Point outputPoint(0.0f, 0.0f, 0.0f);
    if (m_IntegratedMeasure != 0.0) {
        outputPoint = Point(weightedCenter / m_IntegratedMeasure);
    }

    auto output = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    outputPoints->AddPoint(outputPoint);
    output->SetPoints(outputPoints);
    igIndex vertexId = 0;
    output->AddCell(&vertexId, 1, IG_VERTEX);

    auto outputAttributes = output->GetAttributeSet();
    PublishAttributes(pointAttributes, outputAttributes, false, m_IntegratedMeasure);
    PublishAttributes(cellAttributes, outputAttributes,
                      m_DivideAllCellDataByMeasure, m_IntegratedMeasure);

    auto measureArray = DoubleArray::New();
    measureArray->SetName(measureName);
    measureArray->SetDimension(1);
    measureArray->AddValue(m_IntegratedMeasure);
    outputAttributes->AddAttribute(IG_SCALAR, IG_CELL, measureArray);
    outputAttributes->ForceReConvertToDrawableData();

    output->SetName(input->GetName() + "_IntegrateVariables");
    SetOutput(output);
    m_Message = "Integrated " + std::to_string(m_IntegratedCellCount) +
                " cells; total " + measureName + " = " +
                std::to_string(m_IntegratedMeasure) + ".";
    return true;
}

IGAME_NAMESPACE_END
