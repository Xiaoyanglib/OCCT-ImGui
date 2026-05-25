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
#include "imgui.h"

#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Quantity_Color.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include <vector>
#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

struct ShapeEntry {
    Handle(AIS_Shape) aisShape;
    char name[64];
    bool visible = true;
    float color[3] = {0.5f, 0.7f, 0.5f};
    float tx = 0, ty = 0, tz = 0;
};

static std::vector<ShapeEntry> g_shapes;
static std::vector<std::function<void(OcctViewer*)>> g_pendingActions;

static Handle(AIS_Shape) makeColoredShape(const TopoDS_Shape& shape,
                                          float r, float g, float b,
                                          double tx = 0, double ty = 0, double tz = 0) {
    Handle(AIS_Shape) ais = new AIS_Shape(shape);
    Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
    aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
    ais->Attributes()->SetShadingAspect(aspect);

    if (tx != 0 || ty != 0 || tz != 0) {
        gp_Trsf trsf;
        trsf.SetTranslation(gp_Vec(tx, ty, tz));
        ais->SetLocalTransformation(trsf);
    }
    return ais;
}

// ─── DemoApp ────────────────────────────────────────────────────────────────

class DemoApp : public Application {
public:
    void init() override;
    void drawGui() override;
    void onImport() override;
    void onExport() override;
    void onImportFile(const char* path) override;
    void postFrame() override;

private:
    void drawObjectPanel();
    void drawViewerPanel();
    void addImportedShape(const TopoDS_Shape& shape, const char* name);

    int nextId_ = 4;
};

void DemoApp::init() {
    OcctViewer* v = viewer();

    v->setShadedWithEdges();

    TopoDS_Shape box = BRepPrimAPI_MakeBox(40, 40, 40).Shape();
    Handle(AIS_Shape) aisBox = makeColoredShape(box, 0.30f, 0.69f, 0.50f);
    v->displayShape(aisBox, false);
    g_shapes.push_back({aisBox, "Box", true, {0.30f, 0.69f, 0.50f}, 0, 0, 0});

    TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(30).Shape();
    Handle(AIS_Shape) aisSphere = makeColoredShape(sphere, 0.37f, 0.81f, 0.59f, 80, 0, 0);
    v->displayShape(aisSphere, false);
    g_shapes.push_back({aisSphere, "Sphere", true, {0.37f, 0.81f, 0.59f}, 80, 0, 0});

    TopoDS_Shape cyl = BRepPrimAPI_MakeCylinder(20, 60).Shape();
    Handle(AIS_Shape) aisCyl = makeColoredShape(cyl, 0.24f, 0.56f, 0.40f, -80, 0, 0);
    v->displayShape(aisCyl, true);
    g_shapes.push_back({aisCyl, "Cylinder", true, {0.24f, 0.56f, 0.40f}, -80, 0, 0});

    printf("Demo initialized: Box, Sphere, Cylinder\n");
}

// ─── Object Panel ───────────────────────────────────────────────────────────

