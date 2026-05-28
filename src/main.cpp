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

#include "OcctImGui.h"
#include "OcctViewer.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

// ─── DemoApp ────────────────────────────────────────────────────────────────

class DemoApp : public OcctImGui::Viewer {
public:
    void init() override;
    void onImport() override;
    void onExport() override;
    void onImportFile(const char* path) override;
};

void DemoApp::init() {
    addShape(BRepPrimAPI_MakeBox(40, 40, 40).Shape(),    0.30f, 0.69f, 0.50f, "Box",      0, 0, 0);
    addShape(BRepPrimAPI_MakeSphere(30).Shape(),          0.37f, 0.81f, 0.59f, "Sphere",   80, 0, 0);
    addShape(BRepPrimAPI_MakeCylinder(20, 60).Shape(),    0.24f, 0.56f, 0.40f, "Cylinder", -80, 0, 0);

    Viewer::init();
    printf("Demo initialized: Box, Sphere, Cylinder\n");
}

void DemoApp::onImport() {
    char path[1024] = {};
    if (!OcctViewer::openFileDialog(path, sizeof(path)))
        return;
    onImportFile(path);
}

void DemoApp::onImportFile(const char* path) {
    importFile(path);
}

void DemoApp::onExport() {
    NCollection_List<TopoDS_Shape> selected;
    viewer()->getSelectedShapes(selected);
    if (selected.IsEmpty()) {
        printf("No shapes selected for export\n");
        return;
    }

    char path[1024] = {};
    if (!OcctViewer::saveFileDialog(path, sizeof(path)))
        return;

    if (selected.Size() == 1) {
        viewer()->exportShape(selected.First(), path);
    } else {
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (auto& s : selected)
            builder.Add(compound, s);
        viewer()->exportShape(compound, path);
    }
    printf("Exported: %s\n", path);
}

// ─── main ───────────────────────────────────────────────────────────────────

int main() {
    DemoApp app;
    return app.run();
}
