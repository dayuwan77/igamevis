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
void Check(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

template <class T>
T* Control(QWidget* panel, const char* name) {
    auto* result = panel->findChild<T*>(QString::fromLatin1(name));
    Check(result != nullptr, name);
    return result;
}

iGame::SurfaceMesh::Pointer OpenTriangle() {
    auto mesh = iGame::SurfaceMesh::New();
    auto points = iGame::Points::New();
    points->AddPoint(0, 0, 0);
    points->AddPoint(1, 0, 0);
    points->AddPoint(0, 1, 0);
    auto faces = iGame::CellArray::New();
    faces->AddCellId3(0, 1, 2);
    mesh->SetPoints(points);
    mesh->SetFaces(faces);
    mesh->SetName("OpenTriangle");
    return mesh;
}
}

// argv[1] optionally overrides the cylinder VTK file (default: Models/...).
// Optional argv[2] saves a panel preview for visual regression review.
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
        const char* modelFile = argc >= 2 ? argv[1] : "Models/ContourExtraction_cylinder_UnstructedGrid.vtk";
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

        auto cylinder = iGame::FileIO::ReadFile(modelFile);
        Check(cylinder != nullptr, "Cannot read the cylinder model");
        panel->setInput(cylinder);
        Check(apply->isEnabled(), "Valid input did not enable Apply");
        apply->click();
        Check(resultCount == 1 && panel->lastFilter(), "Apply was not connected to the filter");
        Check(panel->lastFilter()->GetMaximumLength() == 1000, "Length was not passed to filter");
        Check(Control<QLabel>(panel, "trianglesBefore")->text() == "3976", "Wrong input triangle count");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == "3976", "Wrong output triangle count");
        Check(panel->lastFilter()->GetNumberOfStrips() > 0, "No strips generated");
        Check(panel->polyLineOutput() == nullptr, "Closed cylinder should have no boundary lines");
        Check(panel->input() == cylinder.get(), "Apply changed the source to its output");

        length->setValue(4);
        Check(panel->apply() && resultCount == 2, "Cannot reapply modified parameters");
        Check(panel->lastFilter()->GetLongestStripLength() <= 4, "New length limit ignored");
        Check(Control<QLabel>(panel, "trianglesAfter")->text() == "3976", "Reapply lost triangles");

        auto patch = OpenTriangle();
        panel->setInput(patch);
        Check(Control<QLabel>(panel, "trianglesBefore")->text() == QStringLiteral("—"), "Input change left stale statistics");
        join->setChecked(false);
        Check(panel->apply(), "Open patch failed");
        Check(panel->polyLineOutput() && panel->polyLineOutput()->GetNumberOfCells() == 3, "Unmerged boundary should have three segments");
        join->setChecked(true);
        apply->click();
        Check(panel->lastFilter()->GetJoinContiguousSegments(), "Merge checkbox not passed to filter");
        auto* lines = panel->polyLineOutput();
        Check(lines && lines->GetNumberOfCells() == 1, "Three boundary segments did not join");
        Check(lines->GetCellType(0) == iGame::IG_POLY_LINE && lines->GetCells()->GetCellSize(0) == 4,
              "Joined output must be one closed polyline, not a polygon");
        const igIndex* ids = nullptr;
        lines->GetCells()->GetCellIds(0, ids);
        Check(ids[0] == ids[3], "Polyline should close at its starting point");
        Check(Control<QLabel>(panel, "polyLineCount")->text() == QStringLiteral("3 → 1"), "Wrong line statistics");

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

        panel->setInput(cylinder);
        length->setValue(1000);
        join->setChecked(false);
        Check(panel->apply(), "Final cylinder run failed");
        if (argc >= 3) {
            dock->resize(470, 780);
            dock->show();
            app.processEvents();
            Check(dock->grab().save(QString::fromLocal8Bit(argv[2])), "Cannot save panel preview");
        }
        std::cout << "Triangle-strip Qt panel tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Triangle-strip Qt panel test failed: " << error.what() << '\n';
        return 1;
    }
}
