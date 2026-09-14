#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>
#include <iGameFileIO.h>
#include <iGamePointSet.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>

#include <iostream>
#include <string>

namespace {

// 相对路径：Examples 构建目录会自动把 Examples/Models 拷贝为 ./Models
const std::string kModelA = "./Models/ForceStaticMesh_TestA.vtk";
const std::string kModelB = "./Models/ForceStaticMesh_TestB.vtk";

} // namespace

int main() {
    using namespace iGame;

    // 两个点/单元数相同、几何不同的独立模型文件（模拟“规模相同的另一个模型”）
    auto meshA = DynamicCast<UnstructuredMesh>(FileIO::ReadFile(kModelA));
    auto meshB = DynamicCast<UnstructuredMesh>(FileIO::ReadFile(kModelB));
    if (!meshA || !meshB) {
        std::cerr << "FAIL: read model files\n";
        return 1;
    }
    if (meshA->GetNumberOfPoints() != 27 || meshA->GetNumberOfCells() != 8) {
        std::cerr << "FAIL: unexpected model size\n";
        return 1;
    }
    if (meshB->GetNumberOfPoints() != meshA->GetNumberOfPoints() ||
        meshB->GetNumberOfCells() != meshA->GetNumberOfCells()) {
        std::cerr << "FAIL: models should have same point/cell counts\n";
        return 1;
    }

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
    if (!ps || ps->GetNumberOfPoints() != 27) {
        std::cerr << "FAIL: rebuilt cache point count\n";
        return 1;
    }
    // meshB 的原点整体偏移到 (10,0,0)
    if (ps->GetPoint(0)[0] != 10.f) {
        std::cerr << "FAIL: rebuilt cache geometry does not match new input\n";
        return 1;
    }
    std::cout << "different-input cache rebuild: yes\n";

    std::cout << "Result: PASS\n";
    return 0;
}
