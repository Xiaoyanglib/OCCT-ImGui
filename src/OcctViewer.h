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

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopoDS_Shape.hxx>
#include <NCollection_List.hxx>
#include <Aspect_Window.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <gp_Pln.hxx>

struct GLFWwindow;

/// Low-level OCCT 3D viewer wrapper. Manages the AIS context, camera,
/// scene objects, and clip planes. Wraps GLFW window into OCCT's Aspect_Window.
class OcctViewer {
public:
    OcctViewer(GLFWwindow* glfwWindow);
    ~OcctViewer();

    /// Render the 3D scene.
    void redraw();

    /// Notify the context that the scene has changed (call after batch updates).
    void updateViewer();

    /// Handle framebuffer size change.
    void resize(int width, int height);

    // ── Camera ──────────────────────────────────────────────────────────

    void startRotation(int x, int y);
    void rotation(int x, int y);
    void zoom(double factor);
    void pan(int x, int y);

    /// Fit the view to show all displayed shapes.
    void fitAll();

    /// Get view and projection matrices for the orientation gizmo.
    void getCameraMatrices(float viewMat[16], float projMat[16]) const;

    /// Set camera from a view matrix (orientation gizmo callback).
    void setCameraFromViewMatrix(const float viewMat[16]);

    // ── Scene objects ───────────────────────────────────────────────────

    /// Add a shape to the display. Set update=true to refresh immediately.
    void displayShape(const Handle(AIS_Shape)& shape, bool update = true);

    /// Remove a shape from the display.
    void removeShape(const Handle(AIS_Shape)& shape, bool update = true);

    /// Set shaded display with visible edges.
    void setShadedWithEdges();

    // ── Import / export ─────────────────────────────────────────────────

    /// Read a CAD file and return the shape (does not add to scene).
    TopoDS_Shape importShape(const char* path);

    /// Write a shape to a CAD file.
    bool exportShape(const TopoDS_Shape& shape, const char* path);

    /// Get currently selected shapes.
    void getSelectedShapes(NCollection_List<TopoDS_Shape>& outList);

    // ── Clip planes ─────────────────────────────────────────────────────

    void addClipPlane(int id, const gp_Pln& plane);
    void removeClipPlane(int id);
    void setClipPlaneEquation(int id, const gp_Pln& plane);
    void setClipPlaneOn(int id, bool on);
    void setClipPlaneCapping(int id, bool enable, Quantity_Color color, float alpha = 0.0f);
    bool hasClipPlane(int id) const;

    /// Get the bounding box of all displayed shapes.
    void getBoundingBox(double* xmin, double* ymin, double* zmin,
                        double* xmax, double* ymax, double* zmax) const;

    // ── File dialogs ────────────────────────────────────────────────────

    static bool openFileDialog(char* outPath, int maxLen);
    static bool saveFileDialog(char* outPath, int maxLen);

    // ── Selection ───────────────────────────────────────────────────────

    void moveTo(int x, int y);
    void deselectOrClear();
    bool shiftSelect(int x, int y);
    void selectRectangle(int x1, int y1, int x2, int y2);
    void shiftSelectRectangle(int x1, int y1, int x2, int y2);
    void clearSelection();

    /// Set global selection mode (Shape/Face/Edge/Vertex).
    void setSelectionMode(int mode);

    // ── Display settings ────────────────────────────────────────────────

    void setBackgroundColor(float r, float g, float b);
    void setOrthographic(bool ortho);

    Handle(V3d_View) view() const { return view_; }
    Handle(V3d_Viewer) viewer() const { return viewer_; }
    Handle(AIS_InteractiveContext) context() const { return context_; }

private:
    GLFWwindow* glfwWindow_ = nullptr;
    Handle(Aspect_Window) nativeWin_;
    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(AIS_InteractiveContext) context_;
    NCollection_DataMap<int, Handle(Graphic3d_ClipPlane)> clipPlanes_;
};
