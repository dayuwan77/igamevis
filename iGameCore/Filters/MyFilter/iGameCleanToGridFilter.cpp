#include "iGameCleanToGridFilter.h"
#include "iGameCellArray.h"
#include "iGameFlatArray.h"

#include <cmath>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

IGAME_NAMESPACE_BEGIN

CleanToGridFilter::CleanToGridFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

CleanToGridFilter::~CleanToGridFilter() {}

double CleanToGridFilter::ComputeEffectiveTolerance(DataObject::Pointer input) {
    if (m_ToleranceIsAbsolute) { return m_AbsoluteTolerance; }

    auto pointSet = DynamicCast<PointSet>(input);
    if (!pointSet) { return m_ToleranceFraction; }

    auto bbox = pointSet->GetBoundingBox();
    double diag = (bbox.max - bbox.min).norm();

    if (diag < 1e-12) { return m_AbsoluteTolerance; }

    return diag * m_ToleranceFraction;
}

bool CleanToGridFilter::IsCellDegenerateWithIds(const igIndex* pointIds, int numPoints) {
    if (!pointIds || numPoints < 3) return true;

    std::set<igIndex> uniquePoints;
    for (int i = 0; i < numPoints; i++) { uniquePoints.insert(pointIds[i]); }

    int minRequired = (numPoints >= 4) ? 4 : 3;
    return ((int) uniquePoints.size() < minRequired);
}


// 修改点 ①：新增 CreateArrayByType，根据原始类型创建对应数组
ArrayObject::Pointer CreateArrayByType(IGenum dataType) {
    if (dataType == IG_FLOAT) {
        return FloatArray::New();
    } else if (dataType == IG_DOUBLE) {
        return DoubleArray::New();
    } else if (dataType == IG_INT) {
        return IntArray::New();
    } else if (dataType == IG_UNSIGNED_INT) {
        return UnsignedIntArray::New();
    } else if (dataType == IG_UNSIGNED_CHAR) {
        return UnsignedCharArray::New();
    } else {
        // 默认创建 FloatArray
        return FloatArray::New();
    }
}


// 修改点 ②：CloneAttributeArray 动态分配，支持任意分量数
ArrayObject::Pointer CleanToGridFilter::CloneAttributeArray(ArrayObject::Pointer src, igIndex newSize,
                                                            const std::vector<igIndex>& oldToNewMap,
                                                            igIndex numOldPoints) {
    if (!src) return nullptr;

    int dim = src->GetDimension();
    IGenum dataType = src->GetArrayType();

    ArrayObject::Pointer dst = CreateArrayByType(dataType);
    if (!dst) {
        std::cerr << "CloneAttributeArray: Failed to create array of type " << dataType << std::endl;
        return nullptr;
    }

    dst->SetName(src->GetName());
    dst->SetDimension(dim);
    dst->Resize(newSize);

    // 建立 newIdx -> oldIdx 的映射
    std::unordered_map<igIndex, igIndex> newToOldMap;
    for (igIndex i = 0; i < numOldPoints; i++) {
        igIndex newIdx = oldToNewMap[i];
        if (newIdx >= 0 && newIdx < newSize) { newToOldMap[newIdx] = i; }
    }

    // 根据类型复制数据
    if (dataType == IG_FLOAT) {
        auto srcArray = DynamicCast<FloatArray>(src);
        auto dstArray = DynamicCast<FloatArray>(dst);
        if (!srcArray || !dstArray) return dst;

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                for (int d = 0; d < dim; d++) {
                    float val = srcArray->GetValue(oldIdx * dim + d);
                    dstArray->SetValue(newIdx * dim + d, val);
                }
            }
        }
    } else if (dataType == IG_DOUBLE) {
        auto srcArray = DynamicCast<DoubleArray>(src);
        auto dstArray = DynamicCast<DoubleArray>(dst);
        if (!srcArray || !dstArray) return dst;

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                for (int d = 0; d < dim; d++) {
                    double val = srcArray->GetValue(oldIdx * dim + d);
                    dstArray->SetValue(newIdx * dim + d, val);
                }
            }
        }
    } else if (dataType == IG_INT) {
        auto srcArray = DynamicCast<IntArray>(src);
        auto dstArray = DynamicCast<IntArray>(dst);
        if (!srcArray || !dstArray) return dst;

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                for (int d = 0; d < dim; d++) {
                    int val = srcArray->GetValue(oldIdx * dim + d);
                    dstArray->SetValue(newIdx * dim + d, val);
                }
            }
        }
    } else if (dataType == IG_UNSIGNED_INT) {
        auto srcArray = DynamicCast<UnsignedIntArray>(src);
        auto dstArray = DynamicCast<UnsignedIntArray>(dst);
        if (!srcArray || !dstArray) return dst;

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                for (int d = 0; d < dim; d++) {
                    unsigned int val = srcArray->GetValue(oldIdx * dim + d);
                    dstArray->SetValue(newIdx * dim + d, val);
                }
            }
        }
    } else if (dataType == IG_UNSIGNED_CHAR) {
        auto srcArray = DynamicCast<UnsignedCharArray>(src);
        auto dstArray = DynamicCast<UnsignedCharArray>(dst);
        if (!srcArray || !dstArray) return dst;

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                for (int d = 0; d < dim; d++) {
                    unsigned char val = srcArray->GetValue(oldIdx * dim + d);
                    dstArray->SetValue(newIdx * dim + d, val);
                }
            }
        }
    } else {
        // 其他类型，使用通用方法
        auto srcArray = DynamicCast<ArrayObject>(src);
        auto dstArray = DynamicCast<ArrayObject>(dst);
        if (!srcArray || !dstArray) return dst;

        int actualDim = dim;
        std::vector<double> values(actualDim);

        for (auto& pair: newToOldMap) {
            igIndex newIdx = pair.first;
            igIndex oldIdx = pair.second;
            if (oldIdx >= 0 && oldIdx < numOldPoints) {
                srcArray->GetElement(oldIdx, values.data());
                dstArray->SetElement(newIdx, values.data());
            }
        }
    }

    return dst;
}


