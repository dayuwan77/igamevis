#include "iGameDecimatePolylineFilter.h"

#include "iGameAttributeSet.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

struct InputPolyline {
    std::vector<igIndex> pointIds;
    IGsize sourceCellId{0};
};

struct VertexNode {
    igIndex pointId{-1};
    int previous{-1};
    int next{-1};
    bool removable{true};
    bool alive{true};
};

// vtkDecimatePolylineFilter uses vtkPriorityQueue.  Its equal-priority heap
// ordering is observable for symmetric polylines, so reproduce that ordering
// instead of relying on std::priority_queue's implementation-defined ties.
class VtkPriorityQueue {
public:
    void Insert(double priority, int id) {
        if (id < static_cast<int>(m_Locations.size()) && m_Locations[id] != -1) return;
        if (id >= static_cast<int>(m_Locations.size())) m_Locations.resize(id + 1, -1);

        const int indexAtBottom = ++m_MaxId;
        if (m_MaxId >= static_cast<int>(m_Items.size())) m_Items.resize(m_MaxId + 1);
        m_Items[m_MaxId] = {priority, id};
        int index = indexAtBottom;
        m_Locations[id] = index;
        while (index > 0) {
            const int parent = (index - 1) / 2;
            if (m_Items[index].priority >= m_Items[parent].priority) break;
            Swap(index, parent);
            index = parent;
        }
    }

    int Pop(int location = 0) {
        if (m_MaxId < 0 || location < 0 || location > m_MaxId) return -1;

        const int id = m_Items[location].id;
        m_Items[location] = m_Items[m_MaxId];
        m_Locations[m_Items[location].id] = location;
        m_Locations[id] = -1;
        if (--m_MaxId <= 0) return id;

        const int lastParent = (m_MaxId - 1) / 2;
        for (int index = location; index <= lastParent;) {
            const int left = 2 * index + 1;
            const int child = left == m_MaxId ||
                            m_Items[left].priority < m_Items[left + 1].priority
                    ? left
                    : left + 1;
            if (m_Items[index].priority <= m_Items[child].priority) break;
            Swap(index, child);
            index = child;
        }

        // vtkPriorityQueue starts this second pass again at the original
        // deletion location, rather than where the downward pass finished.
        for (int index = location; index > 0;) {
            const int parent = (index - 1) / 2;
            if (m_Items[index].priority >= m_Items[parent].priority) break;
            Swap(index, parent);
            index = parent;
        }
        return id;
    }

    void DeleteId(int id) {
        if (id >= 0 && id < static_cast<int>(m_Locations.size()) && m_Locations[id] != -1) {
            Pop(m_Locations[id]);
        }
    }

    void Reset() {
        m_MaxId = -1;
        std::fill(m_Locations.begin(), m_Locations.end(), -1);
    }

private:
    struct Item {
        double priority;
        int id;
    };

    void Swap(int first, int second) {
        std::swap(m_Items[first], m_Items[second]);
        m_Locations[m_Items[first].id] = first;
        m_Locations[m_Items[second].id] = second;
    }

    std::vector<Item> m_Items;
    std::vector<int> m_Locations;
    int m_MaxId{-1};
};

ArrayObject::Pointer CreateArrayLike(const ArrayObject::Pointer& source) {
    if (!source) return nullptr;
    switch (source->GetArrayType()) {
    case IG_FloatArray: return FloatArray::New();
    case IG_DoubleArray: return DoubleArray::New();
    case IG_IntArray: return IntArray::New();
    case IG_UnsignedIntArray: return UnsignedIntArray::New();
    case IG_CharArray: return CharArray::New();
    case IG_UnsignedCharArray: return UnsignedCharArray::New();
    case IG_ShortArray: return ShortArray::New();
    case IG_UnsignedShortArray: return UnsignedShortArray::New();
    case IG_LongLongArray: return LongLongArray::New();
    case IG_UnsignedLongLongArray: return UnsignedLongLongArray::New();
    default: return nullptr;
    }
}

