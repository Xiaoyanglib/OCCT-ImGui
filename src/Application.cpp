// MIT License
//
// Copyright (c) 2026 Xiaoyang Yu
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Application.h"
#include "OcctViewer.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <cstdlib>
#include <thread>

#include <string>

static Application* g_app = nullptr;

// ─── GLFW callbacks ─────────────────────────────────────────────────────────

void Application::glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

void Application::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!g_app) return;
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_F7:  g_app->showViewerPanel_ = !g_app->showViewerPanel_; break;
        case GLFW_KEY_F8:  g_app->showObjectPanel_  = !g_app->showObjectPanel_; break;
        case GLFW_KEY_F:
            g_app->viewer()->fitAll();
            break;
        }
    }
}

void Application::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_app) return;
    if (g_app->guiWantsMouse_) return;
    auto* v = g_app->viewer();
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int ix = static_cast<int>(x), iy = static_cast<int>(y);
    float pr = g_app->pixelRatio_;
    int ixFb = static_cast<int>(x * pr), iyFb = static_cast<int>(y * pr);
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (shift) {
                // Shift+left: toggle-pick on press, then always allow rect-select drag.
                v->shiftSelect(ixFb, iyFb);
                g_app->selecting_   = true;
                g_app->selectShift_ = true;
                g_app->selStartX_   = ix;
                g_app->selStartY_   = iy;
                g_app->selEndX_     = ix;
                g_app->selEndY_     = iy;
            } else {
                // Plain left → record for click-vs-drag detection.
                g_app->selecting_   = false;
                g_app->panning_     = true;
                g_app->clickStartX_ = ix;
                g_app->clickStartY_ = iy;
                g_app->lastMouseX_  = ix;
                g_app->lastMouseY_  = iy;
            }
        } else if (action == GLFW_RELEASE) {
            if (g_app->selecting_) {
                // Defer selection to postFrame — avoids blocking the render thread.
                g_app->pendingRectSelect_ = true;
                g_app->pendingRectShift_  = g_app->selectShift_;
                g_app->rectX1_ = g_app->selStartX_;
                g_app->rectY1_ = g_app->selStartY_;
                g_app->rectX2_ = g_app->selEndX_;
                g_app->rectY2_ = g_app->selEndY_;
                g_app->selecting_ = false;
            } else if (g_app->panning_) {
                // If minimal movement → it was a click, not a drag.
                int dx = ix - g_app->clickStartX_;
                int dy = iy - g_app->clickStartY_;
                if (abs(dx) < 4 && abs(dy) < 4)
                    v->deselectOrClear();
            }
            g_app->panning_ = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)
            v->startRotation(ixFb, iyFb);
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        g_app->lastMouseY_ = iy;
    }
}

void Application::glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!g_app) return;
    if (g_app->guiWantsMouse_) return;
    auto* v = g_app->viewer();
    int ix = static_cast<int>(xpos), iy = static_cast<int>(ypos);
    float pr = g_app->pixelRatio_;
    int ixFb = static_cast<int>(xpos * pr), iyFb = static_cast<int>(ypos * pr);

    // Hover highlight when no button is held
    bool anyButton =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (!anyButton)
        v->moveTo(ixFb, iyFb);

    // Shift+left-drag → track rubber-band end point
    if (g_app->selecting_ &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        g_app->selEndX_ = ix;
        g_app->selEndY_ = iy;
        return;
    }

    // Plain left-drag → pan
    if (g_app->panning_ &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        int dx = ixFb - static_cast<int>(g_app->lastMouseX_ * pr);
        int dy = static_cast<int>(g_app->lastMouseY_ * pr) - iyFb;
        if (dx != 0 || dy != 0) {
            v->pan(dx, dy);
            g_app->lastMouseX_ = ix;
            g_app->lastMouseY_ = iy;
        }
        return;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        v->rotation(ixFb, iyFb);
        return;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        int delta = static_cast<int>((g_app->lastMouseY_ - iy) * pr);
        if (delta != 0) {
            v->zoom(1.0 + delta * 0.01);
            g_app->lastMouseY_ = iy;
        }
        return;
    }
}

void Application::glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!g_app) return;
    if (g_app->guiWantsMouse_) return;
    g_app->viewer()->zoom(1.0 - yoffset / 30.0);
}

