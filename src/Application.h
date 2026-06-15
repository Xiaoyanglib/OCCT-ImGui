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

#pragma once

#include <memory>

#include <AIS_Shape.hxx>
#include <GLFW/glfw3.h>

#include "imgui.h"

class OcctViewer;

/// Base application class. Manages the GLFW window, ImGui context, OCCT viewer,
/// docking layout, and main event loop. Subclass to customize panels and behavior.
class Application {
public:
    Application();
    virtual ~Application();

    /// Start the application: create window, init ImGui/OCCT, enter main loop.
    int run();

protected:
    /// Called once after viewer is created. Override for custom initialization.
    virtual void init();

    /// Called every frame to draw ImGui panels. Override for custom UI.
    virtual void drawGui();

    /// Draw the Viewer info panel (controls reference).
    virtual void drawViewerPanel();

    /// Draw the status bar at the bottom of the window.
    virtual void drawStatusBar();

    /// Draw the rectangle selection rubber band overlay.
    virtual void drawRubberBand();

    /// Draw OpenGL overlays after OCCT rendering (clip plane indicator, etc).
    virtual void drawOverlay();

    /// Called when File > Import is triggered.
    virtual void onImport();

    /// Called when File > Export is triggered.
    virtual void onExport();

    /// Called when a file is imported (from drag-drop or dialog).
    virtual void onImportFile(const char* path);

    /// Called before OCCT rendering each frame.
    virtual void preFrame();

    /// Called after rendering and buffer swap each frame.
    virtual void postFrame();

    /// Called during application shutdown.
    virtual void shutdown();

    GLFWwindow* window() const { return window_; }
    OcctViewer*  viewer() const { return viewer_.get(); }
    int  width()  const { return width_; }
    int  height() const { return height_; }
    float pixelRatio() const { return pixelRatio_; }

    bool showViewerPanel_ = true;
    bool showObjectPanel_ = true;
    int  selectionMode_   = AIS_Shape::SelectionMode(TopAbs_SHAPE);
    char statusMsg_[256]  = {};
    float bgColor_[3]     = {1.0f, 1.0f, 1.0f};
    bool  orthographic_   = true;
    bool guiWantsMouse_   = false;
    bool firstFrame_      = true;
    bool panning_         = false;
    bool selecting_       = false;
    bool selectShift_     = false;
    int  selStartX_ = 0, selStartY_ = 0;
    int  selEndX_   = 0, selEndY_   = 0;
    int  clickStartX_ = 0, clickStartY_ = 0;
    int  lastMouseX_ = 0;
    int  lastMouseY_ = 0;

    bool pendingRectSelect_ = false;
    bool pendingRectShift_  = false;
    int  rectX1_ = 0, rectY1_ = 0, rectX2_ = 0, rectY2_ = 0;

private:
    void initWindow();
    void initImGui();
    void applyStyle();
    void mainLoop();
    void beginFrame();
    void endFrame();
    void drawDockSpace();
    void setupDockLayout(ImGuiID dockspaceId);

    static void glfwErrorCallback(int error, const char* description);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwDropCallback(GLFWwindow* window, int count, const char** paths);

    GLFWwindow* window_ = nullptr;
    std::unique_ptr<OcctViewer> viewer_;
    int width_  = 1400;
    int height_ = 900;
    float fontScale_   = 1.0f;
    float pixelRatio_  = 1.0f;
    bool  docksInitialized_ = false;
};