template<typename TArray, typename TValue>
void CopyArrayElements(ArrayObject::Pointer& destination,
                       const ArrayObject::Pointer& source,
                       const std::vector<IGsize>& sourceIndices) {
    auto sourceArray = DynamicCast<TArray>(source);
    auto destinationArray = DynamicCast<TArray>(destination);
    if (!sourceArray || !destinationArray) return;

    const int dimension = source->GetDimension();
    const TValue* sourceValues = sourceArray->RawPointer();
    TValue* destinationValues = destinationArray->RawPointer();
    for (IGsize outputId = 0; outputId < static_cast<IGsize>(sourceIndices.size()); ++outputId) {
        const IGsize sourceOffset = sourceIndices[outputId] * static_cast<IGsize>(dimension);
        const IGsize destinationOffset = outputId * static_cast<IGsize>(dimension);
        std::copy_n(sourceValues + sourceOffset, dimension, destinationValues + destinationOffset);
    }
}

void CopyArrayByIndex(ArrayObject::Pointer& destination,
                      const ArrayObject::Pointer& source,
                      const std::vector<IGsize>& sourceIndices) {
    switch (source->GetArrayType()) {
    case IG_FloatArray:
        CopyArrayElements<FloatArray, float>(destination, source, sourceIndices);
        break;
    case IG_DoubleArray:
        CopyArrayElements<DoubleArray, double>(destination, source, sourceIndices);
        break;
    case IG_IntArray:
        CopyArrayElements<IntArray, int>(destination, source, sourceIndices);
        break;
    case IG_UnsignedIntArray:
        CopyArrayElements<UnsignedIntArray, unsigned int>(destination, source, sourceIndices);
        break;
    case IG_CharArray:
        CopyArrayElements<CharArray, char>(destination, source, sourceIndices);
        break;
    case IG_UnsignedCharArray:
        CopyArrayElements<UnsignedCharArray, unsigned char>(destination, source, sourceIndices);
        break;
    case IG_ShortArray:
        CopyArrayElements<ShortArray, short>(destination, source, sourceIndices);
        break;
    case IG_UnsignedShortArray:
        CopyArrayElements<UnsignedShortArray, unsigned short>(destination, source, sourceIndices);
        break;
    case IG_LongLongArray:
        CopyArrayElements<LongLongArray, long long>(destination, source, sourceIndices);
        break;
    case IG_UnsignedLongLongArray:
        CopyArrayElements<UnsignedLongLongArray, unsigned long long>(destination, source, sourceIndices);
        break;
    default: break;
    }
}

bool IndicesFitArray(const ArrayObject::Pointer& array, const std::vector<IGsize>& sourceIndices) {
    if (!array) return false;
    const IGsize tupleCount = array->GetNumberOfElements();
    for (IGsize sourceId : sourceIndices) {
        if (sourceId >= tupleCount) return false;
    }
    return true;
}

bool EdgesMatchFaceTopology(SurfaceMesh::Pointer surface, CellArray* edges) {
    auto faces = surface ? surface->GetFaces() : nullptr;
    if (!faces || faces->GetNumberOfCells() == 0 || !edges) return false;

    using EdgeKey = std::pair<igIndex, igIndex>;
    auto makeEdgeKey = [](igIndex first, igIndex second) {
        return first < second ? EdgeKey{first, second} : EdgeKey{second, first};
    };

    std::vector<EdgeKey> faceTopology;
    for (IGsize faceId = 0; faceId < faces->GetNumberOfCells(); ++faceId) {
        const igIndex* ids = nullptr;
        const int size = faces->GetCellIds(faceId, ids);
        if (!ids || size < 3) continue;
        for (int i = 0; i < size; ++i) {
            faceTopology.push_back(makeEdgeKey(ids[i], ids[(i + 1) % size]));
        }
    }
    std::sort(faceTopology.begin(), faceTopology.end());
    faceTopology.erase(std::unique(faceTopology.begin(), faceTopology.end()), faceTopology.end());

    std::vector<EdgeKey> currentEdges;
    currentEdges.reserve(edges->GetNumberOfCells());
    for (IGsize edgeId = 0; edgeId < edges->GetNumberOfCells(); ++edgeId) {
        const igIndex* ids = nullptr;
        const int size = edges->GetCellIds(edgeId, ids);
        if (!ids || size != 2) return false;
        currentEdges.push_back(makeEdgeKey(ids[0], ids[1]));
    }
    std::sort(currentEdges.begin(), currentEdges.end());
    currentEdges.erase(std::unique(currentEdges.begin(), currentEdges.end()), currentEdges.end());
    return currentEdges == faceTopology;
}