void Application::glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (!g_app) return;
    g_app->width_ = width;
    g_app->height_ = height;
    g_app->viewer()->resize(width, height);
}

void Application::glfwDropCallback(GLFWwindow*, int count, const char** paths) {
    if (!g_app) return;
    for (int i = 0; i < count; i++)
        g_app->onImportFile(paths[i]);
}


// ─── Application ────────────────────────────────────────────────────────────

Application::Application() {
    g_app = this;
}

Application::~Application() {
    g_app = nullptr;
}

int Application::run() {
    initWindow();
    initImGui();

    viewer_ = std::make_unique<OcctViewer>(window_);
    viewer_->resize(width_, height_);
    glfwMakeContextCurrent(window_);

    init();

    mainLoop();

    shutdown();

    viewer_.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();

    return 0;
}

void Application::initWindow() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    window_ = glfwCreateWindow(width_, height_, "OCCT + ImGui", nullptr, nullptr);
    if (!window_) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window_, glfwKeyCallback);
    glfwSetMouseButtonCallback(window_, glfwMouseButtonCallback);
    glfwSetCursorPosCallback(window_, glfwCursorPosCallback);
    glfwSetScrollCallback(window_, glfwScrollCallback);
    glfwSetFramebufferSizeCallback(window_, glfwFramebufferSizeCallback);
    glfwSetDropCallback(window_, glfwDropCallback);
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    io.IniFilename = nullptr;

    // HiDPI: load font at native framebuffer resolution for sharp rendering
    int fbW, fbH, wndW, wndH;
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    glfwGetWindowSize(window_, &wndW, &wndH);
    pixelRatio_ = (float)fbW / (float)wndW;

    // Font + UI scaling.
    float xscale, yscale;
    glfwGetWindowContentScale(window_, &xscale, &yscale);
    float hidpi = 0.5f * (xscale + yscale);
    float fontSize = 18.0f;
    float loadedFontSize = fontSize * hidpi / pixelRatio_;

    std::string fontPath = std::string(FONTS_DIR) + "/Roboto-SemiBold.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), loadedFontSize);

    // UI scaling = font_size / 16 * hidpi / pixel_ratio
    fontScale_ = fontSize / 16.0f * hidpi / pixelRatio_;

    applyStyle();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");
}

