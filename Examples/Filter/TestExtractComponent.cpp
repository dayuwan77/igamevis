#include <iostream>
#include <algorithm>
#include <cfloat>
#include <vector>
#include <iGameFileIO.h>
#include <iGameUnstructuredMesh.h>
#include <Attribute/iGameExtractComponentFilter.h>

// 「提取分量」统一验证文件：test/Streamline Test/StreamTest.vtk（8499 点 VECTORS V float）
// 用法：testExtractComponent.exe "<仓库根>/test/Streamline Test/StreamTest.vtk"

// 程序化构造：4 点四面体 + 指定维度的点向量属性（值 = i*dim + j）
iGame::UnstructuredMesh::Pointer CreateMeshWithDimVector(int dim) {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex cell[4] = {0, 1, 2, 3};
    mesh->AddCell(cell, 4, iGame::IG_TETRA);

    iGame::FloatArray::Pointer vec = iGame::FloatArray::New();
    vec->SetName("vec");
    vec->SetDimension(dim);
    std::vector<double> value(dim);
    for (IGsize i = 0; i < 4; ++i) {
        for (int j = 0; j < dim; ++j) value[j] = static_cast<double>(i * dim + j);
        vec->AddElement(value.data());
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, vec);
    return mesh;
}

// 程序化构造：4 点四面体 + 点向量属性 test_1（X/Y/Z 分别取 1/2/3 起，逐点 +3）
iGame::UnstructuredMesh::Pointer CreateMeshWithPointVector() {
    return CreateMeshWithDimVector(3);
}

// 程序化构造：5 点 2 四面体 + 单元向量属性 cellVec（两个单元 (10,20,30)/(40,50,60)）
iGame::UnstructuredMesh::Pointer CreateMeshWithCellVector() {
    auto mesh = iGame::UnstructuredMesh::New();
    mesh->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    mesh->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    mesh->AddPoint(iGame::Point(1.f, 1.f, 0.f));
    igIndex cell0[4] = {0, 1, 2, 3};
    igIndex cell1[4] = {1, 4, 2, 3};
    mesh->AddCell(cell0, 4, iGame::IG_TETRA);
    mesh->AddCell(cell1, 4, iGame::IG_TETRA);

    iGame::FloatArray::Pointer vec = iGame::FloatArray::New();
    vec->SetName("cellVec");
    vec->SetDimension(3);
    vec->AddElement3(10.f, 20.f, 30.f);
    vec->AddElement3(40.f, 50.f, 60.f);
    mesh->GetAttributeSet()->AddVector(IG_CELL, vec);
    return mesh;
}

// 仿 TestGenerateProcessIds：带 argv 时读取真实模型，否则用程序化网格
iGame::UnstructuredMesh::Pointer CreateMesh(int argc, char* argv[]) {
    if (argc > 1) {
        auto obj = iGame::FileIO::ReadFile(argv[1]);
        auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
        if (mesh == nullptr) {
            std::cout << "FAIL: read model " << argv[1] << "\n";
            return nullptr;
        }
        return mesh;
    }
    return CreateMeshWithPointVector();
}

// 3 维向量 test_1：第 i 个元素的第 comp 个分量，期望值 = 0 + 3*i + comp（comp 0/1/2）
double ExpectPointValue(IGsize i, int comp) { return 0.0 + 3.0 * i + comp; }

namespace {

// 提取单分量并校验：输出为继承新对象（几何共享、输入不被修改）、值逐元素正确、dataRange 正确
bool VerifyExtract(int component, const std::string& outputName, IGenum expectAttachment,
                   const std::string& inputName = "") {
    auto mesh = CreateMeshWithPointVector();
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetInputArrayName(inputName);
    filter->SetOutputArrayName(outputName);
    filter->SetComponent(component);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    // 继承语义：输入属性集不含输出数组（输入不被修改）；几何共享
    bool ok = mesh->GetAttributeSet()->GetAttribute(outputName).IsNone();
    ok = ok && (outMesh->GetPoints() == mesh->GetPoints());
    // 输出对象属性校验
    auto& attr = outMesh->GetAttributeSet()->GetScalar(outputName);
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr) && (attr.attachmentType == expectAttachment);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, component));
    }
    // dataRange 断言：magnitude 范围与第一维范围均为所选分量的 [min, max]
    // （分量值全为正，magnitude 与符号值相等；dataRange 布局见 iGameAttributeSet.h 注释）
    auto range = attr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(0) == ExpectPointValue(0, component))
         && (range->GetValue(1) == ExpectPointValue(3, component))
         && (range->GetValue(2) == ExpectPointValue(0, component))
         && (range->GetValue(3) == ExpectPointValue(3, component));
    return ok;
}

