#include <IQCore/igQtMainWindow.h>
#include <IQWidgets/igQtConnectedSurfacePropertiesWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QEventLoop>
#include <QPushButton>
#include <QSurfaceFormat>
#include <QTimer>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckHeap(const char* stage) {
#if defined(_MSC_VER) && defined(_DEBUG)
    if (_CrtCheckMemory() == 0) {
        throw std::runtime_error(std::string("heap corruption detected: ") + stage);
    }
#else
    (void)stage;
#endif
    std::cout << "heap check passed: " << stage << std::endl;
}

QPushButton* FindExecuteButton(QWidget* panel) {
    for (auto* button : panel->findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("执行并生成输出")) return button;
    }
    return nullptr;
}

void ProcessEventsFor(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "starting Qt panel heap test" << std::endl;
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
#if defined(Q_OS_WIN)
        qputenv("QT_QPA_PLATFORM", "windows");
#else
        qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
    }
    QApplication app(argc, argv);
    std::cout << "QApplication created" << std::endl;

    try {
        iGame::Log::Init();
        std::cout << "log initialized" << std::endl;
        igQtMainWindow owner;
        owner.show();
        app.processEvents();
        CheckHeap("main window constructed");
        owner.initArgs({QStringLiteral("testConnectedSurfacePropertiesWidget"),
                        QStringLiteral("--filepath"),
                        QStringLiteral("./Models/ConnectedSurfacePropertiesTwoBoxes.vtk")});
        ProcessEventsFor(1500);
        CheckHeap("input model loaded");

        QAction* filterAction = nullptr;
        for (auto* action : owner.findChildren<QAction*>()) {
            if (action->text().contains(QStringLiteral("Connected Surface Properties"))) {
                filterAction = action;
                break;
            }
        }
        Check(filterAction != nullptr, "missing Connected Surface Properties action");
        filterAction->trigger();
        ProcessEventsFor(250);
        auto* panel = dynamic_cast<igQtConnectedSurfacePropertiesWidget*>(
                owner.findChild<QDialog*>(QStringLiteral("ConnectedSurfacePropertiesWidget")));
        Check(panel != nullptr && panel->isReady() && panel->isVisible(),
              "filter panel did not open with the selected SurfaceMesh");
        CheckHeap("filter panel opened");
        auto* execute = FindExecuteButton(panel);
        Check(execute != nullptr, "missing execute button");

        CheckHeap("before first execution");
        execute->click();
        app.processEvents();
        CheckHeap("first execution completed");
        panel->close();
        app.processEvents();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        Check(!panel->isVisible(),
              "closing must hide the reusable panel without destroying it");
        CheckHeap("filter panel closed");

        filterAction->trigger();
        ProcessEventsFor(250);
        auto* reopenedPanel = dynamic_cast<igQtConnectedSurfacePropertiesWidget*>(
                owner.findChild<QDialog*>(QStringLiteral("ConnectedSurfacePropertiesWidget")));
        Check(panel == reopenedPanel && panel->isVisible(),
              "the same panel instance must reopen for the current model");
        execute->click();
        app.processEvents();
        CheckHeap("reopened panel executed");
        panel->close();
        app.processEvents();
        CheckHeap("filter panel closed a second time");

        owner.close();
        app.processEvents();
        CheckHeap("main window closed");
        std::cout << "Connected-surface-properties Qt panel heap test passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Connected-surface-properties Qt panel test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