// MergeCoincidentPointsBruteForce - 暴力搜索合并重合点
bool CleanToGridFilter::MergeCoincidentPointsBruteForce(Points::Pointer points, double tolerance,
                                                        std::vector<igIndex>& oldToNewMap, igIndex& newPointCount,
                                                        const std::vector<bool>& pointIsUsed) {
    if (!points) { return false; }

    igIndex numPoints = points->GetNumberOfPoints();
    if (numPoints == 0) { return true; }

    std::cout << "MergeCoincidentPoints: Using brute force search..." << std::endl;

    // 初始化：每个点默认指向自己
    for (igIndex i = 0; i < numPoints; i++) { oldToNewMap[i] = i; }

    std::vector<bool> isMerged(numPoints, false);
    int mergeCount = 0;

    // 暴力搜索所有点对
    for (igIndex i = 0; i < numPoints; i++) {
        if (isMerged[i]) continue;

        Point p = points->GetPoint(i);

        for (igIndex j = i + 1; j < numPoints; j++) {
            if (isMerged[j]) continue;

            Point q = points->GetPoint(j);
            double dx = (double) p[0] - (double) q[0];
            double dy = (double) p[1] - (double) q[1];
            double dz = (double) p[2] - (double) q[2];
            double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist < tolerance) {
                // 修改点 ③：优先保留被使用的点
                if (pointIsUsed[j] && !pointIsUsed[i]) {
                    // j 被使用，i 未使用 → 把 i 合并到 j
                    oldToNewMap[i] = j;
                    isMerged[i] = true;
                    mergeCount++;
                } else {
                    // 默认：合并 j 到 i
                    oldToNewMap[j] = i;
                    isMerged[j] = true;
                    mergeCount++;
                }
            }
        }

        // 每处理 500 个点更新一次进度条
        if (i % 500 == 0) {
            double progress = (double) i / (double) numPoints;
            this->UpdateProgress(progress);
        }
    }

    // 完成时设为 100%
    this->UpdateProgress(1.0);

    std::cout << "MergeCoincidentPoints: Found " << mergeCount << " points to merge" << std::endl;

    // 压缩索引
    std::unordered_map<igIndex, igIndex> compactMap;
    igIndex currentNewIdx = 0;

    for (igIndex i = 0; i < numPoints; i++) {
        if (isMerged[i]) { continue; }

        // 检查这个代表点是否被使用
        if (!pointIsUsed[i] && m_CompactPointFields) {
            // 检查是否有被合并到它的点
            bool hasMergedPoint = false;
            for (igIndex j = 0; j < numPoints; j++) {
                if (isMerged[j] && oldToNewMap[j] == i) {
                    hasMergedPoint = true;
                    break;
                }
            }
            if (!hasMergedPoint) {
                oldToNewMap[i] = -1;
                continue;
            }
        }

        if (compactMap.find(i) == compactMap.end()) { compactMap[i] = currentNewIdx++; }
    }

    // 应用压缩映射
    for (igIndex i = 0; i < numPoints; i++) {
        if (isMerged[i]) {
            igIndex target = oldToNewMap[i];
            if (target >= 0 && target < numPoints && compactMap.find(target) != compactMap.end()) {
                oldToNewMap[i] = compactMap[target];
            } else {
                oldToNewMap[i] = -1;
            }
        } else {
            if (compactMap.find(i) != compactMap.end()) {
                oldToNewMap[i] = compactMap[i];
            } else {
                oldToNewMap[i] = -1;
            }
        }
    }

    newPointCount = currentNewIdx;
    std::cout << "MergeCoincidentPoints: Final point count = " << newPointCount << std::endl;

    return true;
}


