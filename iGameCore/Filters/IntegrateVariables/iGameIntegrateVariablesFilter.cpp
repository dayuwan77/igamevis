#include "iGameIntegrateVariablesFilter.h"

#include "CellSize/iGameCellSizeFilter.h"
#include "iGameAttributeSet.h"
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
};

double TriangleArea(const Point& p0, const Point& p1, const Point& p2) {
    const double e10 = static_cast<double>(p1[0]) - p0[0];
    const double e11 = static_cast<double>(p1[1]) - p0[1];
    const double e12 = static_cast<double>(p1[2]) - p0[2];
    const double e20 = static_cast<double>(p2[0]) - p0[0];
    const double e21 = static_cast<double>(p2[1]) - p0[1];
    const double e22 = static_cast<double>(p2[2]) - p0[2];
    const double cx = e11 * e22 - e12 * e21;
    const double cy = e12 * e20 - e10 * e22;
    const double cz = e10 * e21 - e11 * e20;
    return std::sqrt(cx * cx + cy * cy + cz * cz) * 0.5;
}

double SignedTetraVolume(const Point& p0, const Point& p1,
                         const Point& p2, const Point& p3) {
    // vtkIntegrationLinearStrategy intentionally keeps the sign here.  It is
    // part of Integrate Variables semantics for inverted 3-D cells.
    const double e10 = static_cast<double>(p1[0]) - p0[0];
    const double e11 = static_cast<double>(p1[1]) - p0[1];
    const double e12 = static_cast<double>(p1[2]) - p0[2];
    const double e20 = static_cast<double>(p2[0]) - p0[0];
    const double e21 = static_cast<double>(p2[1]) - p0[1];
    const double e22 = static_cast<double>(p2[2]) - p0[2];
    const double e30 = static_cast<double>(p3[0]) - p0[0];
    const double e31 = static_cast<double>(p3[1]) - p0[1];
    const double e32 = static_cast<double>(p3[2]) - p0[2];
    const double cx = e11 * e22 - e12 * e21;
    const double cy = e12 * e20 - e10 * e22;
    const double cz = e10 * e21 - e11 * e20;
    return (cx * e30 + cy * e31 + cz * e32) / 6.0;
}

double SquaredDistance(const Point& p0, const Point& p1) {
    const double dx = static_cast<double>(p1[0]) - p0[0];
    const double dy = static_cast<double>(p1[1]) - p0[1];
    const double dz = static_cast<double>(p1[2]) - p0[2];
    return dx * dx + dy * dy + dz * dz;
}

double AddLineWeights(const std::vector<Point>& points,
                      std::vector<double>& weights) {
    double measure = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const double length = std::sqrt(SquaredDistance(points[i], points[i + 1]));
        weights[i] += length * 0.5;
        weights[i + 1] += length * 0.5;
        measure += length;
    }
    return measure;
}

double AddTriangleWeights(const std::vector<Point>& points,
                          size_t i0, size_t i1, size_t i2,
                          std::vector<double>& weights) {
    const double area = TriangleArea(points[i0], points[i1], points[i2]);
    const double share = area / 3.0;
    weights[i0] += share;
    weights[i1] += share;
    weights[i2] += share;
    return area;
}

double AddTetraWeights(const std::vector<Point>& points,
                       size_t i0, size_t i1, size_t i2, size_t i3,
                       std::vector<double>& weights) {
    const double volume =
            SignedTetraVolume(points[i0], points[i1], points[i2], points[i3]);
    const double share = volume * 0.25;
    weights[i0] += share;
    weights[i1] += share;
    weights[i2] += share;
    weights[i3] += share;
    return volume;
}

