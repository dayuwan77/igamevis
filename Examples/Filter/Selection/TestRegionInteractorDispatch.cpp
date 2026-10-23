// Regression trigger: middle drag on a region handle also moved the camera.
// Expected: press-move-release captured by a special widget does not enter the
// default scene interaction, while a middle drag away still reaches it.
// Fix: "feat: integrate region extraction with latest main". Find its SHA with
// git log --diff-filter=A --format="%h %s" -- Examples/Filter/Selection/TestRegionInteractorDispatch.cpp
#include <iGameCamera.h>
#include <iGameInteractor.h>
#include <iGameScene.h>
#include <iostream>

class TestScene final : public iGame::Scene {
public:
    bool IsInteracting() const { return m_IsInteracting; }
};

class CapturingStyle final : public iGame::InteractorStyle {
public:
    void Initialize(iGame::SmartPointer<iGame::Interactor>) override {}
    bool IsInteractionActive() const override { return captured; }
    void MousePressEvent(iGame::IEvent event) override {
        captured = (event.button == iGame::MiddleButton || event.button == iGame::LeftButton) && event.pos.x < 100.f;
    }
    void MouseMoveEvent(iGame::IEvent) override {
        if (captured) ++moves;
    }
    void MouseReleaseEvent(iGame::IEvent) override { captured = false; }
    bool captured = false;
    int moves = 0;
};

int main() {
    iGame::SmartPointer<TestScene> scene = new TestScene;
    auto interactor = scene->GetInteractor();
    interactor->Initialize(scene);
    auto camera = interactor->GetCamera();
    camera->SetViewPort(800, 600);
    camera->SetPosition(0.f, 0.f, 5.f);
    iGame::SmartPointer<CapturingStyle> widget = new CapturingStyle;
    interactor->_SetSpecialInteractor("RegionDispatchTest", widget);

    const auto before = camera->GetPosition();
    interactor->FilterEvent({iGame::IEvent::MousePress, iGame::MiddleButton, {50.f, 50.f}, 0.});
    interactor->FilterEvent({iGame::IEvent::MouseMove, iGame::MiddleButton, {80.f, 70.f}, 0.});
    interactor->FilterEvent({iGame::IEvent::MouseRelease, iGame::MiddleButton, {80.f, 70.f}, 0.});
    if (widget->moves != 1 || widget->captured || scene->IsInteracting() ||
        (camera->GetPosition() - before).length() > 1e-6f) {
        std::cerr << "Captured middle drag entered default interaction or missed the widget\n";
        return 1;
    }
    // Regression: left boundary resize must not also rotate the camera.
    // Fix: 1021f58. Verify the entire captured press-move-release sequence.
    interactor->FilterEvent({iGame::IEvent::MousePress, iGame::LeftButton, {50.f, 50.f}, 0.});
    interactor->FilterEvent({iGame::IEvent::MouseMove, iGame::LeftButton, {80.f, 70.f}, 0.});
    interactor->FilterEvent({iGame::IEvent::MouseRelease, iGame::LeftButton, {80.f, 70.f}, 0.});
    if (widget->moves != 2 || widget->captured || scene->IsInteracting() ||
        (camera->GetPosition() - before).length() > 1.e-6f) return 3;
    interactor->FilterEvent({iGame::IEvent::MousePress, iGame::MiddleButton, {300.f, 300.f}, 0.});
    interactor->FilterEvent({iGame::IEvent::MouseMove, iGame::MiddleButton, {330.f, 320.f}, 0.});
    const bool defaultReceivedMove = scene->IsInteracting();
    interactor->FilterEvent({iGame::IEvent::MouseRelease, iGame::MiddleButton, {330.f, 320.f}, 0.});
    if (!defaultReceivedMove || scene->IsInteracting()) {
        std::cerr << "Uncaptured middle drag did not reach default interaction\n";
        return 2;
    }
    std::cout << "Region middle-drag event routing passed\n";
    return 0;
}
