#include <GhostCell/iGameGhostCellFilter.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameSurfaceMesh.h>
#include <iGameType.h>
#include <iGameVolumeMesh.h>

#include <fstream>
#include <iostream>
#include <string>

namespace {

bool Check(bool ok, const std::string& name) {
	if (ok) {
		std::cout << "[PASS] " << name << std::endl;
	} else {
		std::cout << "[FAIL] " << name << std::endl;
	}
	return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

// 场景一：读取演示模型（表面网格），运行 filter 并打印每个单元的 GhostCells
bool RunSurfaceDemo() {
	std::cout << "== Test 1: surface mesh from file ==" << std::endl;

	Step("locate GhostDemo.vtk");
	const char* candidates[] = {
	    "./Models/GhostDemo.vtk",           
	    "./Examples/Models/GhostDemo.vtk",  
	    "../Examples/Models/GhostDemo.vtk", 
	};
	std::string path;
	for (auto c : candidates) {
		std::ifstream f(c);
		if (f.good()) {
			path = c;
			break;
		}
	}
	if (path.empty()) {
		std::cout << "MODEL NOT FOUND" << std::endl;
		return false;
	}

	Step("read " + path);
	auto obj = iGame::FileIO::ReadFile(path);
	if (obj.IsNull()) {
		std::cout << "READ FAILED" << std::endl;
		return false;
	}
	auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(obj);
	if (mesh.IsNull()) {
		std::cout << "NOT A SURFACE MESH" << std::endl;
		return false;
	}
	std::cout << "points = " << mesh->GetNumberOfPoints()
	          << ", faces = " << mesh->GetNumberOfFaces() << std::endl;

	Step("run filter");
	auto filter = iGame::GhostCellFilter::New();
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	auto attrs = mesh->GetAttributeSet();
	int idx = attrs->GetAttributeIndex("GhostCells");
	if (!Check(idx >= 0, "GhostCells array exists")) return false;
	auto marker = iGame::DynamicCast<iGame::CharArray>(attrs->GetAttribute(idx).pointer);
	if (!Check(!marker.IsNull(), "GhostCells is a CharArray")) return false;

	bool ok = true;
	ok &= Check(marker->GetNumberOfElements() == 2, "one value per face");
	ok &= Check(marker->GetValue(0) == 0.0, "face 0 is normal (0)");
	ok &= Check(marker->GetValue(1) == 1.0, "face 1 is ghost (1)");
	std::cout << "cell values:";
	for (IGsize i = 0; i < marker->GetNumberOfElements(); i++) {
		std::cout << " " << marker->GetValue(i);
	}
	std::cout << std::endl;
	return ok;
}

// 场景二：体网格
bool TestVolumeMesh() {
	std::cout << "\n== Test 2: volume mesh ==" << std::endl;

	Step("create volume mesh");
	auto mesh = iGame::VolumeMesh::New();
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 0.0f)); // 0
	mesh->AddPoint(iGame::Point(1.0f, 0.0f, 0.0f)); // 1
	mesh->AddPoint(iGame::Point(0.0f, 1.0f, 0.0f)); // 2
	mesh->AddPoint(iGame::Point(0.0f, 0.0f, 1.0f)); // 3  ghost point
	mesh->AddPoint(iGame::Point(1.0f, 1.0f, 1.0f)); // 4

	igIndex tet0[4] = {0, 1, 2, 4};
	igIndex tet1[4] = {0, 1, 2, 3};
	auto volumes = iGame::CellArray::New();
	volumes->AddCellIds(tet0, 4);
	volumes->AddCellIds(tet1, 4);
	mesh->SetVolumes(volumes);

	Step("create GhostPoints array");
	auto pointGhosts = iGame::CharArray::New();
	pointGhosts->SetName("GhostPoints");
	pointGhosts->Resize(5);
	pointGhosts->SetValue(0, 0.0);
	pointGhosts->SetValue(1, 0.0);
	pointGhosts->SetValue(2, 0.0);
	pointGhosts->SetValue(3, 1.0);
	pointGhosts->SetValue(4, 0.0);
	mesh->GetAttributeSet()->AddScalar(IG_POINT, pointGhosts);

	Step("run filter");
	auto filter = iGame::GhostCellFilter::New();
	filter->SetInput(0, mesh);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	auto attrs = mesh->GetAttributeSet();
	int idx = attrs->GetAttributeIndex("GhostCells");
	if (!Check(idx >= 0, "GhostCells array exists")) return false;
	auto marker = iGame::DynamicCast<iGame::CharArray>(attrs->GetAttribute(idx).pointer);
	if (!Check(!marker.IsNull(), "GhostCells is a CharArray")) return false;

	bool ok = true;
	ok &= Check(marker->GetNumberOfElements() == 2, "one value per volume");
	ok &= Check(marker->GetValue(0) == 0.0, "volume 0 is normal (0)");
	ok &= Check(marker->GetValue(1) == 1.0, "volume 1 is ghost (1)");
	std::cout << "GhostCells = [ " << marker->GetValue(0) << ", "
	          << marker->GetValue(1) << " ]" << std::endl;
	return ok;
}

}  

int main() {
	bool ok = true;
	ok &= RunSurfaceDemo();
	ok &= TestVolumeMesh();

	if (ok) {
		std::cout << "\nALL TESTS PASSED" << std::endl;
		return 0;
	}
	std::cout << "\nSOME TESTS FAILED" << std::endl;
	return 1;
}
