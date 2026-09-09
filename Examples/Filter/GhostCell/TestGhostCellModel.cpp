#include <GhostCell/iGameGhostCellFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameType.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool FileExists(const std::string& path) {
	std::ifstream f(path);
	return f.good();
}

// 依次尝试常见运行目录，找到模型后返回完整相对路径
std::string FindModel(const std::string& name) {
	const std::string candidates[] = {
	    "./Examples/Models/" + name,
	    "./Models/" + name,
	    "../Examples/Models/" + name,
	};
	for (const auto& c : candidates) {
		if (FileExists(c)) return c;
	}
	return "";
}

bool Check(bool ok, const std::string& name) {
	if (ok) {
		std::cout << "[PASS] " << name << std::endl;
	} else {
		std::cout << "[FAIL] " << name << std::endl;
	}
	return ok;
}

void Step(const std::string& name) { std::cout << "  >> " << name << std::endl; }

// 读取模型上的 GhostCells 属性，逐个单元打印并返回
bool ReadGhostCells(iGame::DataObject::Pointer obj, std::vector<double>& out) {
	auto attrs = obj->GetAttributeSet();
	if (attrs == nullptr) return false;
	int idx = attrs->GetAttributeIndex("GhostCells");
	if (idx < 0) return false;
	auto marker = iGame::DynamicCast<iGame::CharArray>(attrs->GetAttribute(idx).pointer);
	if (marker.IsNull()) return false;
	out.clear();
	for (IGsize i = 0; i < marker->GetNumberOfElements(); i++) {
		out.push_back(marker->GetValue(i));
	}
	return true;
}

// 运行一次 GhostCellFilter 并核对期望值
bool RunCase(const std::string& modelName, const std::vector<double>& expected, const std::string& title) {
	std::cout << "\n== " << title << " ==" << std::endl;

	Step("find model");
	std::string path = FindModel(modelName);
	if (path.empty()) {
		std::cout << "MODEL NOT FOUND: " << modelName << std::endl;
		return false;
	}

	Step("read " + path);
	auto obj = iGame::FileIO::ReadFile(path);
	if (obj.IsNull()) {
		std::cout << "READ FAILED" << std::endl;
		return false;
	}

	Step("run filter");
	auto filter = iGame::GhostCellFilter::New();
	filter->SetInput(0, obj);
	if (!Check(filter->Execute(), "filter Execute()")) return false;

	Step("check result");
	std::vector<double> actual;
	if (!Check(ReadGhostCells(obj, actual), "read GhostCells array")) return false;
	if (!Check(actual.size() == expected.size(), "cell count matches expected")) return false;

	bool ok = true;
	std::cout << "GhostCells = [";
	for (size_t i = 0; i < actual.size(); i++) {
		std::cout << " " << actual[i];
		ok &= Check(actual[i] == expected[i], "cell " + std::to_string(i) +
		                                          " value = " + std::to_string(expected[i]));
	}
	std::cout << " ]" << std::endl;
	return ok;
}

}  // namespace

int main() {
	bool ok = true;
	// 金字塔：四个侧面都含 ghost 顶点(4)，底面两个三角形正常
	ok &= RunCase("GhostCell_Pyramid.vtk", {1, 1, 1, 1, 0, 0}, "Test 1: pyramid model");
	// 两个四面体：tet0 不含 ghost 点，tet1 含 ghost 点(4)
	ok &= RunCase("GhostCell_TwoTets.vtk", {0, 1}, "Test 2: two-tetrahedra model");

	if (ok) {
		std::cout << "\nALL TESTS PASSED" << std::endl;
		return 0;
	}
	std::cout << "\nSOME TESTS FAILED" << std::endl;
	return 1;
}
