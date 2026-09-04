#include "Interpolation/iGamePointVolumeInterpolatorFilter.h"
#include "iGameAttributeSet.h"
#include "iGameBoundingBox.h"
#include "iGameFileIO.h"
#include "iGamePointSet.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"
#include "iGameType.h"
#include "iGameUnstructuredMesh.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// 守卫：核心 Filter 只允许插值 PointData。若允许选到 CellData，
// 会以点 ID 访问长度=单元数的数组而越界。
namespace {
bool AttributeSelectionGuardChecks() {
    std::cerr << "\n[Guard] filter attribute must be PointData:\n";
    bool ok = true;

    auto mesh = iGame::UnstructuredMesh::New();
    auto pts = mesh->GetPoints();
    pts->AddPoint(iGame::Point(0.f, 0.f, 0.f));
    pts->AddPoint(iGame::Point(1.f, 0.f, 0.f));
    pts->AddPoint(iGame::Point(0.f, 1.f, 0.f));
    pts->AddPoint(iGame::Point(0.f, 0.f, 1.f));
    igIndex tet[4] = {0, 1, 2, 3};
    mesh->AddCell(tet, 4, iGame::IG_TETRA);

    auto pa = iGame::FloatArray::New();
    pa->SetName("PtA");
    pa->SetDimension(1);
    pa->Resize(4);
    for (IGsize i = 0; i < 4; ++i) pa->SetValue(i, double(i));
    mesh->GetAttributeSet()->AddScalar(IG_POINT, pa);

    auto ca = iGame::FloatArray::New();
    ca->SetName("CellA");
    ca->SetDimension(1);
    ca->Resize(1);
    ca->SetValue(0, 99.0);
    mesh->GetAttributeSet()->AddScalar(IG_CELL, ca);

    auto query = iGame::PointSet::New();
    query->AddPoint(iGame::Point(0.25f, 0.25f, 0.25f));

    auto makeFilter = [&]() {
        auto f = iGame::PointVolumeInterpolatorFilter::New();
        f->SetInput(mesh);
        f->SetInput(1, query);
        return f;
    };

    {
        auto f = makeFilter();
        f->SetAttributeByName("CellA");
        const bool rejected = !f->Execute();
        std::cerr << (rejected ? "  -> PASS" : "  -> FAIL")
                  << " : selecting CellData by name is rejected\n";
        ok = ok && rejected;
    }
    {
        auto f = makeFilter();
        f->SetAttributeByIndex(0);
        const bool succeeded = f->Execute();
        std::cerr << (succeeded ? "  -> PASS" : "  -> FAIL")
                  << " : selecting PointData by index works\n";
        ok = ok && succeeded;
    }
    {
        auto f = makeFilter();
        f->SetAttributeByName("PtA");
        const bool succeeded = f->Execute();
        std::cerr << (succeeded ? "  -> PASS" : "  -> FAIL")
                  << " : selecting PointData by name works\n";
        ok = ok && succeeded;
    }
    {
        auto f = makeFilter();
        f->SetAttributeByIndex(7); // 越界（点属性只有 1 个）
        const bool rejected = !f->Execute();
        std::cerr << (rejected ? "  -> PASS" : "  -> FAIL")
                  << " : out-of-range point attribute index is rejected\n";
        ok = ok && rejected;
    }
    return ok;
}
} // namespace

