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

class OcctViewer {
public:
    OcctViewer(GLFWwindow* glfwWindow);
    ~OcctViewer();

    void redraw();
    void updateViewer();
    void resize(int width, int height);

    // Camera
    void startRotation(int x, int y);
    void rotation(int x, int y);
    void zoom(double factor);
    void pan(int x, int y);
    void fitAll();

    // Camera matrix helpers for orientation gizmo
    void getCameraMatrices(float viewMat[16], float projMat[16]) const;
    void setCameraFromViewMatrix(const float viewMat[16]);

    // Scene
    void displayShape(const Handle(AIS_Shape)& shape, bool update = true);
    void removeShape(const Handle(AIS_Shape)& shape, bool update = true);

    // Display mode
    void setShadedWithEdges();

    // Import / export
    TopoDS_Shape importShape(const char* path);
    bool exportShape(const TopoDS_Shape& shape, const char* path);
    void getSelectedShapes(NCollection_List<TopoDS_Shape>& outList);

    // Clip / section planes
    void addClipPlane(int id, const gp_Pln& plane);
    void removeClipPlane(int id);
    void setClipPlaneEquation(int id, const gp_Pln& plane);
    void setClipPlaneOn(int id, bool on);
    void setClipPlaneCapping(int id, bool enable, Quantity_Color color, float alpha = 0.0f);
    bool hasClipPlane(int id) const;
    void getBoundingBox(double* xmin, double* ymin, double* zmin,
                        double* xmax, double* ymax, double* zmax) const;

    // File dialogs
    static bool openFileDialog(char* outPath, int maxLen);
    static bool saveFileDialog(char* outPath, int maxLen);

    // Selection & hover
    void moveTo(int x, int y);
    void deselectOrClear();
    bool shiftSelect(int x, int y);
    void selectRectangle(int x1, int y1, int x2, int y2);
    void shiftSelectRectangle(int x1, int y1, int x2, int y2);
    void clearSelection();

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
