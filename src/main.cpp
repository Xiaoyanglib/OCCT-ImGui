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

/// Demo application: creates a viewer with three sample shapes and starts the event loop.

#include "OcctImGui.h"   // Viewer class with full UI + 3D viewer framework

// OCCT primitive shape builders for the demo scene
#include <BRepPrimAPI_MakeBox.hxx>       // Box shape
#include <BRepPrimAPI_MakeSphere.hxx>    // Sphere shape
#include <BRepPrimAPI_MakeCylinder.hxx>  // Cylinder shape

int main() {
    // Create the application. This sets up GLFW, ImGui, and the OCCT renderer.
    OcctImGui::Viewer app;

    // ── Box (40×40×40) at origin ─────────────────────────────────────
    // Create a cube. BRepPrimAPI_MakeBox builds a box with 6 faces.
    // addShape returns an index for later face-color customization.
    int boxIdx = app.addShape(
        BRepPrimAPI_MakeBox(40, 40, 40).Shape(),   // OCCT TopoDS_Shape
        0.7f, 0.7f, 0.7f,                          // default metallic gray
        "Box",                                       // name in Objects panel
        0, 0, 0);                                    // position (no offset)

    // Pre-defined palette: 6 distinct colors for the 6 faces
    // Box face order: +X, -X, +Y, -Y, +Z, -Z (per BRepPrimAPI_MakeBox convention)
    static float colors[6][3] = {
        {0.8f, 0.2f, 0.2f},  // +X face → red
        {0.2f, 0.8f, 0.2f},  // -X face → green
        {0.2f, 0.2f, 0.8f},  // +Y face → blue
        {0.8f, 0.8f, 0.2f},  // -Y face → yellow
        {0.8f, 0.4f, 0.8f},  // +Z face → magenta
        {0.2f, 0.8f, 0.8f},  // -Z face → cyan
    };
    // Apply each color to the corresponding face
    // setFaceColor(shapeIndex, faceId, red, green, blue)
    for (int i = 0; i < 6; ++i)
        app.setFaceColor(boxIdx, i, colors[i][0], colors[i][1], colors[i][2]);

    // Add a sphere of radius 30, offset to the right
    app.addShape(
        BRepPrimAPI_MakeSphere(30).Shape(),
        0.7f, 0.7f, 0.7f,
        "Sphere",
        80, 0, 0);   // shifted +80 on X

    // Add a cylinder of radius 20 and height 60, offset to the left
    int cylIdx = app.addShape(
        BRepPrimAPI_MakeCylinder(20, 60).Shape(),
        0.7f, 0.7f, 0.7f,
        "Cylinder",
        -80, 0, 0);  // shifted -80 on X

    // Color cylinder faces (0=side, 1=top, 2=bottom)
    app.setFaceColor(cylIdx, 1, 0.8f, 0.2f, 0.2f);   // top → red
    app.setFaceColor(cylIdx, 2, 0.2f, 0.2f, 0.8f);   // bottom → blue

    // Start the main loop. Blocks until the user closes the window.
    return app.run();
}