void CopyAttributes(DataObject::Pointer input,
                    UnstructuredMesh::Pointer output,
                    const std::vector<IGsize>& pointSourceIds,
                    const std::vector<IGsize>& cellSourceIds) {
    auto outputAttributes = AttributeSet::New();
    auto inputAttributes = input ? input->GetAttributeSet() : nullptr;
    if (!inputAttributes) {
        output->SetAttributeSet(outputAttributes);
        return;
    }

    auto allAttributes = inputAttributes->GetAllAttributes();
    if (!allAttributes) {
        output->SetAttributeSet(outputAttributes);
        return;
    }

    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
        auto& attribute = allAttributes->GetElement(i);
        if (attribute.isDeleted || !attribute.pointer) continue;

        const std::vector<IGsize>* sourceIndices = nullptr;
        if (attribute.attachmentType == IG_POINT) {
            sourceIndices = &pointSourceIds;
        } else if (attribute.attachmentType == IG_CELL) {
            sourceIndices = &cellSourceIds;
        } else {
            continue;
        }

        if (!IndicesFitArray(attribute.pointer, *sourceIndices)) continue;
        auto outputArray = CreateArrayLike(attribute.pointer);
        if (!outputArray) continue;

        outputArray->SetName(attribute.pointer->GetName());
        outputArray->SetDimension(attribute.pointer->GetDimension());
        outputArray->Resize(static_cast<IGsize>(sourceIndices->size()));
        CopyArrayByIndex(outputArray, attribute.pointer, *sourceIndices);

        const IGsize attributeId = outputAttributes->AddAttribute(
                attribute.type, attribute.attachmentType, outputArray);
        if (attributeId != static_cast<IGsize>(-1)) {
            outputAttributes->GetAttribute(attributeId).UpdateAllDataRange();
        }
    }
    output->SetAttributeSet(outputAttributes);
}

bool CollectInputPolylines(DataObject::Pointer input,
                           PointSet::Pointer& pointSet,
                           std::vector<InputPolyline>& polylines) {
    auto surface = DynamicCast<SurfaceMesh>(input);
    if (!surface || !surface->GetPoints() || surface->GetNumberOfPoints() == 0) return false;

    auto edges = surface->GetEdges();
    if (!edges) return false;

    // Rendering a polygon-only SurfaceMesh may lazily populate GetEdges() from
    // face topology. vtkDecimatePolylineFilter only consumes vtkPolyData::Lines,
    // never polygon edges, so ignore that exact derived-edge set. If explicit
    // LINES coexist with POLYGONS, the reader keeps a different edge array and
    // those line cells remain valid input.
    if (EdgesMatchFaceTopology(surface, edges)) return false;

    pointSet = surface;
    const IGsize numberOfEdges = edges->GetNumberOfCells();
    polylines.reserve(numberOfEdges);
    for (IGsize cellId = 0; cellId < numberOfEdges; ++cellId) {
        const igIndex* ids = nullptr;
        const int size = edges->GetCellIds(cellId, ids);
        if (size < 2 || !ids) continue;
        polylines.push_back({std::vector<igIndex>(ids, ids + size), cellId});
    }

    if (polylines.empty()) return false;
    const IGsize numberOfPoints = pointSet->GetNumberOfPoints();
    for (const auto& polyline : polylines) {
        for (igIndex pointId : polyline.pointIds) {
            if (pointId < 0 || static_cast<IGsize>(pointId) >= numberOfPoints) return false;
        }
    }
    return true;
}

double ComputeDistanceError(const Points::Pointer& points,
                            const std::vector<VertexNode>& vertices,
                            int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const Point& origin = points->GetPoint(current.pointId);
    const Point& p1 = points->GetPoint(vertices[current.previous].pointId);
    const Point& p2 = points->GetPoint(vertices[current.next].pointId);

    Vector3d neighbourLine = Vector3d(p1) - Vector3d(p2);
    const double lineLength = neighbourLine.norm();
    // vtkDecimatePolylineDistanceStrategy defines this degenerate case as zero.
    if (lineLength == 0.0) return 0.0;

    // Keep vtkLine::DistanceToLine's operation order. Dividing each component
    // before the dot product matters for exact priority ordering when two
    // candidates have almost equal errors.
    neighbourLine /= lineLength;
    const Vector3d fromP1 = Vector3d(origin) - Vector3d(p1);
    const double projection = DotProduct(fromP1, neighbourLine);
    // vtkLine::DistanceToLine(x, p1, p2) returns squared distance to the
    // infinite line (not to the finite segment).
    return fromP1.squaredNorm() - projection * projection;
}

