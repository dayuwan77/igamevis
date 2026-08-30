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

    // 验证是否有重合点
    std::cout << "\n========== BRUTE FORCE TEST ==========" << std::endl;
    std::cout << "Total points: " << numPoints << std::endl;
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

    // 合并重合点（使用暴力搜索）
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

        // 检查退化单元
        bool isDegenerate = false;
        if (m_RemoveDegenerateCells) { isDegenerate = IsCellDegenerateWithIds(newIds, validCount); }

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

    //复制属性数据
    auto inAttrSet = inputMesh->GetAttributeSet();
    if (inAttrSet) {
        auto outAttrSet = AttributeSet::New();
        auto pointAttrs = inAttrSet->GetAllPointAttributes();
        if (pointAttrs) {
            for (int i = 0; i < pointAttrs->GetNumberOfElements(); i++) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.pointer) {
                    auto newArray = DoubleArray::New();
                    newArray->SetName(attr.pointer->GetName());
                    newArray->SetDimension(attr.pointer->GetDimension());
                    newArray->Resize(newPointCount);

                    for (auto& pair: newToOldMap) {
                        igIndex newIdx = pair.first;
                        igIndex oldIdx = pair.second;
                        if (oldIdx >= 0 && oldIdx < numPoints) {
                            double values[16] = {0};
                            attr.pointer->GetElement(oldIdx, values);
                            newArray->SetElement(newIdx, values);
                        }
                    }
                    outAttrSet->AddAttribute(attr.type, attr.attachmentType, newArray);
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
                oldToNewMap[j] = i;
                isMerged[j] = true;
                mergeCount++;
            }
        }

        //// 每处理 5000 个点输出一次进度
        //if (i % 5000 == 0) {
        //    std::cout << "  Progress: " << i << " / " << numPoints << " points processed, found " << mergeCount
        //              << " merges" << std::endl;
        //}
        // 每处理 500 个点更新一次进度条
        if (i % 500 == 0) {
            double progress = (double) i / (double) numPoints;
            this->UpdateProgress(progress);
            std::cout << "  Progress: " << i << " / " << numPoints << " points processed (" << (int) (progress * 100)
                      << "%), found " << mergeCount << " merges" << std::endl;
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

        if (!pointIsUsed[i] && m_CompactPointFields) {
            oldToNewMap[i] = -1;
            continue;
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

IGAME_NAMESPACE_END