void Application::applyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsLight();

    // ── Sizing & rounding (scaled by content scale for HiDPI) ──
    float s = fontScale_;
    style.WindowRounding     = 3.0f;
    style.FrameRounding      = 2.0f;
    style.GrabRounding       = 2.0f;
    style.ScrollbarRounding  = 2.0f;
    style.ChildRounding      = 3.0f;
    style.PopupRounding      = 2.0f;
    style.ScrollbarSize      = 14.0f * s;
    style.FramePadding       = ImVec2(6 * s, 4 * s);
    style.ItemSpacing        = ImVec2(8 * s, 5 * s);
    style.ItemInnerSpacing   = ImVec2(6 * s, 4 * s);
    style.IndentSpacing      = 18.0f * s;
    style.WindowBorderSize   = 0.0f;
    style.FrameBorderSize    = 0.0f;
    style.PopupBorderSize    = 1.0f;
    style.TabBorderSize      = 0.0f;
    style.Alpha              = 1.0f;
    style.WindowMenuButtonPosition = ImGuiDir_None;

    // ── Color refinement ──
    auto set = [](ImGuiCol_ idx, float r, float g, float b, float a = 1.0f) {
        ImGui::GetStyle().Colors[idx] = ImVec4(r, g, b, a);
    };

    set(ImGuiCol_WindowBg,           0.94f, 0.94f, 0.94f, 0.94f);
    set(ImGuiCol_ChildBg,            0.94f, 0.94f, 0.94f, 0.00f);
    set(ImGuiCol_PopupBg,            0.97f, 0.97f, 0.97f, 0.94f);

    set(ImGuiCol_Text,               0.00f, 0.00f, 0.00f, 1.00f);
    set(ImGuiCol_TextDisabled,       0.50f, 0.50f, 0.50f, 1.00f);

    set(ImGuiCol_Border,             0.70f, 0.70f, 0.70f, 0.50f);
    set(ImGuiCol_BorderShadow,       0.00f, 0.00f, 0.00f, 0.00f);

    set(ImGuiCol_TitleBg,            0.90f, 0.90f, 0.90f, 1.00f);
    set(ImGuiCol_TitleBgActive,      0.85f, 0.85f, 0.85f, 1.00f);
    set(ImGuiCol_TitleBgCollapsed,   0.92f, 0.92f, 0.92f, 1.00f);

    set(ImGuiCol_Header,             0.82f, 0.82f, 0.82f, 0.80f);
    set(ImGuiCol_HeaderHovered,      0.76f, 0.76f, 0.76f, 0.80f);
    set(ImGuiCol_HeaderActive,       0.24f, 0.56f, 0.42f, 0.40f);

    set(ImGuiCol_Separator,          0.65f, 0.65f, 0.65f, 0.60f);
    set(ImGuiCol_SeparatorHovered,   0.24f, 0.56f, 0.42f, 0.60f);
    set(ImGuiCol_SeparatorActive,    0.24f, 0.56f, 0.42f, 0.80f);

    set(ImGuiCol_ResizeGrip,         0.70f, 0.70f, 0.70f, 0.25f);
    set(ImGuiCol_ResizeGripHovered,  0.24f, 0.56f, 0.42f, 0.50f);
    set(ImGuiCol_ResizeGripActive,   0.24f, 0.56f, 0.42f, 0.70f);

    set(ImGuiCol_ScrollbarBg,        0.95f, 0.95f, 0.95f, 0.00f);
    set(ImGuiCol_ScrollbarGrab,      0.68f, 0.68f, 0.68f, 0.50f);
    set(ImGuiCol_ScrollbarGrabHovered,0.55f, 0.55f, 0.55f, 0.60f);
    set(ImGuiCol_ScrollbarGrabActive, 0.24f, 0.56f, 0.42f, 0.60f);

    set(ImGuiCol_FrameBg,            0.88f, 0.88f, 0.88f, 0.60f);
    set(ImGuiCol_FrameBgHovered,     0.82f, 0.82f, 0.82f, 0.70f);
    set(ImGuiCol_FrameBgActive,      0.78f, 0.78f, 0.78f, 0.80f);

    set(ImGuiCol_Button,             0.88f, 0.88f, 0.88f, 0.80f);
    set(ImGuiCol_ButtonHovered,      0.82f, 0.82f, 0.82f, 0.90f);
    set(ImGuiCol_ButtonActive,       0.24f, 0.56f, 0.42f, 0.50f);

    set(ImGuiCol_CheckMark,          0.24f, 0.56f, 0.42f, 1.00f);

    set(ImGuiCol_SliderGrab,         0.24f, 0.56f, 0.42f, 0.50f);
    set(ImGuiCol_SliderGrabActive,   0.24f, 0.56f, 0.42f, 0.80f);

    set(ImGuiCol_Tab,                0.82f, 0.86f, 0.84f, 0.86f);
    set(ImGuiCol_TabHovered,         0.72f, 0.78f, 0.76f, 0.90f);
    set(ImGuiCol_TabSelected,        0.60f, 0.78f, 0.70f, 1.00f);

    set(ImGuiCol_DockingPreview,     0.24f, 0.56f, 0.42f, 0.40f);
    set(ImGuiCol_DockingEmptyBg,     0.90f, 0.90f, 0.90f, 1.00f);

    set(ImGuiCol_TableHeaderBg,      0.86f, 0.86f, 0.86f, 1.00f);
    set(ImGuiCol_TableBorderStrong,  0.70f, 0.70f, 0.70f, 0.50f);
    set(ImGuiCol_TableBorderLight,   0.80f, 0.80f, 0.80f, 0.30f);
    set(ImGuiCol_TableRowBg,         0.94f, 0.94f, 0.94f, 0.00f);
    set(ImGuiCol_TableRowBgAlt,      0.88f, 0.88f, 0.88f, 0.06f);

    set(ImGuiCol_PlotLines,          0.24f, 0.56f, 0.42f, 1.00f);
    set(ImGuiCol_PlotHistogram,      0.24f, 0.56f, 0.42f, 0.80f);
}

