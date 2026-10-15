// CGNS used to be disabled, and NGON/NFACE cells could not be probed.
// Fix commit: first addition of this test, subject:
// fix: correct CGNS resampling and align Resample To Image UI
// Resolve SHA: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestCGNSResample.cpp
// Command-line export for independent ParaView comparison, without creating GL resources.
#include <Convert/iGameResampleToImageFilter.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iGameFileIO.h>
#include <iGameStructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <iGameScene.h>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <iostream>
#include <cmath>

using namespace iGame;

struct DrawArrayProbe : DrawObject {
    static IGsize Indices(DrawObject* object) {
        auto member = &DrawArrayProbe::m_PointIndices;
        return (object->*member)->GetNumberOfValues();
    }
    static IGsize DisplayedPoints(DrawObject* object) {
        object->ConvertToDrawableData();
        auto positions = object->GetRenderPoints();
        return Indices(object) ? Indices(object) : positions->GetNumberOfElements();
    }
};

// A hidden real context verifies the production Scene/Painter path, not just CPU arrays.
bool CheckRendering(StructuredMesh::Pointer output, const std::filesystem::path& dir) {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto* window = glfwCreateWindow(640, 480, "Resample regression", nullptr, nullptr);
    if (!window) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(window);
    bool ok = false;
    {
        auto scene = Scene::New();
        if (scene->Initialize()) {
            scene->Resize(640, 480, 1);
            scene->EnableFramePacing(false);
            scene->SetAxesVisible(false);
            scene->SetCenterAxesVisible(false);
            scene->SetColorBarVisible(false);
            scene->SetCornerAnnotationVisible(false);
            scene->SetBackGround(51, 51, 51);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            scene->Draw();
            std::vector<unsigned char> background(640 * 480 * 4), rendered(background.size());
            glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, background.data());
            scene->AddModel(output);
            const auto& first = output->GetPoint(0);
            const auto& last = output->GetPoint(output->GetNumberOfPoints() - 1);
            igm::vec3 center{(first[0] + last[0]) * 0.5f, (first[1] + last[1]) * 0.5f,
                             (first[2] + last[2]) * 0.5f};
            auto camera = scene->GetCamera();
            camera->SetType(Camera::Type::PERSPECTIVE);
            camera->SetFocal(center);
            camera->SetPosition(center + igm::vec3{4, -7, 34});
            camera->SetUp(0, 1, 0);
            camera->SetFov(30);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            scene->Draw();
            glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, rendered.data());
            std::ofstream solidFrame(dir / "ig-solid.rgba", std::ios::binary);
            solidFrame.write(reinterpret_cast<const char*>(rendered.data()), rendered.size());
            size_t changed = 0;
            for (size_t i = 0; i < rendered.size(); i += 4) {
                if (rendered[i] != background[i] || rendered[i + 1] != background[i + 1] ||
                    rendered[i + 2] != background[i + 2]) ++changed;
            }
            auto error = glGetError();
            std::cout << "OpenGL: " << glGetString(GL_VERSION) << "; changed pixels: " << changed
                      << "; GL error: " << error << '\n';
            ok = changed > 100 && error == GL_NO_ERROR;
            const int pressureIndex = output->GetAttributeSet()->GetAttributeIndex("Pressure");
            if (pressureIndex < 0) {
                ok = false;
            } else {
                output->ViewCloudPicture(scene, pressureIndex, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                scene->Draw();
                glReadPixels(0, 0, 640, 480, GL_RGBA, GL_UNSIGNED_BYTE, background.data());
                size_t colored = 0;
                for (size_t i = 0; i < rendered.size(); i += 4) {
                    if (rendered[i] != background[i] || rendered[i + 1] != background[i + 1] ||
                        rendered[i + 2] != background[i + 2]) ++colored;
                }
                error = glGetError();
                std::cout << "Pressure color changed pixels: " << colored << "; GL error: " << error << '\n';
                ok = ok && colored > 100 && error == GL_NO_ERROR;
            }
            output->ReleaseGpuResourcesKeepCpuData();
        }
        // All scene-owned GL objects are destroyed while the context is current.
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return ok;
}

