#include "iGameExtractEdgesFilter.h"

// —— 各数据类型头文件 ——
#include "iGameCellArray.h"   // 单元数组（连接关系）
#include "iGameCellType.h"    // 单元类型枚举（IG_LINE / IG_TRIANGLE / IG_TETRA ...）
#include "iGameFlatArray.h"   // FlatArray 模板（DoubleArray/IntArray 等）
#include "iGameSurfaceMesh.h" // 表面网格
#include "iGameVolumeMesh.h"  // 体网格

IGAME_NAMESPACE_BEGIN

ExtractEdgesFilter::ExtractEdgesFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
}

bool ExtractEdgesFilter::Execute() {
    UpdateProgress(0);
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }
    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }
    return ExecuteWithPointSet(input);
}

bool ExtractEdgesFilter::ExecuteWithPointSet(DataObject::Pointer input) {
    // 框架现成的转换函数：把任意网格转成 UnstructuredMesh 表示。
    UnstructuredMesh::Pointer um =
        UnstructuredMesh::TransDataObjToUnstructuredMesh(input);

    if (!um || um->GetNumberOfCells() == 0) {
        SetOutput(0, input);
        UpdateProgress(1);
        return true;
    }

    UnstructuredMesh::Pointer outMesh = UnstructuredMesh::New();
    if (!ExtractEdgesFromMesh(um, outMesh)) {
        SetOutput(0, input);
        UpdateProgress(1);
        return false;
    }
    outMesh->SetPoints(um->GetPoints());
    outMesh->SetAttributeSet(um->GetAttributeSet());

    SetOutput(0, outMesh);
    UpdateProgress(1);
    return true;
}

bool ExtractEdgesFilter::ExtractEdgesFromMesh(
    UnstructuredMesh::Pointer input, UnstructuredMesh::Pointer output) {

    CellArray::Pointer edges = CellArray::New();
    UnsignedIntArray::Pointer edgeTypes = UnsignedIntArray::New();

    auto cells = input->GetCells();
    auto types = input->GetCellTypes();
    if (!cells || !types) { return false; }

    IGsize numCells = cells->GetNumberOfCells();

    std::set<std::pair<igIndex, igIndex>> seen;

    igIndex vhs[IGAME_CELL_MAX_SIZE] = { 0 };

    edges->Reserve(numCells * 3);

    for (IGsize cid = 0; cid < numCells; ++cid) {
        int vcnt = cells->GetCellIds(cid, vhs);
        if (vcnt < 2) { continue; }

        IGenum cellType = types->GetValue(cid);
        if (cellType == IG_LINE || cellType == IG_POLY_LINE) {
            // 单条线段：两个端点就是一条边
            if (cellType == IG_LINE && vcnt == 2) {
                auto key = std::minmax(vhs[0], vhs[1]);
                if (seen.insert(key).second) {
                    edges->AddCellId2(vhs[0], vhs[1]);
                    edgeTypes->AddValue(IG_LINE);
                }
            }
            else if (cellType == IG_POLY_LINE && vcnt > 2) {
                for (int e = 0; e < vcnt - 1; ++e) {
                    auto key = std::minmax(vhs[e], vhs[e + 1]);
                    if (seen.insert(key).second) {
                        edges->AddCellId2(vhs[e], vhs[e + 1]);
                        edgeTypes->AddValue(IG_LINE);
                    }
                }
            }
            continue;
        }


        if (cellType == IG_VERTEX || cellType == IG_EMPTY_CELL) {
            continue;
        }

        Cell::Pointer cell = nullptr;
        input->GetCell(cid, cell);
        if (!cell) { continue; }

        int nEdges = cell->GetNumberOfEdges();
        for (int e = 0; e < nEdges; ++e) {
            Cell* edge = cell->GetEdge(e);
            if (!edge || edge->GetCellSize() != 2) { continue; }

            igIndex p0 = edge->GetPointId(0);
            igIndex p1 = edge->GetPointId(1);

            auto key = std::minmax(p0, p1);
            if (seen.insert(key).second) {
                edges->AddCellId2(p0, p1);
                edgeTypes->AddValue(IG_LINE);
            }
        }

        if (cid % 10000 == 0) {
            UpdateProgress(static_cast<double>(cid) / numCells * 0.5);
        }
    }
    output->SetCells(edges, edgeTypes);
    return true;
}

IGAME_NAMESPACE_END
