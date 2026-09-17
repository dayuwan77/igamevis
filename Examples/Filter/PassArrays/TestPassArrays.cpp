#include <PassArrays/iGamePassArraysFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a new scene
    auto scene = iGame::Scene::New();

    // Read the file and add it to the scene
    const std::string fileName = "./Models/kit.vtk";

    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (!obj) {
        std::cout << "Failed to read file: " << fileName << std::endl;
        std::cin.get();
        return 1;
    }
    std::cout << "Before modified:" << std::endl;
    auto attrSet = obj->GetAttributeSet();
    if (attrSet) {
        // 输出点属性名称
        auto pointAttrs = attrSet->GetAllPointAttributes();
        if (pointAttrs) {
            std::cout << "\nPoint AttributSet:\n";
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                // 输出属性名、元组数、分量数
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }

        // 输出单元属性名称
        auto cellAttrs = attrSet->GetAllCellAttributes();
        if (cellAttrs) {
            std::cout << "\nCell AttributeSet:\n";
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "AttributeSet is null.\n";
    }
    std::vector<std::string> stdNames = {"v1", "u1", "w1"};

    auto filter = iGame::PassArrays::New();
    filter->SetArrayNames(stdNames);
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "Filter ERROR!\n";
        std::cin.get();
        return -1;
    }
    obj = filter->GetOutput();
    scene->AddModel(obj);
    std::cout << "After modified:" << std::endl;
    attrSet = obj->GetAttributeSet();
    if (attrSet) {
        // 输出点属性名称
        auto pointAttrs = attrSet->GetAllPointAttributes();
        if (pointAttrs) {
            std::cout << "\nPoint AttributSet:\n";
            for (IGsize i = 0; i < pointAttrs->GetNumberOfElements(); ++i) {
                auto& attr = pointAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                // 输出属性名、元组数、分量数
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }

        // 输出单元属性名称
        auto cellAttrs = attrSet->GetAllCellAttributes();
        if (cellAttrs) {
            std::cout << "\nCell AttributeSet\n";
            for (IGsize i = 0; i < cellAttrs->GetNumberOfElements(); ++i) {
                auto& attr = cellAttrs->GetElement(i);
                if (attr.IsNone()) continue;
                std::cout << "  " << attr.pointer->GetName() << " (tuples=" << attr.pointer->GetNumberOfElements()
                          << ", comps=" << attr.pointer->GetDimension() << ")" << std::endl;
            }
        }
    } else {
        std::cout << "AttributeSet is null.\n";
    }
    std::cin.get();
}