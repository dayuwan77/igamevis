// ─────────────────────────────────────────────────────────────────────────────
// Windows 平台前置处理：必须在【所有 iGameVis 头文件之前】完成。
// windows.h 会定义宏 `#define GetMessage GetMessageA`，若先被某个 iGameVis 头文件
// 间接引入，随后解析 iGameGroupDatasetsFilter.h 时类内的 GetMessage() 声明会被
// 宏改名，导致调用 filter->GetMessage() 报 C2039。这里先引入并立即 #undef。
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef GetMessage
#endif

#include <GroupDatasets/iGameGroupDatasetsFilter.h>
#include <iGameDrawObject.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_Passed = 0;
int g_Failed = 0;

void Check(bool condition, const std::string& name, const std::string& detail = "") {
    if (condition) {
        ++g_Passed;
        std::cout << "    [PASS] " << name << "\n";
    } else {
        ++g_Failed;
        std::cout << "    [FAIL] " << name;
        if (!detail.empty()) { std::cout << "  -> " << detail; }
        std::cout << "\n";
    }
}

std::string FindModelPath(const std::string& modelName) {
    const std::vector<std::string> searchPaths = {
            "./Models/" + modelName,
            "./Examples/Models/" + modelName,
            "../Examples/Models/" + modelName,
            "../../Examples/Models/" + modelName,
    };
    for (const auto& path: searchPaths) {
        if (std::filesystem::exists(path)) { return path; }
    }
    return "./Models/" + modelName;
}

/// 取点数（UnstructuredMesh / SurfaceMesh 都继承自 PointSet）
int PointCount(const iGame::DataObject::Pointer& obj) {
    auto ps = iGame::DynamicCast<iGame::PointSet>(obj);
    return ps ? static_cast<int>(ps->GetNumberOfPoints()) : -1;
}

/// 取单元/面片数
int CellCount(const iGame::DataObject::Pointer& obj) {
    if (auto m = iGame::DynamicCast<iGame::UnstructuredMesh>(obj)) {
        return static_cast<int>(m->GetNumberOfCells());
    }
    if (auto m = iGame::DynamicCast<iGame::SurfaceMesh>(obj)) {
        return static_cast<int>(m->GetNumberOfFaces());
    }
    return -1;
}

/// 取属性个数（用于验证属性是否随块保留）
int AttributeCount(const iGame::DataObject::Pointer& obj) {
    auto set = obj ? obj->GetAttributeSet() : nullptr;
    return set ? static_cast<int>(set->GetNumberOfAttributes()) : -1;
}

std::string TypeName(const iGame::DataObject::Pointer& obj) {
    if (!obj) { return "null"; }
    switch (obj->GetDataObjectType()) {
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_SURFACE_MESH:      return "SurfaceMesh";
        case IG_VOLUME_MESH:       return "VolumeMesh";
        case IG_STRUCTURED_MESH:   return "StructuredMesh";
        default:                   return "Other";
    }
}

/// 把多块组的子块收集成数组。
///
/// ⚠️ 重要：多块子对象由 `SubDataObjectsHelper` 以
/// `std::map<DataObjectId, DataObject::Pointer>` 保存，
/// 因此【迭代顺序 = DataObjectId 顺序（≈对象创建顺序）】，
/// **不是** AddSubDataObject 的调用顺序。
/// 所以下面所有断言一律用「按指针查找」，绝不用下标，
/// 否则会依赖一个未被保证的顺序。
std::vector<iGame::DataObject::Pointer> CollectBlocks(const iGame::DataObject::Pointer& group) {
    std::vector<iGame::DataObject::Pointer> blocks;
    if (!group) { return blocks; }
    for (auto it = group->SubDataObjectIteratorBegin(); it != group->SubDataObjectIteratorEnd(); ++it) {
        blocks.push_back(it->second);
    }
    return blocks;
}

/// 按对象指针在子块中查找对应块；找不到返回 nullptr。
iGame::DataObject::Pointer FindBlock(const std::vector<iGame::DataObject::Pointer>& blocks,
                                     const iGame::DataObject::Pointer& target) {
    for (const auto& b: blocks) {
        if (b.GetPointer() == target.GetPointer()) { return b; }
    }
    return nullptr;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); // 控制台切 UTF-8，保证中文提示不乱码
