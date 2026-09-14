#include "iGameCountCellVerticesFilter.h"

// —— 下面这些头文件提供我们用到的数据类型 ——
#include "iGameAttributeSet.h"      // 属性集：管理网格上的点属性/单元属性数组
#include "iGameCellArray.h"         // 单元数组：存储每个 cell 由哪些点组成（连接关系）
#include "iGameFlatArray.h"         // 一维数组基类（DoubleArray 等 FlatArray 模板的实例）
#include "iGameSurfaceMesh.h"       // 表面网格类型（三角形/四边形面）
#include "iGameUnstructuredMesh.h"  // 非结构网格类型（最通用，任意混合单元）
#include "iGameVolumeMesh.h"        // 体网格类型（四面体/六面体等）

IGAME_NAMESPACE_BEGIN
CountCellVerticesFilter::CountCellVerticesFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

bool CountCellVerticesFilter::Execute() {
    UpdateProgress(0);

    auto input = GetInput(0);
    if (input == nullptr) { return false; }
    CellArray::Pointer cells = input->GetCellArray();
    if (cells == nullptr) { return false; }

    int numCells = static_cast<int>(cells->GetNumberOfCells());  // 单元总数
    if (numCells == 0) {
        SetOutput(0, input);
        UpdateProgress(1);
        return true;
    }
    auto mesh = DynamicCast<PointSet>(input);
    if (mesh == nullptr) {
        SetOutput(0, input);
        UpdateProgress(1);
        return true;
    }

    DoubleArray::Pointer vertexCounts = DoubleArray::New();
    vertexCounts->SetDimension(1);
    vertexCounts->Reserve(numCells);
    vertexCounts->SetName("cell_vertex_count");

    auto attrs = mesh->GetAttributeSet();
    attrs->AddScalar(IG_CELL, vertexCounts);

    for (int i = 0; i < numCells; i++) {

        double count = static_cast<double>(cells->GetCellSize(i));
        vertexCounts->AddElement(&count);
    }

    UpdateProgress(0.5);
    // 直接对网格调用渲染数据刷新（mesh 是 DrawObject 的成员方法，无空指针风险）。
    // 【原因】AttributeSet::ForceReConvertToDrawableData() 内部依赖 m_DataObject，
    // 在 GUI 场景中该指针可能未被设置（为 nullptr）→ DynamicCast 后空指针调用崩溃。
    // 因此改为网格自身调用，效果等价且安全（GradientFilter 也采用同一写法）。
    mesh->ForceReConvertToDrawableData();
    UpdateProgress(1);
    SetOutput(0, input);
    return true;
}

IGAME_NAMESPACE_END
