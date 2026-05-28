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

#include "Application.h"
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

namespace OcctImGui {

class Viewer : public Application {
public:
    Viewer();
    ~Viewer() override;

    void addShape(const TopoDS_Shape& shape, float r, float g, float b,
                  const char* name = nullptr,
                  double tx = 0, double ty = 0, double tz = 0);
    void fitAll();
    void importFile(const char* path);
    void show();

protected:
    void init() override;
    void drawGui() override;
    void postFrame() override;

private:
    struct ShapeEntry {
        Handle(AIS_Shape) aisShape;
        std::string name;
        bool visible = true;
        float color[3] = {0.5f, 0.7f, 0.5f};
        double tx = 0, ty = 0, tz = 0;
        ShapeEntry(const Handle(AIS_Shape)& s, const std::string& n, bool v,
                   float cr, float cg, float cb,
                   double px = 0, double py = 0, double pz = 0)
            : aisShape(s), name(n), visible(v), color{cr, cg, cb},
              tx(px), ty(py), tz(pz) {}
    };

    struct PendingShape {
        TopoDS_Shape shape;
        float r, g, b;
        std::string name;
        double tx = 0, ty = 0, tz = 0;
    };

    void drawObjectPanel();
    Handle(AIS_Shape) makeColoredShape(const TopoDS_Shape& shape,
                                       float r, float g, float b,
                                       double tx = 0, double ty = 0, double tz = 0);

    std::vector<ShapeEntry> shapes_;
    std::vector<PendingShape> pendingShapes_;
    std::vector<std::function<void(OcctViewer*)>> pendingActions_;
    int nextId_ = 0;
};

} // namespace OcctImGui
