// ============================================================================
// TestProbe — ProbeFilter 命令行探测测试
//
// 用法:
//   testProbe.exe <模型相对路径> <centerX> <centerY> <centerZ> [radius] [n] [seed]
//
// 参数说明:
//   <模型相对路径>  模型文件，例如 ./Models/SteadyFlowLaminarAndTurbulentInAnSBend1_final.ccm
//   <centerX/Y/Z>   探测球心坐标（必填）
//   [radius]        球体半径，默认 0：全部查询点生成在球心本身，方便测试
//   [n]             球体内随机采样点数，默认 1
//   [seed]          随机种子，默认 0（随机）；传 >0 固定种子便于复现
//
// 流程:
//   1. 读取模型，打印模型规模与点属性清单（帮助挑选探测中心坐标）。
//   2. radius==0 时全部 n 个查询点都生成在球心；radius>0 时在球体内均匀随机
//      采样 n 个点。
//   3. 执行 ProbeFilter：对查询点做单元定位 + 点属性线性插值，输出
//      ValidPointMask（命中单元=1，未命中=0），结果原地写回查询点集。
//   4. 逐点打印坐标、有效标记与各插值属性，终端输出 Result: PASS/FAIL。
// ============================================================================
#include <Probe/iGameProbeFilter.h>

#include <iGameAttributeSet.h>
#include <iGameDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGamePoints.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

using namespace iGame;

namespace {

void PrintUsage(const char* exeName) {
    std::cout << "Usage: " << exeName
              << " <model> <centerX> <centerY> <centerZ> [radius] [n] [seed]\n"
                 "  <model>          model file relative path, "
                 "e.g. ./Models/xxx.ccm\n"
                 "  <centerX/Y/Z>    probe sphere center\n"
                 "  [radius]         default 0 -> probe the center point only\n"
                 "  [n]              sample count in sphere, default 1\n"
                 "  [seed]           random seed, 0 = random (default)\n";
}

void PrintModelSummary(const DataObject::Pointer& model) {
    if (model.IsNull()) return;

    const IGsize numPoints =
            model->GetPoints().IsNull()
                    ? 0
                    : model->GetPoints()->GetNumberOfPoints();
    const IGsize numCells =
            model->GetCellArray().IsNull()
                    ? 0
                    : model->GetCellArray()->GetNumberOfCells();
    std::cout << "Model points=" << numPoints << " cells=" << numCells << "\n";

    const BoundingBox& bbox = model->GetBoundingBox();
    if (!bbox.isNull() && !bbox.isEmpty()) {
        const Vector3d c = bbox.center();
        std::cout << "Bounding box center=(" << c[0] << ", " << c[1] << ", "
                  << c[2] << ") diag=" << bbox.diag() << "\n";
    }

    AttributeSet* attrs = model->GetAttributeSet();
    if (attrs == nullptr) return;
    auto pointAttrs = attrs->GetAllPointAttributes();
    std::cout << "Point attributes: ";
    bool first = true;
    for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
        auto& attr = pointAttrs->GetElement(i);
        if (attr.isDeleted || attr.pointer.IsNull()) continue;
        if (!first) std::cout << ", ";
        first = false;
        std::cout << attr.pointer->GetName()
                  << "[dim=" << attr.pointer->GetDimension() << "]";
    }
    std::cout << "\n";
}

// radius == 0 时全部点落在球心本身；否则按球体体积均匀随机采样 n 个点。
bool MakeQueryPoints(PointSet::Pointer query, const Point& center, float radius,
                     int count, unsigned seed) {
    if (query.IsNull()) return false;
    auto points = query->GetPoints();
    if (points.IsNull()) return false;
    if (count <= 0) return false;

    if (radius <= 0.0f) {
        // GenerateSpherePoints 在 radius<=0 时直接返回、不生成任何点，
        // 因此这里手动把 n 个查询点全部放到球心。
        points->Reset();
        for (int i = 0; i < count; ++i) {
            points->AddPoint(center);
        }
        query->Modified();
        return points->GetNumberOfPoints() > 0;
    }

    ProbeFilter::GenerateSpherePoints(query, center, radius, count, seed);
    return points->GetNumberOfPoints() > 0;
}

