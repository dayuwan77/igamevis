#include <IQWidgets/igQtTriangleStripWidget.h>
#include <iGameFileIO.h>

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QFontDatabase>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
constexpr const char* ModelFilePath = "Models/TriangleStripTestModel.vtk";
constexpr const char* ExpectedTriangleCount = "8";
constexpr IGsize ExpectedBoundarySegmentCount = 10;
constexpr int ExpectedJoinedPointCount = 11;

void Check(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

template <class T>
T* Control(QWidget* panel, const char* name) {
    auto* result = panel->findChild<T*>(QString::fromLatin1(name));
    Check(result != nullptr, name);
    return result;
}

}

int main(int argc, char** argv) {
    Q_INIT_RESOURCE(iGameQtMainWindow);
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) { qputenv("QT_QPA_PLATFORM", "offscreen"); }
    QApplication app(argc, argv);
    try {
        // Windows' offscreen platform does not enumerate system fonts. Use the
        // same bundled Chinese font as the application for meaningful layout QA.
        const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/Styles/Styles/SourceHanSansCN-Normal.otf"));
        Check(fontId >= 0, "Cannot load the bundled UI font");
        const auto families = QFontDatabase::applicationFontFamilies(fontId);
        Check(!families.isEmpty(), "The bundled font has no family");
        app.setFont(QFont(families.first(), 10));
        iGame::Log::Init();
        std::unique_ptr<QDockWidget> dock(igQtTriangleStripWidget::createDockWidget(nullptr));
        auto* panel = dock->findChild<igQtTriangleStripWidget*>();
        Check(panel != nullptr, "Missing triangle-strip panel");
        auto* length = Control<QSpinBox>(panel, "maximumStripLength");
        auto* slider = Control<QSlider>(panel, "maximumStripLengthSlider");
        auto* join = Control<QCheckBox>(panel, "joinContiguousPolyLines");
        auto* apply = Control<QPushButton>(panel, "applyTriangleStrip");
        Check(length->value() == 1000 && !join->isChecked(), "Wrong parameter defaults");
        Check(!apply->isEnabled() && !panel->apply(), "Empty input should not execute");
        length->setValue(0);
        Check(length->value() == 1 && slider->value() == 1, "Invalid length was not clamped");
        slider->setValue(1000);
        Check(length->value() == 1000, "Slider/spinbox are not synchronized");

        int resultCount = 0;
        iGame::DataObject::Pointer lastSurface;
        QObject::connect(panel, &igQtTriangleStripWidget::resultReady,
                         [&](iGame::DataObject::Pointer surface, iGame::DataObject::Pointer lines) {
                             ++resultCount;
                             lastSurface = surface;
                             Check(panel->isOutput(surface), "Surface output is not recognized");
                             Check(!lines || panel->isOutput(lines), "Line output is not recognized");
                         });

        auto testModel = iGame::FileIO::ReadFile(ModelFilePath);
        Check(testModel != nullptr, "Cannot read the triangle-strip test model");
        panel->setInput(testModel);
        Check(apply->isEnabled(), "Valid input did not enable Apply");
        apply->click();
        Check(resultCount == 1 && panel->lastFilter(), "Apply was not connected to the filter");
        Check(panel->lastFilter()->GetMaximumLength() == 1000, "Length was not passed to filter");
        Check(Control<QLabel>(panel, "trianglesBefore")->text() == ExpectedTriangleCount, "Wrong input triangle count");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == ExpectedTriangleCount, "Wrong output triangle count");
        Check(panel->lastFilter()->GetNumberOfStrips() == 1, "Model was not converted into one full strip");
        Check(panel->lastFilter()->GetLongestStripLength() == 8, "Full strip has the wrong length");
        auto publishedSurface = iGame::DynamicCast<iGame::SurfaceMesh>(lastSurface);
        iGame::CellArray::Pointer publishedStrips;
        iGame::CellArray::Pointer publishedSourceFaceIds;
        Check(publishedSurface != nullptr &&
                      iGame::TriangleStripFilter::ReadOutputStrips(
                              publishedSurface, publishedStrips,
                              publishedSourceFaceIds),
              "Published output did not retain triangle-strip metadata");
        Check(publishedStrips->GetNumberOfCells() == 1 &&
                      publishedStrips->GetCellSize(0) == 10 &&
                      publishedSourceFaceIds->GetCellSize(0) == 8,
              "Published triangle-strip topology is invalid");
        Check(panel->polyLineOutput() &&
                      panel->polyLineOutput()->GetNumberOfCells() == ExpectedBoundarySegmentCount,
              "Open test model has an unexpected boundary-segment count");
        Check(Control<QLabel>(panel, "polyLineCount")->text() == QStringLiteral("10 → 10"),
              "Wrong unmerged boundary statistics");
        Check(panel->input() == testModel.get(), "Apply changed the source to its output");

        length->setValue(4);
        Check(panel->apply() && resultCount == 2, "Cannot reapply modified parameters");
        Check(panel->lastFilter()->GetNumberOfStrips() == 2, "Length limit did not split the full strip in two");
        Check(panel->lastFilter()->GetLongestStripLength() == 4, "New length limit ignored");
        publishedSurface = iGame::DynamicCast<iGame::SurfaceMesh>(lastSurface);
        Check(publishedSurface != nullptr &&
                      iGame::TriangleStripFilter::ReadOutputStrips(
                              publishedSurface, publishedStrips,
                              publishedSourceFaceIds) &&
                      publishedStrips->GetNumberOfCells() == 2 &&
                      publishedSourceFaceIds->GetNumberOfCells() == 2,
              "Published output lost length-limited strips or mappings");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == ExpectedTriangleCount, "Reapply lost triangles");

        join->setChecked(true);
        apply->click();
        Check(panel->lastFilter()->GetJoinContiguousSegments(), "Merge checkbox not passed to filter");
        auto* lines = panel->polyLineOutput();
        Check(lines && lines->GetNumberOfCells() == 1, "Model boundary segments did not join");
        Check(lines->GetCellType(0) == iGame::IG_POLY_LINE &&
                      lines->GetCells()->GetCellSize(0) == ExpectedJoinedPointCount,
              "Joined output must be one closed polyline, not a polygon");
        const igIndex* ids = nullptr;
        lines->GetCells()->GetCellIds(0, ids);
        Check(ids[0] == ids[ExpectedJoinedPointCount - 1],
              "Polyline should close at its starting point");
        Check(Control<QLabel>(panel, "polyLineCount")->text() == QStringLiteral("10 → 1"),
              "Wrong joined boundary statistics");

        // Unsupported explicit lines must not be silently dropped by extraction.
        iGame::UnstructuredMesh::Pointer explicitLines = lines;
        panel->setInput(explicitLines);
        const int successes = resultCount;
        Check(!panel->apply() && resultCount == successes, "Explicit lines should fail without publishing an empty surface");
        Check(!Control<QLabel>(panel, "triangleStripStatus")->text().isEmpty(), "No input error shown");
        panel->setInput(iGame::DataObject::New());
        Check(!apply->isEnabled(), "Unsupported data enabled Apply");
        panel->setInput(nullptr);
        Check(!apply->isEnabled() && panel->lastFilter() == nullptr, "Clearing source left an active result");

        panel->setInput(testModel);
        length->setValue(1000);
        join->setChecked(false);
        Check(panel->apply(), "Final triangle-strip test model run failed");
        std::cout << "Triangle-strip Qt panel tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Triangle-strip Qt panel test failed: " << error.what() << '\n';
        return 1;
    }
}