// 修改点 ④：新增 CopyCellAttributes 复制单元属性
void CopyCellAttributes(ArrayObject::Pointer srcArray, ArrayObject::Pointer dstArray, igIndex validCellCount,
                        const std::vector<igIndex>& oldToNewMap, igIndex newPointCount, bool removeDegenerateCells,
                        UnstructuredMesh::Pointer inputMesh, CellArray::Pointer cellArray) {
    if (!srcArray || !dstArray) return;

    int dim = srcArray->GetDimension();
    IGenum dataType = srcArray->GetArrayType();

    igIndex cellIds[IGAME_CELL_MAX_SIZE];
    igIndex newIds[IGAME_CELL_MAX_SIZE];

    igIndex validCellIdx = 0;
    for (igIndex cellIdx = 0; cellIdx < inputMesh->GetNumberOfCells(); cellIdx++) {
        int cellSize = cellArray->GetCellSize(cellIdx);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }

        int actualSize = inputMesh->GetCellPointIds(cellIdx, cellIds);
        if (actualSize <= 0) { continue; }

        bool hasInvalidPoint = false;
        int validCount = 0;
        for (int j = 0; j < actualSize && j < IGAME_CELL_MAX_SIZE; j++) {
            igIndex newIdx = oldToNewMap[cellIds[j]];
            if (newIdx < 0 || newIdx >= newPointCount) {
                hasInvalidPoint = true;
                break;
            }
            newIds[validCount++] = newIdx;
        }

        if (hasInvalidPoint || validCount < 3) { continue; }

        bool isDegenerate = false;
        if (removeDegenerateCells) {
            std::set<igIndex> uniquePoints;
            for (int j = 0; j < validCount; j++) { uniquePoints.insert(newIds[j]); }
            int minRequired = (validCount >= 4) ? 4 : 3;
            isDegenerate = ((int) uniquePoints.size() < minRequired);
        }

        if (isDegenerate) { continue; }

        // 复制单元属性
        if (dataType == IG_FLOAT) {
            auto src = DynamicCast<FloatArray>(srcArray);
            auto dst = DynamicCast<FloatArray>(dstArray);
            if (src && dst) {
                for (int d = 0; d < dim; d++) {
                    dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
                }
            }
        } else if (dataType == IG_DOUBLE) {
            auto src = DynamicCast<DoubleArray>(srcArray);
            auto dst = DynamicCast<DoubleArray>(dstArray);
            if (src && dst) {
                for (int d = 0; d < dim; d++) {
                    dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
                }
            }
        } else if (dataType == IG_INT) {
            auto src = DynamicCast<IntArray>(srcArray);
            auto dst = DynamicCast<IntArray>(dstArray);
            if (src && dst) {
                for (int d = 0; d < dim; d++) {
                    dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
                }
            }
        } else if (dataType == IG_UNSIGNED_INT) {
            auto src = DynamicCast<UnsignedIntArray>(srcArray);
            auto dst = DynamicCast<UnsignedIntArray>(dstArray);
            if (src && dst) {
                for (int d = 0; d < dim; d++) {
                    dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
                }
            }
        } else if (dataType == IG_UNSIGNED_CHAR) {
            auto src = DynamicCast<UnsignedCharArray>(srcArray);
            auto dst = DynamicCast<UnsignedCharArray>(dstArray);
            if (src && dst) {
                for (int d = 0; d < dim; d++) {
                    dst->SetValue(validCellIdx * dim + d, src->GetValue(cellIdx * dim + d));
                }
            }
        } else {
            // 其他类型，使用通用方法
            std::vector<double> values(dim);
            srcArray->GetElement(cellIdx, values.data());
            dstArray->SetElement(validCellIdx, values.data());
        }

        validCellIdx++;
    }
}


