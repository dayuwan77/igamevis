#include <PointLineInterpolator/iGamePointLineInterpolatorFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {
bool Near(double lhs, double rhs, double tolerance = 1e-5) { return std::abs(lhs - rhs) <= tolerance; }

bool Check(bool condition, const std::string& message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

iGame::PointSet::Pointer CreateSource() {
    auto source = iGame::PointSet::New();
    source->SetName("TwoPointCloud");
    source->AddPoint(iGame::Point(0.0, 0.0, 0.0));
    source->AddPoint(iGame::Point(2.0, 0.0, 0.0));

    auto scalar = iGame::DoubleArray::New();
    scalar->SetName("Temperature");
    scalar->SetDimension(1);
    scalar->AddValue(0.0);
    scalar->AddValue(20.0);
    source->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalar);

    auto vector = iGame::FloatArray::New();
    vector->SetName("Velocity");
    vector->SetDimension(3);
    vector->AddElement3(0.0, 0.0, 0.0);
    vector->AddElement3(2.0, 4.0, 6.0);
    source->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, vector);

    auto integer = iGame::IntArray::New();
    integer->SetName("IntegerSamples");
    integer->SetDimension(1);
    integer->AddValue(0);
    integer->AddValue(3);
    source->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, integer);
    return source;
}

bool TestParameterizedLineAndVoronoi() {
    std::cout << "Running parameterized-line and Voronoi tests..." << std::endl;
    auto source = CreateSource();
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(0.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(2.0, 0.0, 0.0));
    filter->SetResolution(4);
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::VORONOI);
    if (!Check(filter->Execute(), "Voronoi execution")) return false;

    auto output = filter->GetLineOutput();
    bool ok = Check(output != nullptr, "output exists");
    ok &= Check(output->GetNumberOfPoints() == 5, "Resolution 4 creates 5 points");
    ok &= Check(output->GetNumberOfCells() == 4, "Resolution 4 creates 4 line cells");
    for (int i = 0; i < 5; ++i) {
        const auto& point = output->GetPoint(i);
        ok &= Check(Near(point[0], 0.5 * i) && Near(point[1], 0.0) && Near(point[2], 0.0),
                    "parameterized sample coordinate " + std::to_string(i));
    }

    auto& temperature = output->GetAttributeSet()->GetAttribute("Temperature");
    auto& velocity = output->GetAttributeSet()->GetAttribute("Velocity");
    auto& integer = output->GetAttributeSet()->GetAttribute("IntegerSamples");
    ok &= Check(!temperature.IsNone() && temperature.pointer->GetArrayType() == IG_DoubleArray,
                "scalar name and type are preserved");
    ok &= Check(!velocity.IsNone() && velocity.pointer->GetDimension() == 3 &&
                        velocity.pointer->GetArrayType() == IG_FloatArray,
                "vector name, type, and components are preserved");
    ok &= Check(!integer.IsNone() && integer.pointer->GetArrayType() == IG_FloatArray,
                "integral arrays are promoted to float like VTK");
    ok &= Check(Near(temperature.pointer->GetValue(0), 0.0) && Near(temperature.pointer->GetValue(1), 0.0),
                "Voronoi samples nearest to the first source point");
    ok &= Check(Near(temperature.pointer->GetValue(3), 20.0) && Near(temperature.pointer->GetValue(4), 20.0),
                "Voronoi samples nearest to the second source point");
    const double tieValue = temperature.pointer->GetValue(2);
    ok &= Check(Near(tieValue, 0.0) || Near(tieValue, 20.0),
                "an equidistant Voronoi sample selects either nearest source point");
    ok &= Check(source->GetNumberOfPoints() == 2 &&
                        source->GetAttributeSet()->GetAttribute("Temperature").pointer->GetNumberOfElements() == 2,
                "input remains unchanged");

    filter->SetPoint1(iGame::Point(1.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(3.0, 0.0, 0.0));
    filter->SetResolution(2);
    ok &= Check(filter->Execute(), "repeated execution");
    ok &= Check(filter->GetLineOutput()->GetNumberOfPoints() == 3 &&
                        Near(filter->GetLineOutput()->GetPoint(0)[0], 1.0) &&
                        Near(filter->GetLineOutput()->GetPoint(2)[0], 3.0),
                "repeated execution rebuilds output");
    return ok;
}