int main() {
    if (!AttributeSelectionGuardChecks()) {
        std::cerr << "\nGuard FAILED, aborting.\n";
        return 1;
    }

    auto scene = iGame::Scene::New();

    // 读取一个体网格（含点标量属性）
    const std::string fileName = "./Models/Tet_Plane.vtk";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj == nullptr) {
        igError("Error reading the file");
        return 0;
    }
    auto umesh = iGame::DynamicCast<iGame::UnstructuredMesh>(dataObj);
    if (umesh == nullptr) {
        igError("Error: expected an UnstructuredMesh");
        return 0;
    }
    scene->AddModel(dataObj);

    // 飞机表面按 test_1 模长上色，并半透明显示
    auto meshDraw = iGame::DynamicCast<iGame::DrawObject>(dataObj);
    if (meshDraw) {
        meshDraw->SetViewStyle(IG_SURFACE);
        meshDraw->ViewCloudPicture(scene, 0, -1);
        meshDraw->SetTransparency(0.5f);
    }

    const auto& box = dataObj->GetBoundingBox();
    std::cout << "mesh bounding box: min(" << box.min[0] << ", " << box.min[1] << ", " << box.min[2]
              << ") max(" << box.max[0] << ", " << box.max[1] << ", " << box.max[2] << ")\n";

    auto meshPts = umesh->GetPoints();
    const IGsize numMeshPts = meshPts->GetNumberOfPoints();
    const IGsize numCells = umesh->GetNumberOfCells();
    std::cout << "mesh points: " << numMeshPts << ", cells: " << numCells << "\n";

    // 网格的点属性（POINT_DATA test_1）作为插值比对基准
    auto pointAttrs = umesh->GetAttributeSet()->GetAllPointAttributes();
    if (pointAttrs->GetNumberOfElements() == 0) {
        igError("Error: mesh has no point attribute");
        return 0;
    }
    iGame::ArrayObject* meshPointData = pointAttrs->GetElement(0).pointer.get();
    const int dim = meshPointData->GetDimension();

    double lo = 1e30, hi = -1e30;
    for (IGsize i = 0; i < numMeshPts; ++i) {
        for (int d = 0; d < dim; ++d) {
            const double v = meshPointData->GetElementValue(i, d);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    }
    std::cout << "point attribute test_1: dim = " << dim << ", range [" << lo << ", " << hi << "]\n";

    // 运行一次插值过滤器：query 为查询点集，返回插值属性与命中掩码

    auto runInterp = [&](iGame::PointSet::Pointer query, iGame::ArrayObject::Pointer& outData,
                         iGame::ArrayObject::Pointer& outHit) {
        auto filter = iGame::PointVolumeInterpolatorFilter::New();
        filter->SetInput(dataObj);
        filter->SetInput(1, query);
        filter->SetAttributeByIndex(0);
        filter->Execute();
        auto result = filter->GetOutput();
        if (result == nullptr) {
            igError("Error: filter produced no output");
            return;
        }
        auto attrSet = result->GetAttributeSet();
        outData = attrSet->GetAttribute(0).pointer;
        outHit = attrSet->GetAttribute(iGame::PointVolumeInterpolatorFilter::HitMaskName).pointer;
    };

    const double kTol = 1e-4;

    // 检验插值计算本身是否正确
    // 以网格全部顶点为查询点，因顶点同时属于多个单元，无论选中哪个包含它的单元
    // 其重心坐标都是 (1,0,0,0)，插值结果必精确还原顶点存储的点属性
    auto vertexQuery = iGame::PointSet::New();
    for (IGsize i = 0; i < numMeshPts; ++i) vertexQuery->AddPoint(meshPts->GetPoint(i));
    iGame::ArrayObject::Pointer vData, vHit;
    runInterp(vertexQuery, vData, vHit);

    double maxVertexErr = 0.0;
    IGsize vertexMiss = 0;
    for (IGsize i = 0; i < numMeshPts; ++i) {
        if (vHit->GetElementValue(i, 0) == 0.0) { ++vertexMiss; continue; }
        for (int d = 0; d < dim; ++d) {
            const double expected = meshPointData->GetElementValue(i, d);
            const double actual = vData->GetElementValue(i, d);
            maxVertexErr = std::max(maxVertexErr, std::abs(actual - expected));
        }
    }
    std::cerr << "\n[Verify A] vertex exact reproduction: " << numMeshPts << " vertices, miss " << vertexMiss
              << ", max error = " << maxVertexErr
              << (maxVertexErr < kTol ? "  -> PASS" : "  -> FAIL") << "\n";

    // 检验查询点定位到的包围它的四面体/六面体有没有选错
    // 以每个四面体中心为查询点，质心只属于唯一一个单元，若定位选错单元
    // 会用其他单元的顶点做加权，误差立刻暴露；选对时线性插值结果应等于四个顶点值的平均值
    auto centroidQuery = iGame::PointSet::New();
    std::vector<double> expected(numCells * dim, 0.0);
    igIndex cellIds[IGAME_CELL_MAX_SIZE];
    for (IGsize c = 0; c < numCells; ++c) {
        const int npt = umesh->GetCellPointIds(c, cellIds);
        double cx = 0, cy = 0, cz = 0;
        for (int j = 0; j < npt; ++j) {
            auto p = umesh->GetPoint(cellIds[j]);
            cx += p[0]; cy += p[1]; cz += p[2];
            for (int d = 0; d < dim; ++d) expected[c * dim + d] += meshPointData->GetElementValue(cellIds[j], d);
        }
        for (int d = 0; d < dim; ++d) expected[c * dim + d] /= npt;
        centroidQuery->AddPoint(iGame::Point(float(cx / npt), float(cy / npt), float(cz / npt)));
    }
    iGame::ArrayObject::Pointer cData, cHit;
    runInterp(centroidQuery, cData, cHit);

    double maxCentroidErr = 0.0;
    IGsize centroidMiss = 0;
    for (IGsize c = 0; c < numCells; ++c) {
        if (cHit->GetElementValue(c, 0) == 0.0) { ++centroidMiss; continue; }
        for (int d = 0; d < dim; ++d) {
            const double actual = cData->GetElementValue(c, d);
            maxCentroidErr = std::max(maxCentroidErr, std::abs(actual - expected[c * dim + d]));
        }
    }
    std::cerr << "[Verify B] tetra centroid linearity: " << numCells << " centroids, miss " << centroidMiss
              << ", max error = " << maxCentroidErr
              << (maxCentroidErr < kTol ? "  -> PASS" : "  -> FAIL") << "\n";




    const double subFrac = 0.35;  // 子区域各向为包围盒尺寸的 70%
    const double cxx = (box.min[0] + box.max[0]) * 0.5;
    const double cyy = (box.min[1] + box.max[1]) * 0.5;
    const double czz = (box.min[2] + box.max[2]) * 0.5;
    const double hxx = (box.max[0] - box.min[0]) * subFrac;
    const double hyy = (box.max[1] - box.min[1]) * subFrac;
    const double hzz = (box.max[2] - box.min[2]) * subFrac;
    const iGame::Point subBoxMin{float(cxx - hxx), float(cyy - hyy), float(czz - hzz)};
    const iGame::Point subBoxMax{float(cxx + hxx), float(cyy + hyy), float(czz + hzz)};
    std::cout << "query sub-box: min(" << subBoxMin[0] << ", " << subBoxMin[1] << ", " << subBoxMin[2]
              << ") max(" << subBoxMax[0] << ", " << subBoxMax[1] << ", " << subBoxMax[2] << ")\n";


    const int N = 50;
    const int NZ = 10;
    auto querySet = iGame::PointSet::New();
    for (int k = 0; k < NZ; ++k) {
        double z = subBoxMin[2] + (subBoxMax[2] - subBoxMin[2]) * k / (NZ - 1);
        for (int j = 0; j < N; ++j) {
            double y = subBoxMin[1] + (subBoxMax[1] - subBoxMin[1]) * j / (N - 1);
            for (int i = 0; i < N; ++i) {
                double x = subBoxMin[0] + (subBoxMax[0] - subBoxMin[0]) * i / (N - 1);
                querySet->AddPoint(iGame::Point(float(x), float(y), float(z)));
            }
        }
    }
    std::cout << "query points: " << querySet->GetNumberOfPoints() << "\n";

    iGame::ArrayObject::Pointer hitData, interpData;
    runInterp(querySet, interpData, hitData);

    // 只保留命中的查询点，按其插值结果上色显示
    auto display = iGame::PointSet::New();
    auto displayVals = iGame::FloatArray::New();
    displayVals->SetDimension(dim);
    std::vector<float> val(dim);
    int hitCount = 0;
    int sampleCount = 0;
    const IGsize numQuery = querySet->GetNumberOfPoints();
    for (IGsize k = 0; k < numQuery; ++k) {
        if (hitData->GetElementValue(k, 0) == 0.0) continue;
        auto p = querySet->GetPoint(k);
        display->AddPoint(p);
        interpData->GetElement(k, val.data());
        displayVals->AddElement(val);
        hitCount++;
        if (sampleCount < 5) {
            std::cout << "sample query (" << p[0] << ", " << p[1] << ", " << p[2] << ") -> value =";
            for (int d = 0; d < dim; ++d) std::cout << " " << interpData->GetElementValue(k, d);
            std::cout << "\n";
            sampleCount++;
        }
    }
    std::cerr << "hit cells: " << hitCount << " / " << numQuery << "\n";

    // 点集默认不可见，需显式开启点模式、放大点尺寸，并按插值属性上色
    auto displayDraw = iGame::DynamicCast<iGame::DrawObject>(display);
    if (displayDraw != nullptr) {
        if (dim == 1) {
            display->GetAttributeSet()->AddScalar(IG_POINT, displayVals);
        } else {
            display->GetAttributeSet()->AddVector(IG_POINT, displayVals);
        }
        displayDraw->SetViewStyle(IG_POINTS);
        displayDraw->SetPointSize(6);
        scene->AddModel(display);
        displayDraw->ViewCloudPicture(scene, 0, -1);
    }

    // 用绿色线框画出查询子区域的范围
    {
        auto frame = iGame::UnstructuredMesh::New();
        const float x0 = subBoxMin[0], y0 = subBoxMin[1], z0 = subBoxMin[2];
        const float x1 = subBoxMax[0], y1 = subBoxMax[1], z1 = subBoxMax[2];
        iGame::Point corners[8] = {
            iGame::Point(x0, y0, z0), iGame::Point(x1, y0, z0),
            iGame::Point(x1, y1, z0), iGame::Point(x0, y1, z0),
            iGame::Point(x0, y0, z1), iGame::Point(x1, y0, z1),
            iGame::Point(x1, y1, z1), iGame::Point(x0, y1, z1),
        };
        for (auto& c : corners) frame->AddPoint(c);
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                  {4, 5}, {5, 6}, {6, 7}, {7, 4},
                                  {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (auto& e : edges) {
            igIndex ids[2] = {igIndex(e[0]), igIndex(e[1])};
            frame->AddCell(ids, 2, iGame::IG_LINE);
        }
        frame->SetViewStyle(IG_WIREFRAME);
        frame->SetLineColor(igm::vec3(0.0f, 1.0f, 0.0f));
        frame->SetLineWidth(1.0f);
        scene->AddModel(frame);
    }

    std::cout << std::flush;
    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}