// Execute - 核心执行函数
bool CleanToGridFilter::Execute() {
    std::cout << "========== CleanToGridFilter::Execute() START ==========" << std::endl;

    // 获取输入数据
    auto input = this->GetInput(0);
    if (!input) {
        std::cerr << "CleanToGridFilter: No input data!" << std::endl;
        return false;
    }

    // 转换为非结构网格
    auto inputMesh = UnstructuredMesh::TransDataObjToUnstructuredMesh(input);
    if (!inputMesh) {
        std::cerr << "CleanToGridFilter: Failed to convert to UnstructuredMesh!" << std::endl;
        return false;
    }

    auto points = inputMesh->GetPoints();
    auto cellArray = inputMesh->GetCells();
    auto typeArray = inputMesh->GetCellTypes();

    igIndex numPoints = inputMesh->GetNumberOfPoints();
    igIndex numCells = inputMesh->GetNumberOfCells();

    if (numPoints == 0 || numCells == 0) {
        std::cerr << "CleanToGridFilter: Mesh is empty!" << std::endl;
        return false;
    }

    std::cout << "CleanToGridFilter: Input mesh - " << numPoints << " points, " << numCells << " cells" << std::endl;

    double tolerance = ComputeEffectiveTolerance(input);
    std::cout << "CleanToGridFilter: Tolerance = " << tolerance << std::endl;

    // 检查哪些点被单元引用
    std::vector<bool> pointIsUsed(numPoints, false);
    igIndex cellIds[IGAME_CELL_MAX_SIZE];

    for (igIndex i = 0; i < numCells; i++) {
        int cellSize = cellArray->GetCellSize(i);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }

        int actualSize = inputMesh->GetCellPointIds(i, cellIds);
        for (int j = 0; j < actualSize; j++) {
            if (cellIds[j] >= 0 && cellIds[j] < numPoints) { pointIsUsed[cellIds[j]] = true; }
        }
    }

    // 暴力测试验证重合点
    std::cout << "\n========== BRUTE FORCE TEST ==========" << std::endl;
    std::cout << "Total points: " << numPoints << std::endl;
    std::cout << "Total cells: " << numCells << std::endl;
    std::cout << "Tolerance: " << tolerance << std::endl;

    int bruteMergeCount = 0;
    std::vector<bool> bruteMerged(numPoints, false);

    int testLimit = (std::min)((igIndex) 5000, numPoints);
    for (igIndex i = 0; i < testLimit; i++) {
        if (bruteMerged[i]) continue;
        Point p = points->GetPoint(i);
        for (igIndex j = i + 1; j < testLimit; j++) {
            if (bruteMerged[j]) continue;
            Point q = points->GetPoint(j);
            double dx = (double) p[0] - (double) q[0];
            double dy = (double) p[1] - (double) q[1];
            double dz = (double) p[2] - (double) q[2];
            double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist < tolerance) {
                bruteMergeCount++;
                bruteMerged[j] = true;
                if (bruteMergeCount <= 10) {
                    std::cout << "Found duplicate: " << i << " and " << j << ", distance = " << dist << std::endl;
                }
                break;
            }
        }
    }
    std::cout << "Brute force found " << bruteMergeCount << " duplicate points (first 5000 points)" << std::endl;
    std::cout << "========== BRUTE FORCE TEST END ==========\n" << std::endl;

    // 合并重合点
    std::vector<igIndex> oldToNewMap(numPoints, -1);
    igIndex newPointCount = numPoints;

    if (m_MergePoints && tolerance > 0) {
        std::cout << "CleanToGridFilter: Starting merge (brute force)..." << std::endl;
        if (!MergeCoincidentPointsBruteForce(points, tolerance, oldToNewMap, newPointCount, pointIsUsed)) {
            std::cerr << "CleanToGridFilter: Merge failed!" << std::endl;
            return false;
        }
        std::cout << "CleanToGridFilter: After merge - " << newPointCount << " points" << std::endl;
    } else {
        for (igIndex i = 0; i < numPoints; i++) { oldToNewMap[i] = i; }
        newPointCount = numPoints;
    }

    // 创建输出网格的点
    auto outputMesh = UnstructuredMesh::New();
    auto outPoints = Points::New();
    outPoints->Resize(newPointCount);

    std::unordered_map<igIndex, igIndex> newToOldMap;
    for (igIndex i = 0; i < numPoints; i++) {
        igIndex newIdx = oldToNewMap[i];
        if (newIdx >= 0 && newIdx < newPointCount) {
            if (newToOldMap.find(newIdx) == newToOldMap.end()) { newToOldMap[newIdx] = i; }
        }
    }

    for (auto& pair: newToOldMap) {
        igIndex newIdx = pair.first;
        igIndex oldIdx = pair.second;
        Point p = points->GetPoint(oldIdx);
        outPoints->SetPoint(newIdx, p[0], p[1], p[2]);
    }
    outputMesh->SetPoints(outPoints);

    // 更新单元索引并移除退化单元
    auto outCells = CellArray::New();
    auto outTypes = UnsignedIntArray::New();
    igIndex validCellCount = 0;
    igIndex degenerateCount = 0;

    igIndex newIds[IGAME_CELL_MAX_SIZE];

    for (igIndex i = 0; i < numCells; i++) {
        int cellSize = cellArray->GetCellSize(i);
        if (cellSize <= 0 || static_cast<IGsize>(cellSize) > IGAME_CELL_MAX_SIZE) { continue; }

        int actualSize = inputMesh->GetCellPointIds(i, cellIds);
        if (actualSize <= 0) { continue; }

        bool hasInvalidPoint = false;
        int validCount = 0;

        for (int j = 0; j < actualSize && j < IGAME_CELL_MAX_SIZE; j++) {
            igIndex newIdx = oldToNewMap[cellIds[j]];
            if (newIdx < 0 || newIdx >= newPointCount) {
                hasInvalidPoint = true;
                break;
            }
            newIds[validCount++] = newIdx;
        }

        if (hasInvalidPoint || validCount < 3) { continue; }

        bool isDegenerate = false;
        if (m_RemoveDegenerateCells) {
            std::set<igIndex> uniquePoints;
            for (int j = 0; j < validCount; j++) { uniquePoints.insert(newIds[j]); }
            int minRequired = (validCount >= 4) ? 4 : 3;
            isDegenerate = ((int) uniquePoints.size() < minRequired);
        }

        if (isDegenerate) {
            degenerateCount++;
            continue;
        }

        outCells->AddCellIds(newIds, validCount);
        outTypes->AddValue(typeArray->GetValue(i));
        validCellCount++;
    }

    outputMesh->SetCells(outCells, outTypes);

    std::cout << "CleanToGridFilter: After processing - " << validCellCount << " valid cells" << std::endl;
    if (degenerateCount > 0) {
        std::cout << "CleanToGridFilter: Removed " << degenerateCount << " degenerate cells" << std::endl;
    }


    // 修改点 ⑤：复制点属性 + 单元属性
    auto inAttrSet = inputMesh->GetAttributeSet();
    if (inAttrSet) {
        auto outAttrSet = AttributeSet::New();

        // 复制点属性
        auto pointAttrs = inAttrSet->GetAllPointAttributes();
        if (pointAttrs) {
            for (int i = 0; i < pointAttrs->GetNumberOfElements(); i++) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.pointer && !attr.isDeleted) {
                    auto newArray = CloneAttributeArray(attr.pointer, newPointCount, oldToNewMap, numPoints);
                    if (newArray) { outAttrSet->AddAttribute(attr.type, attr.attachmentType, newArray); }
                }
            }
        }

        // 复制单元属性（CellData） 
        auto cellAttrs = inAttrSet->GetAllCellAttributes();
        if (cellAttrs) {
            for (int i = 0; i < cellAttrs->GetNumberOfElements(); i++) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.pointer && !attr.isDeleted) {
                    auto srcArray = attr.pointer;
                    int dim = srcArray->GetDimension();
                    IGenum dataType = srcArray->GetArrayType();

                    // 创建目标数组
                    ArrayObject::Pointer dstArray = CreateArrayByType(dataType);
                    if (!dstArray) { continue; }

                    dstArray->SetName(srcArray->GetName());
                    dstArray->SetDimension(dim);
                    dstArray->Resize(validCellCount);

                    // 复制单元属性
                    CopyCellAttributes(srcArray, dstArray, validCellCount, oldToNewMap, newPointCount,
                                       m_RemoveDegenerateCells, inputMesh, cellArray);

                    outAttrSet->AddAttribute(attr.type, attr.attachmentType, dstArray);
                }
            }
        }

        outputMesh->SetAttributeSet(outAttrSet);
    }

    this->SetOutput(0, outputMesh);

    std::cout << "CleanToGridFilter: Done!" << std::endl;
    std::cout << "========== CleanToGridFilter::Execute() END ==========" << std::endl;
    return true;
}

IGAME_NAMESPACE_END