// 单元挂载用例：空输入名取单元向量，校验值、挂载类型与 dataRange
bool VerifyExtractCell() {
    auto mesh = CreateMeshWithCellVector();
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetOutputArrayName("Result");
    filter->SetComponent(0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    bool ok = mesh->GetAttributeSet()->GetAttribute("Result").IsNone();
    auto& attr = outMesh->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr) && (attr.attachmentType == IG_CELL)
         && (arr->GetNumberOfElements() == 2);
    if (ok) {
        ok = (arr->GetValue(0) == 10.0) && (arr->GetValue(1) == 40.0);
        auto range = attr.GetDataRange();
        ok = ok && (range != nullptr) && (range->GetValue(2) == 10.0) && (range->GetValue(3) == 40.0);
    }
    return ok;
}

// 空输入名取第一个向量（test_1），显式名取第二个（test_2，值翻倍）
bool VerifyInputArraySelection(bool useExplicitName) {
    auto mesh = CreateMeshWithPointVector();
    auto extra = iGame::FloatArray::New();
    extra->SetName("test_2");
    extra->SetDimension(3);
    for (IGsize i = 0; i < 4; ++i) {
        extra->AddElement3(2.f + 6.f * i, 4.f + 6.f * i, 6.f + 6.f * i);
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, extra);

    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    if (useExplicitName) filter->SetInputArrayName("test_2");
    filter->SetOutputArrayName("Result");
    filter->SetComponent(useExplicitName ? 1 : 0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    bool ok = mesh->GetAttributeSet()->GetAttribute("Result").IsNone();
    auto& attr = outMesh->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    ok = ok && (arr != nullptr);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == (useExplicitName ? 4.0 + 6.0 * i : ExpectPointValue(i, 0)));
    }
    return ok;
}

// 重名覆盖：输出名与输入已有数组重名时不报错，结果属性集中该名字唯一且为新提取值
bool VerifyOverwriteDuplicateName() {
    auto mesh = CreateMeshWithPointVector();
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    filter->SetOutputArrayName("test_1");  // 与输入已有向量属性重名 → 覆盖（不报错）
    filter->SetComponent(0);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    // 输入不被修改（输入属性集仍只有 VECTOR test_1，无标量 test_1）
    if (!mesh->GetAttributeSet()->GetScalar("test_1").IsNone()) {
        std::cout << "FAIL: input should not be modified\n";
        return false;
    }
    // 结果属性集中名为 test_1 的活属性唯一（旧向量被覆盖删除，只有新标量）
    int count = 0;
    auto all = outMesh->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (!attr.IsNone() && attr.pointer->GetName() == "test_1") ++count;
    }
    if (count != 1) {
        std::cout << "FAIL: overwritten name should be unique (count=" << count << ")\n";
        return false;
    }
    // 值 = 新提取的分量（分量 0 → 1,4,7,10）
    auto& attr = outMesh->GetAttributeSet()->GetScalar("test_1");
    auto arr = attr.pointer;
    bool ok = (arr != nullptr) && (attr.type == IG_SCALAR);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, 0));
    }
    return ok;
}

