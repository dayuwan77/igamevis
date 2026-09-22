#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <iGameFileIO.h>
#include <iGameDataObject.h>
#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <MergeVectorComponents/iGameMergeVectorComponentsFilter.h>

namespace {

void PrintScalarPreview(iGame::ArrayObject* arr, IGsize count) {
    if (!arr) { std::cout << "    (null)\n"; return; }
    const IGsize n = arr->GetNumberOfElements();
    const IGsize m = count < n ? count : n;
    std::cout << "    first " << m << " values: ";
    for (IGsize i = 0; i < m; ++i) {
        std::cout << arr->GetValue(i);
        if (i + 1 < m) std::cout << ", ";
    }
    std::cout << "\n";
}

void PrintVectorPreview(iGame::ArrayObject* arr, IGsize count) {
    if (!arr) { std::cout << "    (null)\n"; return; }
    const int dim = arr->GetDimension();
    const IGsize n = arr->GetNumberOfElements();
    const IGsize m = count < n ? count : n;
    std::cout << "    first " << m << " elements (dim=" << dim << "):\n";
    for (IGsize i = 0; i < m; ++i) {
        std::cout << "    [" << i << "] = (";
        for (int d = 0; d < dim; ++d) {
            std::cout << arr->GetElementValue(i, d);
            if (d + 1 < dim) std::cout << ", ";
        }
        std::cout << ")\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string fileName;
    if (argc >= 2) fileName = argv[1];
    if (fileName.empty()) {
        std::cout << "Please enter model file path: ";
        std::getline(std::cin, fileName);
    }
    if (fileName.empty()) {
        std::cerr << "[testMergeVectorComponents] no model path provided\n";
        return 1;
    }
    std::cerr << "[testMergeVectorComponents] cwd=" << std::filesystem::current_path().string()
              << " file=" << fileName << " exists=" << std::filesystem::exists(fileName) << "\n"
              << std::flush;

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cerr << "[testMergeVectorComponents] FAIL: ReadFile returned null\n" << std::flush;
        return 1;
    }
    auto attrs = obj->GetAttributeSet();
    if (!attrs) {
        std::cerr << "[testMergeVectorComponents] FAIL: no AttributeSet\n" << std::flush;
        return 1;
    }

    // 1) choose data type
    std::cout << "Choose data type (0=PointData 1=CellData): ";
    std::string line;
    int typeIdx = 0;
    if (std::getline(std::cin, line)) {
        try { typeIdx = std::stoi(line); } catch (...) { typeIdx = 0; }
    }
    const IGenum attach = (typeIdx == 1) ? IG_CELL : IG_POINT;
    const bool isPoint = (attach == IG_POINT);
    std::cout << "Selected: " << (isPoint ? "PointData" : "CellData") << "\n";

    auto buf = isPoint ? attrs->GetAllPointAttributes() : attrs->GetAllCellAttributes();
    if (!buf || buf->GetNumberOfElements() == 0) {
        std::cerr << "[testMergeVectorComponents] FAIL: no attributes under this data type\n" << std::flush;
        return 1;
    }

    // 2) list single-component scalars
    std::vector<std::string> names;
    std::cout << "\nAvailable single-component scalars:\n";
    for (int i = 0; i < buf->GetNumberOfElements(); ++i) {
        auto& a = buf->GetElement(i);
        if (a.type != IG_SCALAR || !a.pointer || a.pointer->GetDimension() != 1) continue;
        const std::string nm = a.pointer->GetName();
        names.push_back(nm);
        std::cout << "  [" << (names.size() - 1) << "] " << nm
                  << "  (n=" << a.pointer->GetNumberOfElements() << ")\n";
    }
    if (names.empty()) {
        std::cerr << "[testMergeVectorComponents] FAIL: no single-component scalar under this data type\n"
                  << std::flush;
        return 1;
    }

    // 3) input three attribute names (index or name accepted)
    std::vector<std::string> picked(3);
    const char* axisName[3] = {"X", "Y", "Z"};
    for (int k = 0; k < 3; ++k) {
        std::cout << "Enter " << axisName[k] << " component attribute name: ";
        if (!std::getline(std::cin, picked[k])) picked[k].clear();
        if (!picked[k].empty()) {
            bool allDigit = true;
            for (char c : picked[k]) if (c < '0' || c > '9') { allDigit = false; break; }
            if (allDigit) {
                int idx = std::stoi(picked[k]);
                if (idx >= 0 && idx < static_cast<int>(names.size())) picked[k] = names[idx];
            }
        }
        if (picked[k].empty()) {
            std::cerr << "[testMergeVectorComponents] FAIL: no attribute name for " << axisName[k] << "\n" << std::flush;
            return 1;
        }
    }
    std::cout << "\nSelected: X=" << picked[0] << "  Y=" << picked[1] << "  Z=" << picked[2] << "\n";

    // 4) print first 10 values of selected scalars
    std::cout << "\n=== Selected scalar attributes (first 10 values) ===\n";
    for (int k = 0; k < 3; ++k) {
        auto& a = attrs->GetAttribute(picked[k]);
        std::cout << "[" << axisName[k] << "] " << picked[k] << ":\n";
        PrintScalarPreview(a.pointer, 10);
    }

    // 5) merge
    auto filter = iGame::MergeVectorComponentsFilter::New();
    filter->SetInput(obj);
    filter->SetComponentArrayNames(picked);
    filter->SetAttachmentType(attach);
    filter->SetOutputVectorName("vector");
    if (!filter->Execute()) {
        std::cerr << "[testMergeVectorComponents] FAIL: Execute failed: "
                  << filter->GetMessage() << "\n" << std::flush;
        return 1;
    }

    // 6) print first 10 elements of merged vector
    auto& merged = attrs->GetAttribute("vector");
    if (merged.IsNone() || !merged.pointer) {
        std::cerr << "[testMergeVectorComponents] FAIL: merged result \"vector\" not found\n" << std::flush;
        return 1;
    }
    std::cout << "\n=== Merged vector \"vector\" (first 10 elements) ===\n";
    std::cout << "  type=" << (merged.type == IG_VECTOR ? "IG_VECTOR" : std::to_string(merged.type))
              << "  attachment=" << (merged.attachmentType == IG_POINT ? "IG_POINT"
                                   : merged.attachmentType == IG_CELL ? "IG_CELL"
                                   : std::to_string(merged.attachmentType))
              << "  dimension=" << merged.pointer->GetDimension()
              << "  nElements=" << merged.pointer->GetNumberOfElements() << "\n";
    PrintVectorPreview(merged.pointer, 10);

    std::cout << "\n[testMergeVectorComponents] done\n" << std::flush;
    return 0;
}