int main(int argc, char** argv) {
    if (argc != 3 && !(argc == 4 && std::string(argv[3]) == "--render-check")) {
        std::cerr << "usage: testCGNSResample model output-directory [--render-check]\n";
        return 2;
    }
    auto start = std::chrono::steady_clock::now();
    DrawObject::DeferDrawableConversionScope defer;
    auto input = FileIO::ReadFile(argv[1]);
    if (!input) {
        std::cerr << "read failed\n";
        return 1;
    }
    auto points = DynamicCast<PointSet>(input);
    auto volume = DynamicCast<VolumeMesh>(input);
    std::cout << "Loaded: " << (points ? points->GetNumberOfPoints() : 0) << " points, "
              << (volume ? volume->GetNumberOfVolumes() : 0) << " volumes\n"
              << std::flush;
    auto filter = ResampleToImageFilter::New();
    filter->SetInput(input);
    filter->SetSamplingDimensions(100, 100, 100);
    if (!filter->Execute()) {
        std::cerr << filter->GetMessage() << '\n';
        return 1;
    }
    auto output = DynamicCast<StructuredMesh>(filter->GetOutput());
    std::filesystem::path dir(argv[2]);
    std::filesystem::create_directories(dir);
    std::ofstream manifest(dir / "ig-arrays.json");
    manifest << '[';
    auto attrs = output->GetAttributeSet()->GetAllAttributes();
    for (IGsize a = 0; a < attrs->GetNumberOfElements(); ++a) {
        auto& attr = attrs->GetElement(a);
        if (attr.isDeleted || !attr.pointer) continue;
        const auto name = attr.pointer->GetName();
        const auto filename = std::to_string(a) + ".bin";
        std::ofstream file(dir / filename, std::ios::binary);
        for (IGsize i = 0; i < attr.pointer->GetNumberOfElements(); ++i) {
            for (int c = 0; c < attr.pointer->GetDimension(); ++c) {
                double value = attr.pointer->GetElementValue(i, c);
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
        if (a) manifest << ',';
        manifest << "{\"name\":" << std::quoted(name) << ",\"file\":" << std::quoted(filename)
                 << ",\"cell\":" << (attr.attachmentType == IG_CELL ? "true" : "false")
                 << ",\"components\":" << attr.pointer->GetDimension() << '}';
    }
    manifest << ']';
    output->SetViewStyle(IG_POINTS);
    output->ConvertToDrawableData();
    auto drawable = output->GetRenderableObject();
    std::cout << "Displayed surface points: " << DrawArrayProbe::DisplayedPoints(drawable.get()) << '\n';
    auto positions = drawable->GetRenderPoints();
    const auto& first = output->GetPoint(0);
    const auto& last = output->GetPoint(output->GetNumberOfPoints() - 1);
    auto dims = output->GetDimensionSize();
    std::ofstream renderedIds(dir / "ig-render-point-ids.bin", std::ios::binary);
    for (IGsize p = 0; p < positions->GetNumberOfElements(); ++p) {
        int64_t ijk[3];
        for (int a = 0; a < 3; ++a) {
            const double length = static_cast<double>(last[a]) - first[a];
            ijk[a] = length == 0 ? 0 : std::llround((positions->GetElementValue(p, a) - first[a]) * (dims[a] - 1) / length);
        }
        int64_t id = ijk[0] + dims[0] * (ijk[1] + dims[1] * ijk[2]);
        renderedIds.write(reinterpret_cast<const char*>(&id), sizeof(id));
    }
    renderedIds.close();
    if (argc == 4 && !CheckRendering(output, dir)) {
        std::cerr << "hidden OpenGL rendering check failed\n";
        return 1;
    }
    std::cout << filter->GetMessage()
              << "\nElapsed: " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
              << " seconds\n";
    return 0;
}