// 需求 4：1/2 维数组不能提取不存在的分量（2 维 X/Y 合法、Z 非法；1 维 X 合法、Y 非法）
bool VerifyDimensionGuard() {
    bool allOk = true;
    // 2 维数组：元素 i 值为 {2i, 2i+1}
    {
        auto mesh = CreateMeshWithDimVector(2);
        auto f0 = iGame::ExtractComponentFilter::New();
        f0->SetInput(mesh);
        f0->SetOutputArrayName("R0");
        f0->SetComponent(0);
        bool ok0 = f0->Execute();
        if (ok0) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f0->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R0");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok0 && i < arr->GetNumberOfElements(); ++i) {
                ok0 = (arr->GetValue(i) == static_cast<double>(2 * i));
            }
        }
        auto f1 = iGame::ExtractComponentFilter::New();
        f1->SetInput(mesh);
        f1->SetOutputArrayName("R1");
        f1->SetComponent(1);
        bool ok1 = f1->Execute();
        if (ok1) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f1->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R1");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok1 && i < arr->GetNumberOfElements(); ++i) {
                ok1 = (arr->GetValue(i) == static_cast<double>(2 * i + 1));
            }
        }
        auto f2 = iGame::ExtractComponentFilter::New();
        f2->SetInput(mesh);
        f2->SetOutputArrayName("R2");
        f2->SetComponent(2);
        bool ok2 = !f2->Execute() && !f2->GetMessage().empty();
        allOk = allOk && ok0 && ok1 && ok2;
        if (!ok0) std::cout << "FAIL: 2D component 0\n";
        if (!ok1) std::cout << "FAIL: 2D component 1\n";
        if (!ok2) std::cout << "FAIL: 2D component 2 should reject\n";
    }
    // 1 维数组：元素 i 值为 {i}
    {
        auto mesh = CreateMeshWithDimVector(1);
        auto f0 = iGame::ExtractComponentFilter::New();
        f0->SetInput(mesh);
        f0->SetOutputArrayName("R0");
        f0->SetComponent(0);
        bool ok0 = f0->Execute();
        if (ok0) {
            auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(f0->GetOutput());
            auto& attr = outMesh->GetAttributeSet()->GetScalar("R0");
            auto arr = attr.pointer;
            for (IGsize i = 0; ok0 && i < arr->GetNumberOfElements(); ++i) {
                ok0 = (arr->GetValue(i) == static_cast<double>(i));
            }
        }
        auto f1 = iGame::ExtractComponentFilter::New();
        f1->SetInput(mesh);
        f1->SetOutputArrayName("R1");
        f1->SetComponent(1);
        bool ok1 = !f1->Execute() && !f1->GetMessage().empty();
        allOk = allOk && ok0 && ok1;
        if (!ok0) std::cout << "FAIL: 1D component 0\n";
        if (!ok1) std::cout << "FAIL: 1D component 1 should reject\n";
    }
    return allOk;
}

// 回归：对提取分量结果再次提取（触发含 null dataRange 属性的 DeepCopy）不应崩溃
bool VerifyExtractOnExtractedResult() {
    auto mesh = CreateMeshWithPointVector();
    auto f1 = iGame::ExtractComponentFilter::New();
    f1->SetInput(mesh);
    f1->SetOutputArrayName("Result");
    f1->SetComponent(0);
    if (!f1->Execute()) {
        std::cout << "FAIL: first Execute (" << f1->GetMessage() << ")\n";
        return false;
    }
    auto out1 = iGame::DynamicCast<iGame::UnstructuredMesh>(f1->GetOutput());
    if (out1 == nullptr) {
        std::cout << "FAIL: first output is not UnstructuredMesh\n";
        return false;
    }

    // 对结果再次提取：输出名同为 Result（触发覆盖），输入属性集含 dataRange=null 的数组
    auto f2 = iGame::ExtractComponentFilter::New();
    f2->SetInput(out1);
    f2->SetOutputArrayName("Result");
    f2->SetComponent(1);
    if (!f2->Execute()) {
        std::cout << "FAIL: second Execute (" << f2->GetMessage() << ")\n";
        return false;
    }
    auto out2 = iGame::DynamicCast<iGame::UnstructuredMesh>(f2->GetOutput());
    if (out2 == nullptr) {
        std::cout << "FAIL: second output is not UnstructuredMesh\n";
        return false;
    }

    // 结果属性集中名为 Result 的活属性唯一（旧 Result 被覆盖删除）
    int count = 0;
    auto all = out2->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& attr = all->GetElement(i);
        if (!attr.IsNone() && attr.pointer->GetName() == "Result") ++count;
    }
    if (count != 1) {
        std::cout << "FAIL: Result should be unique (count=" << count << ")\n";
        return false;
    }

    // 值 = 第二次提取的分量（输入 V 的第 1 分量 → 1,4,7,10）
    auto& attr = out2->GetAttributeSet()->GetScalar("Result");
    auto arr = attr.pointer;
    bool ok = (arr != nullptr);
    for (IGsize i = 0; ok && i < arr->GetNumberOfElements(); ++i) {
        ok = (arr->GetValue(i) == ExpectPointValue(i, 1));
    }
    // 触发 dataRange 懒计算（修复前此处为空数组越界 → 回归失败）
    auto range = attr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(2) == ExpectPointValue(0, 1))
         && (range->GetValue(3) == ExpectPointValue(3, 1));
    return ok;
}