bool TestGaussianAndShepard() {
    std::cout << "Running Gaussian and Shepard tests..." << std::endl;
    auto source = CreateSource();
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(1.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.5, 0.0, 0.0));
    filter->SetResolution(1);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
    filter->SetRadius(2.0);

    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetSharpness(2.0);
    bool ok = Check(filter->Execute(), "Gaussian execution");
    auto gaussian = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(gaussian->GetValue(0), 10.0), "Gaussian symmetric weights");
    auto promoted = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("IntegerSamples").pointer;
    ok &= Check(Near(promoted->GetValue(0), 1.5), "promoted arrays retain fractional interpolation values");

    filter->SetPoint1(iGame::Point(0.5, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.0, 0.0, 0.0));
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::SHEPARD);
    filter->SetPowerParameter(2.0);
    ok &= Check(filter->Execute(), "Shepard execution");
    auto shepard = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(shepard->GetValue(0), 2.0), "Shepard inverse-square weights");
    ok &= Check(Near(shepard->GetValue(1), 10.0), "Shepard symmetric weights");

    filter->SetPoint1(iGame::Point(0.25, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(1.75, 0.0, 0.0));
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::N_CLOSEST);
    filter->SetNumberOfPoints(1);
    ok &= Check(filter->Execute(), "N-closest footprint execution");
    auto nearest = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(nearest->GetValue(0), 0.0) && Near(nearest->GetValue(1), 20.0),
                "N-closest footprint limits the interpolation basis");
    return ok;
}

bool TestNullPointStrategiesAndInvalidParameters() {
    std::cout << "Running null-point and validation tests..." << std::endl;
    auto source = CreateSource();
    auto filter = iGame::PointLineInterpolatorFilter::New();
    filter->SetInput(source);
    filter->SetPoint1(iGame::Point(10.0, 0.0, 0.0));
    filter->SetPoint2(iGame::Point(11.0, 0.0, 0.0));
    filter->SetResolution(2);
    filter->SetKernelType(iGame::PointLineInterpolatorFilter::GAUSSIAN);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::RADIUS);
    filter->SetRadius(0.1);
    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::MASK_POINTS);
    filter->SetNullValue(-7.0);
    bool ok = Check(filter->Execute(), "mask null-points execution");
    auto output = filter->GetLineOutput();
    auto values = output->GetAttributeSet()->GetAttribute("Temperature").pointer;
    auto mask = output->GetAttributeSet()->GetAttribute("vtkValidPointMask").pointer;
    for (int i = 0; i < 3; ++i) {
        ok &= Check(Near(values->GetValue(i), -7.0), "null value " + std::to_string(i));
        ok &= Check(Near(mask->GetValue(i), 0.0), "invalid mask " + std::to_string(i));
    }

    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::CLOSEST_POINT);
    ok &= Check(filter->Execute(), "closest-point fallback execution");
    values = filter->GetLineOutput()->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(values->GetValue(0), 20.0), "closest-point fallback value");

    filter->SetNullPointsStrategy(iGame::PointLineInterpolatorFilter::NULL_VALUE);
    filter->SetNullValue(-9.0);
    ok &= Check(filter->Execute(), "null-value execution");
    output = filter->GetLineOutput();
    values = output->GetAttributeSet()->GetAttribute("Temperature").pointer;
    ok &= Check(Near(values->GetValue(0), -9.0), "explicit null value is emitted");
    ok &= Check(output->GetAttributeSet()->GetAttribute("vtkValidPointMask").IsNone(),
                "null-value strategy does not emit a validity mask");

    filter->SetResolution(0);
    ok &= Check(!filter->Execute(), "Resolution below one is rejected");
    filter->SetResolution(1);
    filter->SetRadius(0.0);
    ok &= Check(!filter->Execute(), "non-positive radius is rejected");
    filter->SetRadius(1.0);
    filter->SetKernelFootprint(iGame::PointLineInterpolatorFilter::N_CLOSEST);
    filter->SetNumberOfPoints(0);
    ok &= Check(!filter->Execute(), "empty N-closest footprint is rejected");
    return ok;
}
}

int main() {
    const bool ok = TestParameterizedLineAndVoronoi() && TestGaussianAndShepard() &&
                    TestNullPointStrategiesAndInvalidParameters();
    if (!ok) return 1;
    std::cout << "PointLineInterpolator acceptance tests passed.\n";
    return 0;
}
