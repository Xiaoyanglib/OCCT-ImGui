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

#include "../../src/Application.h"
#include <AIS_ColoredShape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

namespace OcctImGui {

/// Per-face metadata. Coloring via parent AIS_ColoredShape::SetCustomColor.
struct FaceEntry {
    TopoDS_Face topoFace;          ///< Face reference for SetCustomColor
    int   id = 0;
    float color[3] = {0.7f, 0.7f, 0.7f};
    bool  useCustomColor = false;
    bool  visible = true;

    FaceEntry(const TopoDS_Face& f, int i, float r, float g, float b)
        : topoFace(f), id(i), color{r, g, b} {}
};

/// Main application class. Create one instance, call addShape / importFile,
/// then run() to start the 3D viewer and UI.
class Viewer : public Application {
public:
    /// Per-shape data, accessible for direct manipulation.
    struct ShapeEntry {
        Handle(AIS_Shape) aisShape;
        std::string name;
        bool visible = true;
        bool facesExpanded = false;
        float color[3] = {0.7f, 0.7f, 0.7f};
        double tx = 0, ty = 0, tz = 0;
        std::vector<FaceEntry> faces;
        ShapeEntry(const Handle(AIS_Shape)& s, const std::string& n, bool v,
                   float cr, float cg, float cb,
                   double px = 0, double py = 0, double pz = 0)
            : aisShape(s), name(n), visible(v), color{cr, cg, cb},
              tx(px), ty(py), tz(pz) {}
    };

    Viewer();
    ~Viewer() override;

    /// Add a CAD model to the scene. Returns the shape index for later use
    /// with setFaceColor.
    /// @param shape  TopoDS_Shape to display
    /// @param r,g,b  RGB color (0–1), used as default face color
    /// @param name   Display name (auto-generated if null)
    /// @param tx,ty,tz  Optional translation offset
    int addShape(const TopoDS_Shape& shape, float r, float g, float b,
                 const char* name = nullptr,
                 double tx = 0, double ty = 0, double tz = 0);

    /// Import a CAD file (STEP, IGES, BREP, STL) into the scene.
    void importFile(const char* path);

    /// Set a face color by shape index and face id.
    void setFaceColor(int shapeIdx, int faceId, float r, float g, float b);

protected:

protected:
    void init() override;
    void drawGui() override;
    void preFrame() override;
    void postFrame() override;
    void onImport() override;
    void onExport() override;
    void onImportFile(const char* path) override;

    /// Fit the 3D view to show all shapes.
    void fitAll();

private:
    struct PendingShape {
        TopoDS_Shape shape;
        float r, g, b;
        std::string name;
        double tx = 0, ty = 0, tz = 0;
        size_t shapeIdx = 0;
    };

    void extractFaces(ShapeEntry& entry, const TopoDS_Shape& shape,
                      float r, float g, float b,
                      double tx = 0, double ty = 0, double tz = 0);

    void drawObjectPanel();
    void drawOverlay() override;
    void updateClipPlane();
    Handle(AIS_ColoredShape) makeColoredShape(const TopoDS_Shape& shape,
                                              float r, float g, float b,
                                              double tx = 0, double ty = 0, double tz = 0);

    std::vector<ShapeEntry> shapes_;
    std::vector<PendingShape> pendingShapes_;
    std::vector<std::function<void(OcctViewer*)>> pendingActions_;
    int nextId_ = 0;

    bool sectionOn_  = false;
    bool clipEditOn_ = false;
    float clipPos_   = 0.0f;
    gp_Pnt clipCenter_;
    gp_Dir clipNormal_, clipDx_, clipDy_;
    static constexpr int kSectionPlaneId = 999;
};

} // namespace OcctImGui
