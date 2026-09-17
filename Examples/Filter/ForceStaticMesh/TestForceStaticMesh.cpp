#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>

namespace {

iGame::UnstructuredMesh::Pointer MakeTriangleMesh(const iGame::Point& p0, const iGame::Point& p1,
                                                  const iGame::Point& p2, const char* scalarName, float value) {
    using namespace iGame;
    auto mesh = UnstructuredMesh::New();
    mesh->AddPoint(p0);
    mesh->AddPoint(p1);
    mesh->AddPoint(p2);
    igIndex cell[3] = {0, 1, 2};
    mesh->AddCell(cell, 3, IG_TRIANGLE);

    auto attr = FloatArray::New();
    attr->SetName(scalarName);
    attr->SetDimension(1);
    attr->AddValue(value);
    attr->AddValue(value);
    attr->AddValue(value);
    mesh->GetAttributeSet()->AddScalar(IG_POINT, attr);
    return mesh;
}

} // namespace

int main() {
    using namespace iGame;

    // 两个点/单元数相同、几何不同的独立网格对象（模拟“规模相同的另一个模型”）
    auto meshA = MakeTriangleMesh(Point(0, 0, 0), Point(1, 0, 0), Point(0, 1, 0), "scalarA", 1.f);
    auto meshB = MakeTriangleMesh(Point(10, 0, 0), Point(11, 0, 0), Point(10, 1, 0), "scalarB", 2.f);

    auto filter = ForceStaticMeshFilter::New();

    // 场景 1：同一输入对象多次执行 → 复用缓存（输出为同一对象）
    filter->SetInput(meshA);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshA #1\n"; return 1; }
    auto out1 = filter->GetOutput();

    filter->SetInput(meshA);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshA #2\n"; return 1; }
    auto out2 = filter->GetOutput();
    if (out1.get() != out2.get()) {
        std::cerr << "FAIL: same input should reuse cache\n";
        return 1;
    }
    std::cout << "same-input cache reuse: yes\n";

    // 场景 2：切换到规模相同的另一个输入对象 → 强制重建缓存（输出为新对象，几何对应 meshB）
    filter->SetInput(meshB);
    if (!filter->Execute()) { std::cerr << "FAIL: execute meshB\n"; return 1; }
    auto out3 = filter->GetOutput();
    if (out2.get() == out3.get()) {
        std::cerr << "FAIL: different input object should rebuild cache\n";
        return 1;
    }
    auto ps = DynamicCast<PointSet>(out3);
    if (!ps || ps->GetNumberOfPoints() != 3) {
        std::cerr << "FAIL: rebuilt cache point count\n";
        return 1;
    }
    if (ps->GetPoint(0)[0] != 10.f) {
        std::cerr << "FAIL: rebuilt cache geometry does not match new input\n";
        return 1;
    }
    auto& attrB = out3->GetAttributeSet()->GetAttribute("scalarB");
    if (attrB.pointer == nullptr) {
        std::cerr << "FAIL: rebuilt cache attributes do not match new input\n";
        return 1;
    }
    std::cout << "different-input cache rebuild: yes\n";

    std::cout << "Result: PASS\n";
    return 0;
}