// 真实数据验证：值与输入分量一致、挂载跟随、dataRange 等于手动扫描的分量范围、输入不被修改
bool VerifyRealData(iGame::UnstructuredMesh::Pointer mesh, const std::string& inputName,
                    int component, const std::string& outputName) {
    auto filter = iGame::ExtractComponentFilter::New();
    filter->SetInput(mesh);
    if (!inputName.empty()) filter->SetInputArrayName(inputName);
    filter->SetOutputArrayName(outputName);
    filter->SetComponent(component);
    if (!filter->Execute()) {
        std::cout << "FAIL: Execute (" << filter->GetMessage() << ")\n";
        return false;
    }
    auto outMesh = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    if (outMesh == nullptr) {
        std::cout << "FAIL: output is not UnstructuredMesh\n";
        return false;
    }
    if (!mesh->GetAttributeSet()->GetAttribute(outputName).IsNone()) {
        std::cout << "FAIL: input should not be modified\n";
        return false;
    }

    auto attrSet = mesh->GetAttributeSet();
    iGame::AttributeSet::Attribute inputAttr;
    if (inputName.empty()) {
        auto all = attrSet->GetAllAttributes();
        for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
            auto& candidate = all->GetElement(i);
            if (!candidate.IsNone() && candidate.type == IG_VECTOR) {
                inputAttr = candidate;
                break;
            }
        }
    } else {
        inputAttr = attrSet->GetAttribute(inputName, IG_VECTOR);
    }
    if (inputAttr.IsNone()) {
        std::cout << "FAIL: input attribute not found\n";
        return false;
    }

    auto inArr = inputAttr.pointer;
    auto& outAttr = outMesh->GetAttributeSet()->GetScalar(outputName);
    auto outArr = outAttr.pointer;
    bool ok = (outArr != nullptr) && (outAttr.attachmentType == inputAttr.attachmentType)
              && (outArr->GetNumberOfElements() == inArr->GetNumberOfElements());

    double minV = DBL_MAX, maxV = DBL_MIN;
    for (IGsize i = 0; ok && i < inArr->GetNumberOfElements(); ++i) {
        double v = inArr->GetElementValue(i, component);
        minV = std::min(minV, v);
        maxV = std::max(maxV, v);
        ok = (outArr->GetValue(i) == v);
    }

    auto range = outAttr.GetDataRange();
    ok = ok && (range != nullptr) && (range->GetValue(2) == minV) && (range->GetValue(3) == maxV);
    return ok;
}

