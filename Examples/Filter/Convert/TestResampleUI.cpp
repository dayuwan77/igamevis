// BUG (2026-10-14): Resample used a separate right dock and stretched diagnostic area.
// Check shared left tabs, retained controls/session, bounds toggling,
// and unchanged parameter-to-filter behavior without displaying desktop windows.
// Fix commit: first addition of this test, subject:
// fix: correct CGNS resampling and align Resample To Image UI
// Resolve SHA: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestResampleUI.cpp
#include <IQCore/igQtMainWindow.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtResampleToImageWidget.h>
#include <iGameModel.h>
#include <iGameSceneManager.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <iostream>

using namespace iGame;

struct RenderWidgetProbe : igQtRenderWidget {
    static void AttachCpuScene(igQtRenderWidget* widget, Scene::Pointer scene) {
        auto member = &RenderWidgetProbe::m_Scene;
        widget->*member = scene;
    }
};

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(iGameQtMainWindow);
    int passed = 0, failed = 0;
    auto check = [&](bool ok, const char* name) {
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n' << std::flush;
        ok ? ++passed : ++failed;
    };
    auto input = UnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(0, 0, 0);
    points->AddPoint(1, 0, 0);
    points->AddPoint(0, 1, 0);
    points->AddPoint(0, 0, 1);
    input->SetPoints(points);
    igIndex ids[] = {0, 1, 2, 3};
    input->AddCell(ids, 4, IG_TETRA);
    auto model = Model::New();
    model->SetDataObject(input);
    {
        igQtResampleToImageWidget panel;
        auto* run = panel.findChild<QPushButton*>("pushButton_Run");
        check(run && !run->isEnabled(), "execute stays disabled without input");
        check(!panel.findChild<QGroupBox*>("groupBox_Info") && !panel.findChild<QLabel*>("label_Info"),
              "estimate and diagnostic widgets are removed");
        panel.setCurrentModel(model);
        check(run->isEnabled(), "mesh binding still enables execution");
        auto* bounds = panel.findChild<QCheckBox*>("checkBox_UseInputBounds");
        auto* xmin = panel.findChild<QLineEdit*>("lineEdit_XMin");
        check(bounds->isChecked() && !xmin->isEnabled(), "input bounds and manual-field state are unchanged");
        bounds->setChecked(false);
        bool allEnabled = true;
        for (auto axis: {"X", "Y", "Z"}) {
            for (auto end: {"Min", "Max"}) {
                auto* edit = panel.findChild<QLineEdit*>(QString("lineEdit_%1%2").arg(axis, end));
                allEnabled &= edit && edit->isEnabled();
            }
        }
        check(allEnabled, "manual bounds still enable all six fields");
        bounds->setChecked(true);
        bool defaults = true;
        for (auto axis: {"X", "Y", "Z"}) {
            auto* spin = panel.findChild<QSpinBox*>(QString("spinBox_Dim%1").arg(axis));
            defaults &= spin->value() == 64 && spin->minimum() == 1 && spin->maximum() == 2048;
            spin->setValue(5);
        }
        check(defaults, "sampling defaults and limits are unchanged");
        check(panel.findChild<QCheckBox*>("checkBox_FailOnUnsupported")->isChecked() &&
                      panel.findChild<QCheckBox*>("checkBox_DisableDiscreteInterp")->isChecked(),
              "both policy choices and their defaults remain available");
        DataObject::Pointer result;
        QObject::connect(&panel, &igQtResampleToImageWidget::resultReady,
                         [&](DataObject::Pointer output) { result = output; });
        run->click();
        auto image = DynamicCast<StructuredMesh>(result);
        check(image && image->GetNumberOfPoints() == 125 && image->GetNumberOfCells() == 64,
              "execute still forwards dimensions and produces the same image topology");
        bounds->setChecked(false);
        for (auto axis: {"X", "Y", "Z"}) {
            panel.findChild<QLineEdit*>(QString("lineEdit_%1Min").arg(axis))->setText("2");
            panel.findChild<QLineEdit*>(QString("lineEdit_%1Max").arg(axis))->setText("3");
        }
        run->click();
        image = DynamicCast<StructuredMesh>(result);
        check(image && image->GetPoint(0)[0] == 2 && image->GetPoint(124)[0] == 3,
              "execute still forwards explicitly chosen sampling bounds");
        for (int width: {360, 440}) {
            panel.resize(width, 390);
            panel.show();
            app.processEvents();
            panel.layout()->activate();
            bool fits = panel.minimumSizeHint().width() <= width;
            for (auto* checkbox: panel.findChildren<QCheckBox*>()) {
                fits &= checkbox->fontMetrics().horizontalAdvance(checkbox->text()) + 28 <= checkbox->width();
            }
            check(fits, "parameter labels fit the left-panel width");
        }
    }
    {
        // Never show the main window or initialize GL; menu/tab routing needs only a CPU scene.
        igQtMainWindow window;
        auto* renderer = window.findChild<igQtModelDrawWidget*>();
        auto scene = SceneManager::Instance()->NewScene();
        RenderWidgetProbe::AttachCpuScene(renderer, scene);
        scene->AddModel(input);
        auto* action = window.findChild<QAction*>("action_filter_resample_to_image");
        auto* tabs = window.findChild<QTabWidget*>("LeftFieldTabs");
        auto* dock = window.findChild<QDockWidget*>("LeftFieldDock");
        check(action && tabs && dock, "resample menu and existing left tool stack are present");
        if (!action || !tabs || !dock) return 1;
        action->trigger();
        auto* panel = window.findChild<igQtResampleToImageWidget*>();
        check(panel && tabs->count() == 1 && tabs->currentWidget()->isAncestorOf(panel) &&
                      window.dockWidgetArea(dock) == Qt::LeftDockWidgetArea,
              "resample opens inside the shared left tool tab");
        auto* shell = window.findChild<QDockWidget*>("dockWidget_ResampleToImage");
        check(shell && shell->isHidden() && !window.findChild<QDockWidget*>("standardFilterPanel_resample_to_image"),
              "no independent right-side resample dock is shown");
        panel->findChild<QSpinBox*>("spinBox_DimX")->setValue(17);
        action->trigger();
        check(tabs->count() == 1 && window.findChildren<igQtResampleToImageWidget*>().size() == 1,
              "repeated menu activation reuses one panel");
        window.openLeftToolPanel(igQtMainWindow::LeftToolPanelId::Flow);
        check(tabs->count() == 2, "flow and resample share the existing tool stack");
        window.closeLeftToolPanel(igQtMainWindow::LeftToolPanelId::Flow);
        panel->findChild<QPushButton*>("pushButton_Close")->click();
        check(tabs->count() == 0, "close command removes the resample tab after tab indices shift");
        action->trigger();
        check(tabs->count() == 1 && panel->findChild<QSpinBox*>("spinBox_DimX")->value() == 17,
              "reopening retains the user's parameter session");
        QMetaObject::invokeMethod(tabs, "tabCloseRequested", Qt::DirectConnection, Q_ARG(int, 0));
        check(tabs->count() == 0, "tab close button uses the same close path");
    }
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