// 打印一个数组元素的所有分量（以及分量数 > 1 时的模）。
void PrintArrayElement(ArrayObject* array, IGsize queryId) {
    if (array == nullptr) return;
    const int dim = array->GetDimension();
    std::cout << "(";
    for (int c = 0; c < dim; ++c) {
        if (c > 0) std::cout << ", ";
        std::cout << array->GetElementValue(queryId, c);
    }
    std::cout << ")";
    if (dim > 1) {
        std::cout << " mag=" << array->GetElementValue(queryId, -1);
    }
}

bool RunProbe(const std::string& modelFile, const Point& center, float radius,
              int count, unsigned seed) {
    std::cout << "Model: " << modelFile << "\n";
    auto model = FileIO::ReadFile(modelFile);
    if (model.IsNull()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Read model failed\n";
        return false;
    }
    PrintModelSummary(model);

    auto query = PointSet::New();
    query->SetName("ProbeQueryPoints");
    if (!MakeQueryPoints(query, center, radius, count, seed)) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "Build query points failed (radius=" << radius
                  << ", n=" << count << ")\n";
        return false;
    }

    auto filter = ProbeFilter::New();
    filter->SetInput(0, model);
    filter->SetInput(1, query);
    if (!filter->Execute()) {
        std::cerr << "Result: FAIL\n";
        std::cerr << "ProbeFilter::Execute() returned false\n";
        return false;
    }

    const IGsize numQuery = query->GetPoints()->GetNumberOfPoints();
    AttributeSet* attrs = query->GetAttributeSet();
    int validCount = 0;
    for (IGsize qi = 0; qi < numQuery; ++qi) {
        const Point& p = query->GetPoint(qi);

        int mask = 0;
        if (attrs != nullptr) {
            const int maskIndex = attrs->GetAttributeIndex("ValidPointMask");
            if (maskIndex >= 0) {
                auto maskArray =
                        DynamicCast<IntArray>(attrs->GetAttribute(maskIndex).pointer);
                if (!maskArray.IsNull()) mask = maskArray->GetValue(qi);
            }
        }
        if (mask == 1) ++validCount;

        std::cout << "Point[" << qi << "] = (" << p[0] << ", " << p[1] << ", "
                  << p[2] << ") valid=" << mask;
        if (attrs != nullptr) {
            auto pointAttrs = attrs->GetAllPointAttributes();
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.isDeleted || attr.pointer.IsNull()) continue;
                if (attr.pointer->GetName() == "ValidPointMask") continue;
                std::cout << "  " << attr.pointer->GetName() << "=";
                PrintArrayElement(attr.pointer.GetPointer(), qi);
            }
        }
        std::cout << "\n";
    }

    std::cout << "Valid points: " << validCount << " / " << numQuery << "\n";
    std::cout << "Result: PASS\n";
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "-h" ||
        std::string(argv[1]) == "--help") {
        PrintUsage(argv[0]);
        return argc < 2 ? 2 : 0;
    }
    if (argc < 5) {
        std::cerr << "Missing model path or center coordinates\n";
        PrintUsage(argv[0]);
        return 2;
    }

    const std::string modelFile = argv[1];
    const Point center(static_cast<float>(std::atof(argv[2])),
                       static_cast<float>(std::atof(argv[3])),
                       static_cast<float>(std::atof(argv[4])));
    const float radius =
            argc > 5 ? static_cast<float>(std::atof(argv[5])) : 0.0f;
    const int count = argc > 6 ? std::atoi(argv[6]) : 1;
    const unsigned seed =
            argc > 7 ? static_cast<unsigned>(std::atoi(argv[7])) : 0u;

    std::cout << "Center=(" << center[0] << ", " << center[1] << ", "
              << center[2] << ") radius=" << radius << " n=" << count
              << " seed=" << seed << "\n";
    return RunProbe(modelFile, center, radius, count, seed) ? 0 : 1;
}