// The output array keeps the same concrete type as the input (Int -> Int, LongLong -> LongLong, ...).
bool VerifyOutputTypePreserved() {
    bool allOk = true;

    // IntArray input
    auto meshInt = iGame::UnstructuredMesh::New();
    meshInt->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    meshInt->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    meshInt->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    meshInt->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex cell[4] = {0, 1, 2, 3};
    meshInt->AddCell(cell, 4, iGame::IG_TETRA);
    auto vecInt = iGame::IntArray::New();
    vecInt->SetName("vecInt");
    vecInt->SetDimension(3);
    for (int i = 0; i < 4; ++i) {
        int v[3] = {i, i + 1, i + 2};
        vecInt->AddElement(v);
    }
    meshInt->GetAttributeSet()->AddVector(IG_POINT, vecInt);

    auto fInt = iGame::ExtractComponentFilter::New();
    fInt->SetInput(meshInt);
    fInt->SetOutputArrayName("RInt");
    fInt->SetComponent(0);
    if (!fInt->Execute()) {
        std::cout << "FAIL: Int Execute (" << fInt->GetMessage() << ")\n";
        return false;
    }
    auto outInt = iGame::DynamicCast<iGame::UnstructuredMesh>(fInt->GetOutput());
    if (outInt == nullptr) return false;
    auto& attrInt = outInt->GetAttributeSet()->GetScalar("RInt");
    bool intOk = (attrInt.pointer != nullptr) && (attrInt.pointer->GetArrayType() == IG_IntArray);
    for (IGsize i = 0; intOk && i < attrInt.pointer->GetNumberOfElements(); ++i) {
        intOk = (attrInt.pointer->GetValue(i) == static_cast<double>(i));
    }
    if (!intOk) std::cout << "FAIL: IntArray output type\n";
    allOk = allOk && intOk;

    // LongLongArray input
    auto meshLL = iGame::UnstructuredMesh::New();
    meshLL->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    meshLL->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    meshLL->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    meshLL->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    meshLL->AddCell(cell, 4, iGame::IG_TETRA);
    auto vecLL = iGame::LongLongArray::New();
    vecLL->SetName("vecLL");
    vecLL->SetDimension(3);
    for (int i = 0; i < 4; ++i) {
        int64_t v[3] = {10LL + i, 20LL + i, 30LL + i};
        vecLL->AddElement(v);
    }
    meshLL->GetAttributeSet()->AddVector(IG_POINT, vecLL);

    auto fLL = iGame::ExtractComponentFilter::New();
    fLL->SetInput(meshLL);
    fLL->SetOutputArrayName("RLL");
    fLL->SetComponent(0);
    if (!fLL->Execute()) {
        std::cout << "FAIL: LongLong Execute (" << fLL->GetMessage() << ")\n";
        return false;
    }
    auto outLL = iGame::DynamicCast<iGame::UnstructuredMesh>(fLL->GetOutput());
    if (outLL == nullptr) return false;
    auto& attrLL = outLL->GetAttributeSet()->GetScalar("RLL");
    bool llOk = (attrLL.pointer != nullptr) && (attrLL.pointer->GetArrayType() == IG_LongLongArray);
    for (IGsize i = 0; llOk && i < attrLL.pointer->GetNumberOfElements(); ++i) {
        llOk = (attrLL.pointer->GetValue(i) == 10.0 + i);
    }
    if (!llOk) std::cout << "FAIL: LongLongArray output type\n";
    allOk = allOk && llOk;

    return allOk;
}

// Same-named arrays on PointData and CellData are distinguished by attachment type.
bool VerifyAttachmentSelection() {
    auto mesh = CreateMeshWithCellVector();
    // Add a same-named Point vector ("cellVec" already exists on CellData).
    auto pv = iGame::FloatArray::New();
    pv->SetName("cellVec");
    pv->SetDimension(3);
    for (int i = 0; i < 5; ++i) {
        pv->AddElement3(1.f + 3.f * i, 2.f + 3.f * i, 3.f + 3.f * i);
    }
    mesh->GetAttributeSet()->AddVector(IG_POINT, pv);

    // Restrict to IG_POINT -> point values 1, 4, 7, 10, 13
    auto fP = iGame::ExtractComponentFilter::New();
    fP->SetInput(mesh);
    fP->SetInputArrayName("cellVec");
    fP->SetInputAttachmentType(IG_POINT);
    fP->SetOutputArrayName("RP");
    fP->SetComponent(0);
    if (!fP->Execute()) {
        std::cout << "FAIL: Point Execute (" << fP->GetMessage() << ")\n";
        return false;
    }
    auto outP = iGame::DynamicCast<iGame::UnstructuredMesh>(fP->GetOutput());
    if (outP == nullptr) return false;
    auto& attrP = outP->GetAttributeSet()->GetScalar("RP");
    bool pOk = (attrP.pointer != nullptr) && (attrP.attachmentType == IG_POINT)
               && (attrP.pointer->GetNumberOfElements() == 5);
    for (IGsize i = 0; pOk && i < attrP.pointer->GetNumberOfElements(); ++i) {
        pOk = (attrP.pointer->GetValue(i) == 1.0 + 3.0 * i);
    }
    if (!pOk) std::cout << "FAIL: Point attachment selection\n";

    // Restrict to IG_CELL -> cell values 10, 40
    auto fC = iGame::ExtractComponentFilter::New();
    fC->SetInput(mesh);
    fC->SetInputArrayName("cellVec");
    fC->SetInputAttachmentType(IG_CELL);
    fC->SetOutputArrayName("RC");
    fC->SetComponent(0);
    if (!fC->Execute()) {
        std::cout << "FAIL: Cell Execute (" << fC->GetMessage() << ")\n";
        return false;
    }
    auto outC = iGame::DynamicCast<iGame::UnstructuredMesh>(fC->GetOutput());
    if (outC == nullptr) return false;
    auto& attrC = outC->GetAttributeSet()->GetScalar("RC");
    bool cOk = (attrC.pointer != nullptr) && (attrC.attachmentType == IG_CELL)
               && (attrC.pointer->GetNumberOfElements() == 2);
    if (cOk) {
        cOk = (attrC.pointer->GetValue(0) == 10.0) && (attrC.pointer->GetValue(1) == 40.0);
    }
    if (!cOk) std::cout << "FAIL: Cell attachment selection\n";

    return pOk && cOk;
}
}  // namespace