double ComputeAngleError(const Points::Pointer& points,
                         const std::vector<VertexNode>& vertices,
                         int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const Vector3d origin(points->GetPoint(current.pointId));
    const Vector3d p1(points->GetPoint(vertices[current.previous].pointId));
    const Vector3d p2(points->GetPoint(vertices[current.next].pointId));
    const Vector3d first = origin - p1;
    const Vector3d second = origin - p2;
    const double normProduct = first.norm() * second.norm();
    if (normProduct <= 0.0) return std::numeric_limits<double>::max();
    return DotProduct(first, second) / normProduct;
}

double ComputeCustomFieldError(const ArrayObject::Pointer& field,
                               const std::vector<VertexNode>& vertices,
                               int vertexIndex) {
    const VertexNode& current = vertices[vertexIndex];
    const igIndex originId = current.pointId;
    const igIndex firstId = vertices[current.previous].pointId;
    const igIndex secondId = vertices[current.next].pointId;
    double error = 0.0;
    for (int component = 0; component < field->GetDimension(); ++component) {
        const double origin = field->GetElementValue(originId, component);
        const double first = field->GetElementValue(firstId, component);
        const double second = field->GetElementValue(secondId, component);
        error = std::max(error, std::abs(origin - first));
        error = std::max(error, std::abs(origin - second));
        error = std::max(error, std::abs(first - second));
    }
    return error;
}

double ComputeError(const Points::Pointer& points,
                    const std::vector<VertexNode>& vertices,
                    int vertexIndex,
                    DecimatePolylineFilter::DecimationStrategy strategy,
                    const ArrayObject::Pointer& customField) {
    switch (strategy) {
    case DecimatePolylineFilter::DecimationStrategy::Angle:
        return ComputeAngleError(points, vertices, vertexIndex);
    case DecimatePolylineFilter::DecimationStrategy::CustomField:
        return ComputeCustomFieldError(customField, vertices, vertexIndex);
    case DecimatePolylineFilter::DecimationStrategy::Distance:
        return ComputeDistanceError(points, vertices, vertexIndex);
    }
    return std::numeric_limits<double>::max();
}

std::vector<igIndex> DecimateOnePolyline(const Points::Pointer& points,
                                         const std::vector<igIndex>& inputIds,
                                         double targetReduction,
                                         double maximumError,
                                         DecimatePolylineFilter::DecimationStrategy strategy,
                                         const ArrayObject::Pointer& customField) {
    const int originalSize = static_cast<int>(inputIds.size());
    std::vector<VertexNode> vertices(originalSize);
    for (int i = 0; i < originalSize; ++i) {
        vertices[i].pointId = inputIds[i];
        vertices[i].previous = i > 0 ? i - 1 : -1;
        vertices[i].next = i + 1 < originalSize ? i + 1 : -1;
    }
    vertices.front().removable = false;
    vertices.back().removable = false;

    const bool isLoop = inputIds.front() == inputIds.back();
    VtkPriorityQueue queue;

    auto updateQueueEntry = [&](int vertexIndex) {
        VertexNode& vertex = vertices[vertexIndex];
        if (!vertex.alive || !vertex.removable || vertex.previous < 0 || vertex.next < 0) return;
        const double error = ComputeError(points, vertices, vertexIndex, strategy, customField);
        queue.DeleteId(vertexIndex);
        if (error <= maximumError) queue.Insert(error, vertexIndex);
    };

    if (originalSize > 2) {
        for (int i = 1; i + 1 < originalSize; ++i) updateQueueEntry(i);
    }

    int currentSize = originalSize;
    const int minimumSize = isLoop ? 3 : 2;
    while (1.0 - static_cast<double>(currentSize) / static_cast<double>(originalSize) < targetReduction &&
           currentSize > minimumSize) {
        const int candidate = queue.Pop();
        if (candidate < 0) break;

        VertexNode& removed = vertices[candidate];
        const int previous = removed.previous;
        const int next = removed.next;
        removed.alive = false;
        vertices[previous].next = next;
        vertices[next].previous = previous;
        --currentSize;

        if (vertices[previous].removable) updateQueueEntry(previous);
        if (vertices[next].removable) updateQueueEntry(next);
    }

    std::vector<igIndex> outputIds;
    outputIds.reserve(currentSize);
    for (int vertexIndex = 0; vertexIndex >= 0; vertexIndex = vertices[vertexIndex].next) {
        if (vertices[vertexIndex].alive) outputIds.push_back(vertices[vertexIndex].pointId);
    }
    return outputIds;
}

} // namespace