// Build the same linear-integration weights and measure used by ParaView's
// vtkIntegrationLinearStrategy.  The measure must come from this very same
// decomposition: normalizing these weights to CellSize's independently
// computed volume changes point integrals on warped 3-D cells.
bool ComputeIntegrationWeights(int dimension, const std::vector<Point>& points,
                               std::vector<double>& weights, double& measure) {
    weights.assign(points.size(), 0.0);
    measure = 0.0;
    if (points.empty()) return false;

    if (dimension == 1 && points.size() >= 2) {
        measure = AddLineWeights(points, weights);
    } else if (dimension == 2 && points.size() >= 3) {
        for (size_t i = 1; i + 1 < points.size(); ++i) {
            measure += AddTriangleWeights(points, 0, i, i + 1, weights);
        }
    } else if (dimension == 3) {
        switch (points.size()) {
            case 4:
                measure = AddTetraWeights(points, 0, 1, 2, 3, weights);
                break;
            case 5: { // pyramid: ParaView splits across the shorter base diagonal
                const double diagonal02 = SquaredDistance(points[0], points[2]);
                const double diagonal13 = SquaredDistance(points[1], points[3]);
                if (diagonal02 < diagonal13) {
                    measure += AddTetraWeights(points, 0, 1, 2, 4, weights);
                    measure += AddTetraWeights(points, 0, 2, 3, 4, weights);
                } else {
                    measure += AddTetraWeights(points, 0, 1, 3, 4, weights);
                    measure += AddTetraWeights(points, 1, 2, 3, 4, weights);
                }
                break;
            }
            case 6: // prism: 0,1,2 and 3,4,5
                measure += AddTetraWeights(points, 0, 1, 2, 3, weights);
                measure += AddTetraWeights(points, 1, 4, 5, 3, weights);
                measure += AddTetraWeights(points, 1, 3, 5, 2, weights);
                break;
            case 8: // hexahedron
                measure += AddTetraWeights(points, 0, 1, 3, 4, weights);
                measure += AddTetraWeights(points, 1, 4, 5, 6, weights);
                measure += AddTetraWeights(points, 1, 4, 6, 3, weights);
                measure += AddTetraWeights(points, 1, 3, 6, 2, weights);
                measure += AddTetraWeights(points, 3, 6, 7, 4, weights);
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

bool ReuseCellSizeLengthOrArea(int dimension, bool hasCellSizeMeasure,
                               double cellSizeMeasure,
                               std::vector<double>& weights,
                               double& integrationMeasure) {
    if (!hasCellSizeMeasure || !std::isfinite(cellSizeMeasure)) return false;

    // Length and area use the same segment/fan formulas in both filters, so
    // CellSize is authoritative for 1-D/2-D. ParaView always computes 3-D
    // integration from its signed tetrahedralization; never mix CellSize
    // volumes with VTK weights on a per-cell tolerance test.
    if (dimension > 2) return false;

    if (integrationMeasure == 0.0) {
        if (cellSizeMeasure != 0.0) return false;
        return true;
    }
    const double weightScale = cellSizeMeasure / integrationMeasure;
    for (double& weight : weights) weight *= weightScale;
    integrationMeasure = cellSizeMeasure;
    return true;
}

bool ResolveGeometry(const DataObject::Pointer& input,
                     Points::Pointer& points, CellArray::Pointer& cells,
                     bool& isLineOnlySurface) {
    isLineOnlySurface = false;
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH: {
            auto mesh = DynamicCast<SurfaceMesh>(input);
            if (!mesh) return false;
            points = mesh->GetPoints();
            auto faces = mesh->GetFaces();
            if (faces && faces->GetNumberOfCells() > 0) {
                cells = faces;
            } else {
                // Legacy VTK POLYDATA LINES are represented as SurfaceMesh
                // edges by iGame's reader. CellSize intentionally operates on
                // SurfaceMesh faces, so keep that filter unchanged and use the
                // edge connectivity as a local 1-D compatibility path.
                cells = mesh->GetEdges();
                isLineOnlySurface = cells && cells->GetNumberOfCells() > 0;
            }
            return true;
        }
        case IG_VOLUME_MESH: {
            auto mesh = DynamicCast<VolumeMesh>(input);
            if (!mesh) return false;
            points = mesh->GetPoints();
            cells = mesh->GetCells();
            return true;
        }
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            if (!mesh) return false;
            points = mesh->GetPoints();
            cells = mesh->GetCells();
            return true;
        }
        case IG_STRUCTURED_MESH: {
            auto mesh = DynamicCast<StructuredMesh>(input);
            if (!mesh) return false;
            mesh->GenStructuredCellConnectivities();
            points = mesh->GetPoints();
            cells = mesh->GetCells();
            return true;
        }
        default:
            return false;
    }
}

void FindMeasureArrays(AttributeSet* attributes,
                       ArrayObject::Pointer& length,
                       ArrayObject::Pointer& area,
                       ArrayObject::Pointer& volume) {
    if (!attributes) return;
    // Search backwards because CellSizeFilter appends its three result arrays
    // after copying input attributes, which may themselves use these names.
    for (int i = static_cast<int>(attributes->GetNumberOfAttributes()) - 1;
         i >= 0; --i) {
        auto& attr = attributes->GetAttribute(i);
        if (attr.IsNone() || attr.attachmentType != IG_CELL || !attr.pointer) continue;
        const std::string& name = attr.pointer->GetName();
        if (!length && name == "Length") length = attr.pointer;
        else if (!area && name == "Area") area = attr.pointer;
        else if (!volume && name == "Volume") volume = attr.pointer;
    }
}

bool GetCellMeasure(IGsize cellId,
                    const ArrayObject::Pointer& length,
                    const ArrayObject::Pointer& area,
                    const ArrayObject::Pointer& volume,
                    int& dimension, double& measure) {
    const ArrayObject::Pointer arrays[3] = {length, area, volume};
    for (int i = 2; i >= 0; --i) {
        if (!arrays[i] || cellId >= arrays[i]->GetNumberOfElements()) continue;
        const double value = arrays[i]->GetElementValue(cellId, 0);
        if (std::isfinite(value)) {
            dimension = i + 1;
            measure = value;
            return true;
        }
    }
    return false;
}

bool ResolveIntegrationDimension(const DataObject::Pointer& input, IGsize cellId,
                                 const ArrayObject::Pointer& length,
                                 const ArrayObject::Pointer& area,
                                 const ArrayObject::Pointer& volume,
                                 bool isLineOnlySurface,
                                 int& dimension, double& cellSizeMeasure,
                                 bool& hasCellSizeMeasure) {
    hasCellSizeMeasure = false;
    cellSizeMeasure = 0.0;
    if (isLineOnlySurface) {
        dimension = 1;
        return true;
    }
    if (GetCellMeasure(cellId, length, area, volume,
                       dimension, cellSizeMeasure)) {
        hasCellSizeMeasure = true;
        return true;
    }

    // CellSize currently has no IG_POLY_LINE dispatch. Keep that reusable
    // filter untouched and handle ParaView's piecewise-linear integration
    // locally in IntegrateVariables.
    if (input->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
        auto mesh = DynamicCast<UnstructuredMesh>(input);
        if (mesh && mesh->GetCellType(cellId) == IG_POLY_LINE) {
            dimension = 1;
            return true;
        }
    }
    return false;
}

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
            // ParaView's generated geometry measure replaces a same-named
            // cell array in the output.
            cellAttributes.push_back(std::move(accumulator));
        }
    }
}

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

} // namespace

