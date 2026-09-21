#include "FeatureExtraction/iGameOutlineCornerFilter.h"
#include "iGameFileIO.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckModel(const char* path, const float bounds[6], float factor) {
    auto input = iGame::FileIO::ReadFile(path);
    Require(!input.IsNull(), "Failed to read model; run from the example directory.");
    auto originalPoints = input->GetPoints();
    auto* originalAttributes = input->GetAttributeSet();
    const auto attributeCount = originalAttributes->GetNumberOfAttributes();
    auto filter = iGame::OutlineCornerFilter::New();
    filter->SetInput(input);
    filter->SetCornerFactor(factor);
    Require(filter->Execute(), "OutlineCornerFilter failed.");
    const auto result = filter->GetResult();
    Require(result && result != input && filter->GetOutput() == result,
            "Expected an independent output.");
    Require(result->GetNumberOfPoints() == 32 && result->GetNumberOfCells() == 24,
            "Expected 32 points and 24 line cells.");
    Require(input->GetPoints() == originalPoints
            && input->GetAttributeSet() == originalAttributes
            && originalAttributes->GetNumberOfAttributes() == attributeCount,
            "Input geometry or attributes were replaced.");

    for (int corner = 0; corner < 8; ++corner) {
        for (int axis = 0; axis < 3; ++axis) {
            const int side = (corner >> axis) & 1;
            const float expected = bounds[axis * 2 + side];
            Require(std::abs(result->GetPoint(corner * 4)[axis] - expected) < 1e-5f,
                    "Wrong corner coordinate.");
            for (int arm = 0; arm < 3; ++arm) {
                const float length = (bounds[axis * 2 + 1] - bounds[axis * 2]) * factor;
                const float end = expected + (axis == arm ? (side ? -length : length) : 0);
                Require(std::abs(result->GetPoint(corner * 4 + arm + 1)[axis] - end) < 1e-5f,
                        "Wrong corner-arm endpoint.");
            }
        }
        for (int arm = 0; arm < 3; ++arm) {
            const igIndex* ids = nullptr;
            Require(result->GetCellPointIds(corner * 3 + arm, ids) == 2 && ids
                    && ids[0] == corner * 4 && ids[1] == corner * 4 + arm + 1,
                    "Wrong line connectivity.");
        }
    }
    std::cout << "Input: " << path << '\n'
              << "Corner factor: " << factor << '\n'
              << "Points: 32, line cells: 24\n"
              << "Corner coordinates, arm lengths and input preservation: PASS\n";
}
} // namespace

int main() {
    try {
        const float boxBounds[6] = {-2, 4, 1, 5, -3, 7};
        const float planeBounds[6] = {-3, 5, -2, 4, 2, 2};
        CheckModel("Models/OutlineCorners_Box.vtk", boxBounds, 0.2f);
        CheckModel("Models/OutlineCorners_Box.vtk", boxBounds, 0.5f);
        CheckModel("Models/OutlineCorners_Plane.vtk", planeBounds, 0.2f);
        std::cout << "OutlineCorners: all tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