DecimatePolylineFilter::DecimatePolylineFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void DecimatePolylineFilter::SetTargetReduction(double value) {
    if (std::isnan(value)) value = 0.0;
    m_TargetReduction = std::max(0.0, std::min(1.0, value));
}

void DecimatePolylineFilter::SetMaximumError(double value) {
    if (std::isnan(value)) value = 0.0;
    m_MaximumError = std::max(0.0, std::min(std::numeric_limits<double>::max(), value));
}

bool DecimatePolylineFilter::CanProcessInput(DataObject::Pointer input) {
    PointSet::Pointer pointSet;
    std::vector<InputPolyline> polylines;
    return CollectInputPolylines(input, pointSet, polylines);
}

bool DecimatePolylineFilter::Execute() {
    if (m_DecimationStrategy != DecimationStrategy::Angle &&
        m_DecimationStrategy != DecimationStrategy::CustomField &&
        m_DecimationStrategy != DecimationStrategy::Distance) {
        return false;
    }

    auto input = GetInput(0);
    PointSet::Pointer inputPointSet;
    std::vector<InputPolyline> inputPolylines;
    if (!CollectInputPolylines(input, inputPointSet, inputPolylines)) return false;

    ArrayObject::Pointer customField;
    if (m_DecimationStrategy == DecimationStrategy::CustomField) {
        auto attributes = input ? input->GetAttributeSet() : nullptr;
        if (!attributes || m_CustomFieldName.empty()) return false;
        const auto& attribute = attributes->GetAttribute(m_CustomFieldName);
        if (attribute.IsNone() || attribute.isDeleted || attribute.attachmentType != IG_POINT ||
            !attribute.pointer || !CreateArrayLike(attribute.pointer) ||
            attribute.pointer->GetDimension() < 1 ||
            attribute.pointer->GetNumberOfElements() < inputPointSet->GetNumberOfPoints()) {
            return false;
        }
        customField = attribute.pointer;
    }

    auto output = UnstructuredMesh::New();
    auto outputPoints = Points::New();
    auto outputCells = CellArray::New();
    auto outputTypes = UnsignedIntArray::New();
    outputTypes->SetDimension(1);

    std::unordered_map<igIndex, igIndex> pointIdMap;
    std::vector<IGsize> pointSourceIds;
    std::vector<IGsize> cellSourceIds;
    pointIdMap.reserve(inputPointSet->GetNumberOfPoints());
    cellSourceIds.reserve(inputPolylines.size());

    for (const auto& inputPolyline : inputPolylines) {
        const auto retainedIds = DecimateOnePolyline(
                inputPointSet->GetPoints(), inputPolyline.pointIds, m_TargetReduction, m_MaximumError,
                m_DecimationStrategy, customField);
        if (retainedIds.size() < 2) continue;

        std::vector<igIndex> outputIds;
        outputIds.reserve(retainedIds.size());
        for (igIndex sourcePointId : retainedIds) {
            auto found = pointIdMap.find(sourcePointId);
            if (found == pointIdMap.end()) {
                const igIndex outputPointId = static_cast<igIndex>(outputPoints->GetNumberOfPoints());
                outputPoints->AddPoint(inputPointSet->GetPoint(sourcePointId));
                pointIdMap.emplace(sourcePointId, outputPointId);
                pointSourceIds.push_back(static_cast<IGsize>(sourcePointId));
                outputIds.push_back(outputPointId);
            } else {
                outputIds.push_back(found->second);
            }
        }

        outputCells->AddCellIds(outputIds.data(), static_cast<int>(outputIds.size()));
        outputTypes->AddValue(outputIds.size() == 2 ? IG_LINE : IG_POLY_LINE);
        cellSourceIds.push_back(inputPolyline.sourceCellId);
    }

    if (outputCells->GetNumberOfCells() == 0) return false;
    output->SetPoints(outputPoints);
    output->SetCells(outputCells, outputTypes);
    if (input && !input->GetName().empty()) output->SetName(input->GetName() + "_decimate_polyline");
    CopyAttributes(input, output, pointSourceIds, cellSourceIds);
    SetOutput(output);
    return true;
}

IGAME_NAMESPACE_END