IntegrateVariablesFilter::IntegrateVariablesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

std::string IntegrateVariablesFilter::GetMeasureName() const {
    switch (m_IntegrationDimension) {
        case 1: return "Length";
        case 2: return "Area";
        case 3: return "Volume";
        default: return "";
    }
}

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

    Points::Pointer points;
    CellArray::Pointer cells;
    bool isLineOnlySurface = false;
    if (!ResolveGeometry(input, points, cells, isLineOnlySurface) ||
        !points || !cells) {
        m_Message = "Unsupported input. Integrate Variables requires a surface, volume, unstructured, or structured mesh.";
        return false;
    }
    const IGsize pointCount = points->GetNumberOfPoints();
    const IGsize cellCount = cells->GetNumberOfCells();
    if (pointCount == 0 || cellCount == 0) {
        m_Message = "The input mesh has no points or cells to integrate.";
        return false;
    }

    ArrayObject::Pointer length;
    ArrayObject::Pointer area;
    ArrayObject::Pointer volume;
    if (!isLineOnlySurface) {
        // Reuse CellSizeFilter for supported-cell discovery, dimensions, and
        // final 1-D/2-D measures. Volume integration always uses ParaView's
        // signed tetrahedralization; mixing CellSize and VTK volume definitions
        // across cells produces accumulated errors on distorted hexahedra.
        auto cellSize = CellSizeFilter::New();
        cellSize->SetInput(input);
        if (!cellSize->Execute()) {
            m_Message = "CellSizeFilter failed: " + cellSize->GetMessage();
            return false;
        }
        auto measuredData = cellSize->GetOutput();
        if (!measuredData) {
            m_Message = "CellSizeFilter did not produce an output.";
            return false;
        }

        FindMeasureArrays(measuredData->GetAttributeSet(), length, area, volume);
        if (!length || !area || !volume) {
            m_Message = "CellSizeFilter output is missing Length, Area, or Volume.";
            return false;
        }
    }

    // ParaView integrates only the highest-dimensional cells in a mixed mesh.
    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        int dimension = 0;
        double cellSizeMeasure = 0.0;
        bool hasCellSizeMeasure = false;
        if (ResolveIntegrationDimension(input, cellId, length, area, volume,
                                        isLineOnlySurface,
                                        dimension, cellSizeMeasure,
                                        hasCellSizeMeasure)) {
            m_IntegrationDimension = std::max(m_IntegrationDimension, dimension);
        }
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

    double weightedCenter[3] = {0.0, 0.0, 0.0};
    const igIndex* pointIds = nullptr;
    std::vector<Point> cellPoints;
    std::vector<double> pointWeights;

    for (IGsize cellId = 0; cellId < cellCount; ++cellId) {
        int dimension = 0;
        double cellSizeMeasure = 0.0;
        bool hasCellSizeMeasure = false;
        if (!ResolveIntegrationDimension(input, cellId, length, area, volume,
                                         isLineOnlySurface,
                                         dimension, cellSizeMeasure,
                                         hasCellSizeMeasure) ||
            dimension != m_IntegrationDimension) {
            continue;
        }

        const int cellPointCount = cells->GetCellIds(cellId, pointIds);
        if (cellPointCount <= 0 || !pointIds) continue;
        cellPoints.clear();
        cellPoints.reserve(static_cast<size_t>(cellPointCount));
        bool validIds = true;
        for (int j = 0; j < cellPointCount; ++j) {
            if (pointIds[j] < 0 || static_cast<IGsize>(pointIds[j]) >= pointCount) {
                validIds = false;
                break;
            }
            cellPoints.push_back(points->GetPoint(pointIds[j]));
        }
        double measure = 0.0;
        if (!validIds || !ComputeIntegrationWeights(
                dimension, cellPoints, pointWeights, measure)) {
            continue;
        }
        ReuseCellSizeLengthOrArea(dimension, hasCellSizeMeasure,
                                  cellSizeMeasure, pointWeights, measure);

        ++m_IntegratedCellCount;
        m_IntegratedMeasure += measure;
        for (int localId = 0; localId < cellPointCount; ++localId) {
            const double weight = pointWeights[static_cast<size_t>(localId)];
            const Point& point = cellPoints[static_cast<size_t>(localId)];
            weightedCenter[0] += static_cast<double>(point[0]) * weight;
            weightedCenter[1] += static_cast<double>(point[1]) * weight;
            weightedCenter[2] += static_cast<double>(point[2]) * weight;

            for (auto& accumulator : pointAttributes) {
                for (size_t component = 0; component < accumulator.values.size(); ++component) {
                    accumulator.values[component] +=
                            accumulator.input->GetElementValue(pointIds[localId],
                                                               static_cast<int>(component)) * weight;
                }
            }
        }

        for (auto& accumulator : cellAttributes) {
            for (size_t component = 0; component < accumulator.values.size(); ++component) {
                accumulator.values[component] +=
                        accumulator.input->GetElementValue(cellId,
                                                           static_cast<int>(component)) * measure;
            }
        }
    }

    if (m_IntegratedCellCount == 0) {
        m_Message = "No cells of the highest integration dimension could be integrated.";
        return false;
    }

    Point outputPoint(0.0f, 0.0f, 0.0f);
    if (m_IntegratedMeasure != 0.0) {
        outputPoint[0] = static_cast<float>(weightedCenter[0] / m_IntegratedMeasure);
        outputPoint[1] = static_cast<float>(weightedCenter[1] / m_IntegratedMeasure);
        outputPoint[2] = static_cast<float>(weightedCenter[2] / m_IntegratedMeasure);
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