#endif
    std::cout << std::unitbuf;

    std::cout << "\n==============================================================================\n";
    std::cout << "  【iGameVis】GroupDatasetsFilter 自检（对齐 ParaView Group Datasets）\n";
    std::cout << "  语义：只做容器组合，不合并几何；允许混合类型；多块输入嵌套不展平。\n";
    std::cout << "==============================================================================\n";

    // ---------- 准备输入 ----------
    auto tet = iGame::FileIO::ReadFile(FindModelPath("group_tet.vtk"));
    auto hex = iGame::FileIO::ReadFile(FindModelPath("group_hex.vtk"));
    auto quad = iGame::FileIO::ReadFile(FindModelPath("group_quad.vtk"));

    std::cout << "\n[准备] 载入测试模型\n";
    std::cout << "    group_tet.vtk  : " << TypeName(tet) << ", Points = " << PointCount(tet)
              << ", Cells = " << CellCount(tet) << "\n";
    std::cout << "    group_hex.vtk  : " << TypeName(hex) << ", Points = " << PointCount(hex)
              << ", Cells = " << CellCount(hex) << "\n";
    std::cout << "    group_quad.vtk : " << TypeName(quad) << ", Points = " << PointCount(quad)
              << ", Cells = " << CellCount(quad) << "\n";

    // =========================================================================
    std::cout << "\n[Test 1] 基本组合：3 个不同类型（体网格 + 体网格 + 面片）\n";
    // =========================================================================
    {
        std::vector<iGame::DataObject::Pointer> inputs = {tet, hex, quad};
        const int sumPoints = PointCount(tet) + PointCount(hex) + PointCount(quad);
        const int sumCells = CellCount(tet) + CellCount(hex) + CellCount(quad);

        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs(inputs);
        filter->SetOutputName("GroupDatasets_1");

        bool ok = filter->Execute();
        Check(ok, "Execute() 返回 true", filter->GetMessage());
        Check(filter->GetGroupedCount() == 3, "合并对象数 == 3",
              "actual=" + std::to_string(filter->GetGroupedCount()));

        auto group = filter->GetOutput(0);
        Check(group != nullptr, "输出对象非空");
        Check(group && group->HasSubDataObject(), "输出是多块组");

        auto blocks = CollectBlocks(group);
        Check(static_cast<int>(blocks.size()) == 3, "顶层块数 == 输入数",
              "actual=" + std::to_string(blocks.size()));

        // 逐个输入验证：在输出中能找到对应块，且该块与原输入完全一致。
        // 用「按指针查找」而非下标 —— 子块迭代顺序由 DataObjectId 决定。
        for (size_t i = 0; i < inputs.size(); ++i) {
            auto block = FindBlock(blocks, inputs[i]);
            const std::string tag = "输入[" + std::to_string(i) + "] " + inputs[i]->GetName();

            Check(block != nullptr, tag + " 在输出中能找到对应块");
            if (!block) { continue; }

            Check(block->GetName() == inputs[i]->GetName(), tag + " 块名 == 输入名");
            Check(block->GetDataObjectType() == inputs[i]->GetDataObjectType(),
                  tag + " 类型保留", TypeName(block));
            Check(PointCount(block) == PointCount(inputs[i]), tag + " 点数未被改动");
            Check(CellCount(block) == CellCount(inputs[i]), tag + " 单元数未被改动");
            Check(AttributeCount(block) == AttributeCount(inputs[i]), tag + " 属性保留");
        }

        // 关键：共享指针，没有复制几何 —— 块与输入是同一个对象
        Check(FindBlock(blocks, tet) != nullptr, "tet 与输出块是同一对象（共享，未复制）");
        Check(FindBlock(blocks, hex) != nullptr, "hex 与输出块是同一对象（共享，未复制）");
        Check(FindBlock(blocks, quad) != nullptr, "quad 与输出块是同一对象（共享，未复制）");

        // 关键：点数/单元数是"各自独立"，不是"合并后"
        int blockPoints = 0, blockCells = 0;
        for (const auto& b: blocks) { blockPoints += PointCount(b); blockCells += CellCount(b); }
        Check(blockPoints == sumPoints, "各块点数之和 == 各输入点数之和（未合并几何）",
              std::to_string(blockPoints) + " vs " + std::to_string(sumPoints));
        Check(blockCells == sumCells, "各块单元数之和 == 各输入单元数之和（未合并几何）",
              std::to_string(blockCells) + " vs " + std::to_string(sumCells));

        // 混合类型确实被保留（体网格仍是体网格，面片仍是面片）
        auto quadBlock = FindBlock(blocks, quad);
        Check(quadBlock && quadBlock->GetDataObjectType() == IG_SURFACE_MESH,
              "面片输入在输出中仍是 SurfaceMesh");
    }

    // =========================================================================
    std::cout << "\n[Test 2] 多块输入：嵌套不展平\n";
    // =========================================================================
    {
        // 先用 tet + hex 做出一个 2 块的多块组
        auto innerFilter = iGame::GroupDatasetsFilter::New();
        innerFilter->SetInputs({tet, hex});
        innerFilter->SetOutputName("InnerGroup");
        Check(innerFilter->Execute(), "内层多块组构建成功", innerFilter->GetMessage());
        auto inner = innerFilter->GetOutput(0);

        // 再用 [内层多块组, quad] 做外层组合
        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({inner, quad});
        filter->SetOutputName("GroupDatasets_nested");
        bool ok = filter->Execute();
        Check(ok, "外层组合执行成功", filter->GetMessage());

        auto outer = filter->GetOutput(0);
        auto blocks = CollectBlocks(outer);
        Check(static_cast<int>(blocks.size()) == 2,
              "顶层块数 == 输入数（多块输入算 1 个，未展平）",
              "actual=" + std::to_string(blocks.size()));

        // 按指针找，不按下标 —— 迭代顺序是 DataObjectId 序，不是加入顺序
        auto innerBlock = FindBlock(blocks, inner);
        auto quadBlock = FindBlock(blocks, quad);

        Check(innerBlock != nullptr, "多块输入在输出中能找到对应块");
        Check(innerBlock && innerBlock->HasSubDataObject(),
              "该块仍是多块组（结构被保留，未展平）");
        Check(innerBlock && innerBlock->GetNumberOfSubDataObjects() == 2,
              "该块内部仍有 2 个子块",
              innerBlock ? "actual=" + std::to_string(innerBlock->GetNumberOfSubDataObjects()) : "");

        Check(quadBlock != nullptr, "面片输入在输出中能找到对应块");
        Check(quadBlock && quadBlock->GetDataObjectType() == IG_SURFACE_MESH,
              "该块是面片", TypeName(quadBlock));

        // 顶层块数 == 2 而不是 3，说明多块输入没有被展开
        Check(static_cast<int>(blocks.size()) == 2, "多块输入未被展平（否则会是 3 个顶层块）");
    }

    // =========================================================================
    std::cout << "\n[Test 3] 单输入：允许，等价于包一层\n";
    // =========================================================================
    {
        auto filter = iGame::GroupDatasetsFilter::New();
        filter->SetInputs({tet});
        filter->SetOutputName("GroupDatasets_single");
        bool ok = filter->Execute();
        Check(ok, "单输入 Execute() 返回 true", filter->GetMessage());
        Check(filter->GetGroupedCount() == 1, "合并对象数 == 1");

        auto blocks = CollectBlocks(filter->GetOutput(0));
        Check(static_cast<int>(blocks.size()) == 1, "顶层块数 == 1",
              "actual=" + std::to_string(blocks.size()));
        Check(!blocks.empty() && blocks[0].GetPointer() == tet.GetPointer(),
              "块与输入是同一对象");
    }

    // =========================================================================
    std::cout << "\n[Test 4] 输入校验\n";
    // =========================================================================
    {
        // 4.1 空输入
        auto empty = iGame::GroupDatasetsFilter::New();
        empty->SetInputs({});
        bool ok = empty->Execute();
        Check(!ok, "空输入时 Execute() 返回 false");
        Check(!empty->GetMessage().empty(), "空输入时给出提示信息");
        std::cout << "      * 提示内容: " << empty->GetMessage() << "\n";

        // 4.2 含 nullptr：应被过滤掉，只统计有效对象
        auto mixed = iGame::GroupDatasetsFilter::New();
        mixed->SetInputs({nullptr, tet, nullptr, quad});
        mixed->SetOutputName("GroupDatasets_test");
        bool ok2 = mixed->Execute();
        Check(ok2, "含 nullptr 的输入仍能执行", mixed->GetMessage());
        Check(mixed->GetGroupedCount() == 2, "nullptr 被过滤，只合并 2 个有效对象",
              "actual=" + std::to_string(mixed->GetGroupedCount()));

        auto blocks = CollectBlocks(mixed->GetOutput(0));
        Check(static_cast<int>(blocks.size()) == 2, "顶层块数 == 2（不含 nullptr）",
              "actual=" + std::to_string(blocks.size()));

        // 4.3 输出名字被正确设置
        Check(mixed->GetOutput(0) && mixed->GetOutput(0)->GetName() == "GroupDatasets_test",
              "输出名 == SetOutputName 指定的名字",
              mixed->GetOutput(0) ? mixed->GetOutput(0)->GetName() : "");

        // 4.4 同一个对象重复加入：底层 SubDataObjectsHelper 自带去重，只保留 1 份
        auto dup = iGame::GroupDatasetsFilter::New();
        dup->SetInputs({tet, tet});
        dup->SetOutputName("GroupDatasets_dup");
        dup->Execute();
        auto dupBlocks = CollectBlocks(dup->GetOutput(0));
        Check(static_cast<int>(dupBlocks.size()) == 1,
              "同一对象重复加入只保留 1 块（底层 map 自带去重）",
              "actual=" + std::to_string(dupBlocks.size()));
    }

    // =========================================================================
    std::cout << "\n============================================================\n";
    std::cout << "  Result: " << g_Passed << " passed, " << g_Failed << " failed\n";
    std::cout << "============================================================\n";
    if (g_Failed == 0) {
        std::cout << "[SUCCESS] all assertions passed.\n";
        return 0;
    }
    std::cout << "[FAILURE] some assertions failed.\n";
    return 1;
}