int main(int argc, char* argv[]) {
    bool allOk = true;

    // 真实模型数据验证（带 argv 时，如：testExtractComponent.exe "<仓库根>/test/Streamline Test/StreamTest.vtk"）
    if (argc > 1) {
        auto mesh = CreateMesh(argc, argv);
        if (mesh == nullptr) return 1;

        bool defaultOk = VerifyRealData(mesh, "", 0, "Result");
        std::cout << (defaultOk ? "PASS" : "FAIL") << ": real data, empty input name -> first vector, X\n";
        allOk = allOk && defaultOk;

        bool explicitOk = VerifyRealData(mesh, "V", 1, "ResultV");
        std::cout << (explicitOk ? "PASS" : "FAIL") << ": real data, explicit input name V, Y\n";
        allOk = allOk && explicitOk;

        return allOk ? 0 : 1;
    }

    bool xOk = VerifyExtract(0, "Result", IG_POINT);
    std::cout << (xOk ? "PASS" : "FAIL") << ": extract X -> Result\n";
    allOk = allOk && xOk;

    bool yOk = VerifyExtract(1, "Result", IG_POINT);
    std::cout << (yOk ? "PASS" : "FAIL") << ": extract Y -> Result\n";
    allOk = allOk && yOk;

    bool zOk = VerifyExtract(2, "Result", IG_POINT);
    std::cout << (zOk ? "PASS" : "FAIL") << ": extract Z -> Result\n";
    allOk = allOk && zOk;

    bool customOk = VerifyExtract(0, "MyScalar", IG_POINT);
    std::cout << (customOk ? "PASS" : "FAIL") << ": custom output name MyScalar\n";
    allOk = allOk && customOk;

    bool cellOk = VerifyExtractCell();
    std::cout << (cellOk ? "PASS" : "FAIL") << ": cell data attachment (IG_CELL)\n";
    allOk = allOk && cellOk;

    bool defaultOk = VerifyInputArraySelection(false);
    std::cout << (defaultOk ? "PASS" : "FAIL") << ": empty input name -> first vector test_1\n";
    allOk = allOk && defaultOk;

    bool explicitOk = VerifyInputArraySelection(true);
    std::cout << (explicitOk ? "PASS" : "FAIL") << ": explicit input name test_2\n";
    allOk = allOk && explicitOk;

    bool dupOk = VerifyOverwriteDuplicateName();
    std::cout << (dupOk ? "PASS" : "FAIL") << ": duplicate output name overwritten (unique)\n";
    allOk = allOk && dupOk;

    bool dimOk = VerifyDimensionGuard();
    std::cout << (dimOk ? "PASS" : "FAIL") << ": dimension guard (1D/2D arrays)\n";
    allOk = allOk && dimOk;

    bool regOk = VerifyExtractOnExtractedResult();
    std::cout << (regOk ? "PASS" : "FAIL") << ": extract on extracted result (regression)\n";
    allOk = allOk && regOk;

    bool typeOk = VerifyOutputTypePreserved();
    std::cout << (typeOk ? "PASS" : "FAIL") << ": output type preserved (Int/LongLong)\n";
    allOk = allOk && typeOk;

    bool attachOk = VerifyAttachmentSelection();
    std::cout << (attachOk ? "PASS" : "FAIL") << ": Point/Cell attachment selection\n";
    allOk = allOk && attachOk;

    return allOk ? 0 : 1;
}