void Application::drawRubberBand() {
    if (!selecting_) return;

    ImVec2 p1((float)selStartX_, (float)selStartY_);
    ImVec2 p2((float)selEndX_,   (float)selEndY_);

    ImU32 col = IM_COL32(0x1A, 0xC1, 0xFF, 0x60);
    ImU32 border = IM_COL32(0x1A, 0xC1, 0xFF, 0xC0);

    ImGui::GetForegroundDrawList()->AddRectFilled(p1, p2, col);
    ImGui::GetForegroundDrawList()->AddRect(p1, p2, border, 0.0f, 1.5f);
}

void Application::drawStatusBar() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float barHeight = 22.0f * fontScale_;
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - barHeight));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, barHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("##statusbar", nullptr, flags);
    ImGui::PopStyleVar(2);

    float fps = ImGui::GetIO().Framerate;
    ImGui::Text("FPS: %.0f", fps);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200);
    ImGui::TextDisabled("F7:Viewer  F8:Objects  F:FitAll");

    ImGui::End();
}

void Application::setupDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockBottom, dockCenter;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockCenter);

    ImGuiID dockObjects, dock3D;
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, 0.18f, &dockObjects, &dock3D);

    ImGui::DockBuilderDockWindow("Objects",         dockObjects);
    ImGui::DockBuilderDockWindow("Viewer",          dockBottom);

    ImGui::DockBuilderFinish(dockspaceId);
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        glfwMakeContextCurrent(window_);

        beginFrame();

        drawDockSpace();
        drawGui();
        drawStatusBar();
        drawRubberBand();

        if (firstFrame_) {
            firstFrame_ = false;
            int fw, fh;
            glfwGetFramebufferSize(window_, &fw, &fh);
            viewer_->resize(fw, fh);
            viewer_->fitAll();
        }

        preFrame();
        viewer_->redraw();
        drawOverlay();
        glfwMakeContextCurrent(window_);

        endFrame();

        guiWantsMouse_ = ImGui::GetIO().WantCaptureMouse;

        glfwSwapBuffers(window_);  // swap BEFORE postFrame — don't block display
        glfwMakeContextCurrent(window_);
        postFrame();
    }
}

void Application::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

void Application::drawDockSpace() {
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    static bool open = true;
    ImGui::Begin(
        "DockSpace", &open,
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar
    );
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import...",  "Cmd+I")) onImport();
            if (ImGui::MenuItem("Export...",  "Cmd+E")) onExport();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Cmd+Q"))
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Viewer",    "F7", &showViewerPanel_);
            ImGui::MenuItem("Objects",   "F8", &showObjectPanel_);
            ImGui::Separator();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_None |
        ImGuiDockNodeFlags_PassthruCentralNode |
        ImGuiDockNodeFlags_AutoHideTabBar);

    if (!docksInitialized_) {
        setupDockLayout(dockspaceId);
        docksInitialized_ = true;
    }

    ImGui::End();
}

void Application::init() {}
void Application::drawGui() {}

void Application::drawViewerPanel() {
    ImGui::Begin("Viewer", &showViewerPanel_);

    ImGui::Text("Display");
    ImGui::Separator();
    ImGui::BulletText("Background: White");
    ImGui::BulletText("Projection: Perspective");

    ImGui::Spacing();
    ImGui::Text("Mouse Controls");
    ImGui::Separator();
    ImGui::BulletText("Left drag:   Pan");
    ImGui::BulletText("Right drag:  Rotate");
    ImGui::BulletText("Middle drag: Zoom");
    ImGui::BulletText("Scroll:      Zoom");

    ImGui::Spacing();
    ImGui::Text("Keyboard");
    ImGui::Separator();
    ImGui::BulletText("F:     Fit All");
    ImGui::BulletText("F7:    Toggle Viewer");
    ImGui::BulletText("F8:    Toggle Objects");

    ImGui::End();
}
void Application::preFrame() {}
void Application::onImport() {}
void Application::onExport() {}
void Application::onImportFile(const char*) {}
void Application::postFrame() {
    if (pendingRectSelect_) {
        float pr = pixelRatio_;
        if (pendingRectShift_)
            viewer_->shiftSelectRectangle(rectX1_ * pr, rectY1_ * pr,
                                          rectX2_ * pr, rectY2_ * pr);
        else
            viewer_->selectRectangle(rectX1_ * pr, rectY1_ * pr,
                                     rectX2_ * pr, rectY2_ * pr);
        pendingRectSelect_ = false;
    }
}
void Application::drawOverlay() {}
void Application::shutdown() {}