void DemoApp::drawObjectPanel() {
    ImGui::Begin("Objects", &showObjectPanel_);

    // ── Scene Objects ──
    ImGui::Text("Scene Objects");
    ImGui::Separator();

    if (g_shapes.empty()) {
        ImGui::TextDisabled("No objects in scene");
    }

    for (int i = 0; i < (int)g_shapes.size(); ++i) {
        auto& entry = g_shapes[i];
        ImGui::PushID(i);

        if (ImGui::Checkbox("##vis", &entry.visible)) {
            auto shape = entry.aisShape;
            bool vis = entry.visible;
            g_pendingActions.push_back([shape, vis](OcctViewer* viewer) {
                if (vis)
                    viewer->displayShape(shape, false);
                else
                    viewer->removeShape(shape, false);
            });
        }
        ImGui::SameLine();
        if (ImGui::ColorEdit3("##color", entry.color,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            auto shape = entry.aisShape;
            float r = entry.color[0], g = entry.color[1], b = entry.color[2];
            g_pendingActions.push_back([shape, r, g, b](OcctViewer* viewer) {
                Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
                aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
                shape->Attributes()->SetShadingAspect(aspect);
                shape->Redisplay(false);
            });
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(entry.name);

        float btnSize = ImGui::GetFrameHeight();
        ImGui::SameLine(ImGui::GetWindowWidth() - btnSize - 6);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        bool del = ImGui::Button("X", ImVec2(btnSize, btnSize));
        ImGui::PopStyleColor(4);
        if (del)
            ImGui::OpenPopup("Delete Shape?");
        if (ImGui::BeginPopupModal("Delete Shape?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Remove '%s'?", entry.name);
            if (ImGui::Button("Yes", ImVec2(80, 0))) {
                auto shape = entry.aisShape;
                g_pendingActions.push_back([shape](OcctViewer* viewer) {
                    viewer->removeShape(shape, true);
                });
                entry.aisShape.Nullify();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── Actions ──
    ImGui::Text("Actions");
    ImGui::Separator();

    if (ImGui::Button("Fit All", ImVec2(-1, 0))) {
        g_pendingActions.push_back([](OcctViewer* v) { v->fitAll(); });
    }

    if (ImGui::Button("Show All", ImVec2(-1, 0))) {
        for (auto& entry : g_shapes) {
            if (entry.visible) continue;
            entry.visible = true;
            auto shape = entry.aisShape;
            float r = entry.color[0], g = entry.color[1], b = entry.color[2];
            g_pendingActions.push_back([shape, r, g, b](OcctViewer* viewer) {
                viewer->displayShape(shape, false);
                Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
                aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
                shape->Attributes()->SetShadingAspect(aspect);
                shape->Redisplay(false);
            });
        }
    }

    if (ImGui::Button("Hide All", ImVec2(-1, 0))) {
        for (auto& entry : g_shapes) {
            if (!entry.visible) continue;
            entry.visible = false;
            auto shape = entry.aisShape;
            g_pendingActions.push_back([shape](OcctViewer* viewer) {
                viewer->removeShape(shape, false);
            });
        }
    }

    // Delete All with confirmation popup
    if (ImGui::Button("Delete All", ImVec2(-1, 0)))
        ImGui::OpenPopup("Delete All?");
    if (ImGui::BeginPopupModal("Delete All?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Remove all %d shapes?", (int)g_shapes.size());
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            for (auto& entry : g_shapes) {
                auto shape = entry.aisShape;
                g_pendingActions.push_back([shape](OcctViewer* viewer) {
                    viewer->removeShape(shape, true);
                });
            }
            g_shapes.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Spacing();

    // ── Add Primitive ──
    ImGui::Text("Add Primitive");
    ImGui::Separator();

    auto addBox = [this]() {
        OcctViewer* v = viewer();
        float x = (nextId_ - 4) * 120.0f;
        TopoDS_Shape box = BRepPrimAPI_MakeBox(40, 40, 40).Shape();
        float c[3] = {0.30f, 0.60f, 0.50f};
        Handle(AIS_Shape) ais = makeColoredShape(box, c[0], c[1], c[2], x, 0, 0);
        v->displayShape(ais, true);
        char name[32];
        snprintf(name, sizeof(name), "Box %d", nextId_++);
        g_shapes.push_back({ais, "", true, {c[0], c[1], c[2]}, (float)x, 0, 0});
        strncpy(g_shapes.back().name, name, sizeof(g_shapes.back().name) - 1);
    };

    auto addSphere = [this]() {
        OcctViewer* v = viewer();
        float x = (nextId_ - 4) * 120.0f;
        TopoDS_Shape s = BRepPrimAPI_MakeSphere(25).Shape();
        float c[3] = {0.40f, 0.80f, 0.60f};
        Handle(AIS_Shape) ais = makeColoredShape(s, c[0], c[1], c[2], x, 0, 0);
        v->displayShape(ais, true);
        char name[32];
        snprintf(name, sizeof(name), "Sphere %d", nextId_++);
        g_shapes.push_back({ais, "", true, {c[0], c[1], c[2]}, (float)x, 0, 0});
        strncpy(g_shapes.back().name, name, sizeof(g_shapes.back().name) - 1);
    };

    auto addCylinder = [this]() {
        OcctViewer* v = viewer();
        float x = (nextId_ - 4) * 120.0f;
        TopoDS_Shape c = BRepPrimAPI_MakeCylinder(18, 50).Shape();
        float col[3] = {0.27f, 0.50f, 0.68f};
        Handle(AIS_Shape) ais = makeColoredShape(c, col[0], col[1], col[2], x, 0, 0);
        v->displayShape(ais, true);
        char name[32];
        snprintf(name, sizeof(name), "Cylinder %d", nextId_++);
        g_shapes.push_back({ais, "", true, {col[0], col[1], col[2]}, (float)x, 0, 0});
        strncpy(g_shapes.back().name, name, sizeof(g_shapes.back().name) - 1);
    };

    if (ImGui::Button("+ Box",      ImVec2(-1, 0))) addBox();
    if (ImGui::Button("+ Sphere",   ImVec2(-1, 0))) addSphere();
    if (ImGui::Button("+ Cylinder", ImVec2(-1, 0))) addCylinder();

    ImGui::End();
}

// ─── Viewer Panel ───────────────────────────────────────────────────────────

void DemoApp::drawViewerPanel() {
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
    ImGui::BulletText("F10:   Toggle Tool");

    ImGui::End();
}

// ─── Main drawGui ───────────────────────────────────────────────────────────

void DemoApp::drawGui() {
    if (showObjectPanel_) drawObjectPanel();
    if (showViewerPanel_) drawViewerPanel();
}

void DemoApp::postFrame() {
    OcctViewer* v = viewer();

    // Deferred rectangle selection — avoids blocking the render thread.
    if (pendingRectSelect_) {
        float pr = pixelRatio();
        if (pendingRectShift_)
            v->shiftSelectRectangle(rectX1_ * pr, rectY1_ * pr, rectX2_ * pr, rectY2_ * pr);
        else
            v->selectRectangle(rectX1_ * pr, rectY1_ * pr, rectX2_ * pr, rectY2_ * pr);
        pendingRectSelect_ = false;
    }

    if (!g_pendingActions.empty()) {
        for (auto& action : g_pendingActions)
            action(v);
        g_pendingActions.clear();
    }

    // Clean up deleted shapes (Null handle = marked for deletion)
    g_shapes.erase(
        std::remove_if(g_shapes.begin(), g_shapes.end(),
            [](const ShapeEntry& e) { return e.aisShape.IsNull(); }),
        g_shapes.end());

    v->updateViewer();
}

// ─── Import / Export ─────────────────────────────────────────────────────

void DemoApp::addImportedShape(const TopoDS_Shape& shape, const char* name) {
    if (shape.IsNull()) return;
    OcctViewer* v = viewer();

    float hue = (nextId_ * 0.37f);
    float r = 0.30f + 0.20f * sinf(hue);
    float g = 0.50f + 0.30f * sinf(hue + 2.0f);
    float b = 0.40f + 0.30f * sinf(hue + 4.0f);

    Handle(AIS_Shape) ais = new AIS_Shape(shape);
    Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
    aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
    ais->Attributes()->SetShadingAspect(aspect);

    v->displayShape(ais, true);
    g_shapes.push_back({ais, "", true, {r, g, b}, 0, 0, 0});
    strncpy(g_shapes.back().name, name, sizeof(g_shapes.back().name) - 1);
    printf("Imported: %s\n", name);
    nextId_++;
}

void DemoApp::onImport() {
    char path[1024] = {};
    if (!OcctViewer::openFileDialog(path, sizeof(path)))
        return;
    onImportFile(path);
}

void DemoApp::onImportFile(const char* path) {
    TopoDS_Shape shape = viewer()->importShape(path);
    if (shape.IsNull()) {
        printf("Import failed: %s\n", path);
        return;
    }
    const char* name = strrchr(path, '/');
#ifdef _WIN32
    const char* bs = strrchr(path, '\\');
    if (bs > name) name = bs;
#endif
    name = name ? name + 1 : path;
    addImportedShape(shape, name);
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

    // Export first selected shape (or merge into compound for multiple)
    if (selected.Size() == 1) {
        viewer()->exportShape(selected.First(), path);
    } else {
        // Merge into compound
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
