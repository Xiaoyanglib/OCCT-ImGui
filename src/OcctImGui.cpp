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
#include "imgui.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Quantity_Color.hxx>
#include <Prs3d_ShadingAspect.hxx>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

namespace OcctImGui {

// ─── Lifecycle ──────────────────────────────────────────────────────────────

Viewer::Viewer() {}

Viewer::~Viewer() {}

void Viewer::show() {
    run();
}

// ─── Public API ─────────────────────────────────────────────────────────────

void Viewer::addShape(const TopoDS_Shape& shape, float r, float g, float b,
                      const char* name, double tx, double ty, double tz) {
    std::string displayName;
    if (name && name[0])
        displayName = name;
    else {
        displayName = "Shape " + std::to_string(nextId_);
    }
    nextId_++;

    if (viewer()) {
        Handle(AIS_Shape) ais = makeColoredShape(shape, r, g, b, tx, ty, tz);
        viewer()->displayShape(ais);
        shapes_.emplace_back(ais, displayName, true, r, g, b, tx, ty, tz);
    } else {
        pendingShapes_.push_back({shape, r, g, b, displayName, tx, ty, tz});
    }
}

void Viewer::fitAll() {
    pendingActions_.push_back([](OcctViewer* v) { v->fitAll(); });
}

void Viewer::importFile(const char* path) {
    std::string p(path);
    pendingActions_.push_back([this, p](OcctViewer* v) {
        TopoDS_Shape shape = v->importShape(p.c_str());
        if (shape.IsNull()) {
            printf("Import failed: %s\n", p.c_str());
            return;
        }
        float hue = (nextId_ * 0.37f);
        float r = 0.30f + 0.20f * sinf(hue);
        float g = 0.50f + 0.30f * sinf(hue + 2.0f);
        float b = 0.40f + 0.30f * sinf(hue + 4.0f);
        Handle(AIS_Shape) ais = new AIS_Shape(shape);
        Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
        aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
        ais->Attributes()->SetShadingAspect(aspect);
        v->displayShape(ais);
        const char* name = strrchr(p.c_str(), '/');
#ifdef _WIN32
        const char* bs = strrchr(p.c_str(), '\\');
        if (bs > name) name = bs;
#endif
        name = name ? name + 1 : p.c_str();
        char buf[64];
        strncpy(buf, name, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        shapes_.emplace_back(ais, buf, true, r, g, b);
        nextId_++;
    });
}

// ─── Protected Overrides ────────────────────────────────────────────────────

void Viewer::init() {
    viewer()->setShadedWithEdges();

    for (auto& ps : pendingShapes_) {
        Handle(AIS_Shape) ais = makeColoredShape(ps.shape, ps.r, ps.g, ps.b,
                                                 ps.tx, ps.ty, ps.tz);
        viewer()->displayShape(ais, false);
        shapes_.emplace_back(ais, ps.name, true, ps.r, ps.g, ps.b,
                            ps.tx, ps.ty, ps.tz);
    }
    pendingShapes_.clear();

    if (!shapes_.empty())
        viewer()->fitAll();
}

void Viewer::drawGui() {
    if (showObjectPanel_) drawObjectPanel();
    if (showViewerPanel_) drawViewerPanel();  // Application::drawViewerPanel
}

void Viewer::postFrame() {
    Application::postFrame();  // rect select handling

    OcctViewer* v = viewer();
    if (!pendingActions_.empty()) {
        for (auto& action : pendingActions_)
            action(v);
        pendingActions_.clear();
    }

    shapes_.erase(
        std::remove_if(shapes_.begin(), shapes_.end(),
                       [](const ShapeEntry& e) { return e.aisShape.IsNull(); }),
        shapes_.end());

    v->updateViewer();
}

// ─── Panels ─────────────────────────────────────────────────────────────────

void Viewer::drawObjectPanel() {
    ImGui::Begin("Objects", &showObjectPanel_);

    ImGui::Text("Scene Objects");
    ImGui::Separator();

    if (shapes_.empty()) {
        ImGui::TextDisabled("No objects in scene");
    }

    for (int i = 0; i < (int)shapes_.size(); ++i) {
        auto& entry = shapes_[i];
        ImGui::PushID(i);

        if (ImGui::Checkbox("##vis", &entry.visible)) {
            auto shape = entry.aisShape;
            bool vis = entry.visible;
            pendingActions_.push_back([shape, vis](OcctViewer* v) {
                if (vis) v->displayShape(shape, false);
                else     v->removeShape(shape, false);
            });
        }
        ImGui::SameLine();
        if (ImGui::ColorEdit3("##color", entry.color,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            auto shape = entry.aisShape;
            float r = entry.color[0], g = entry.color[1], b = entry.color[2];
            pendingActions_.push_back([shape, r, g, b](OcctViewer* v) {
                Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
                aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
                shape->Attributes()->SetShadingAspect(aspect);
                shape->Redisplay(false);
            });
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(entry.name.c_str());

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
            ImGui::Text("Remove '%s'?", entry.name.c_str());
            if (ImGui::Button("Yes", ImVec2(80, 0))) {
                auto shape = entry.aisShape;
                pendingActions_.push_back([shape](OcctViewer* v) {
                    v->removeShape(shape, true);
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
    ImGui::Text("Actions");
    ImGui::Separator();

    if (ImGui::Button("Fit All", ImVec2(-1, 0)))
        fitAll();

    if (ImGui::Button("Show All", ImVec2(-1, 0))) {
        for (auto& entry : shapes_) {
            if (entry.visible) continue;
            entry.visible = true;
            auto shape = entry.aisShape;
            pendingActions_.push_back([shape](OcctViewer* v) {
                v->displayShape(shape, false);
            });
        }
    }

    if (ImGui::Button("Hide All", ImVec2(-1, 0))) {
        for (auto& entry : shapes_) {
            if (!entry.visible) continue;
            entry.visible = false;
            auto shape = entry.aisShape;
            pendingActions_.push_back([shape](OcctViewer* v) {
                v->removeShape(shape, false);
            });
        }
    }

    if (ImGui::Button("Delete All", ImVec2(-1, 0)))
        ImGui::OpenPopup("Delete All?");
    if (ImGui::BeginPopupModal("Delete All?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Remove all %d shapes?", (int)shapes_.size());
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            for (auto& entry : shapes_) {
                auto shape = entry.aisShape;
                pendingActions_.push_back([shape](OcctViewer* v) {
                    v->removeShape(shape, true);
                });
            }
            shapes_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(80, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Text("Add Primitive");
    ImGui::Separator();

    if (ImGui::Button("+ Box",      ImVec2(-1, 0))) addShape(BRepPrimAPI_MakeBox(40, 40, 40).Shape(), 0.30f, 0.60f, 0.50f, "Box");
    if (ImGui::Button("+ Sphere",   ImVec2(-1, 0))) addShape(BRepPrimAPI_MakeSphere(25).Shape(),     0.40f, 0.80f, 0.60f, "Sphere");
    if (ImGui::Button("+ Cylinder", ImVec2(-1, 0))) addShape(BRepPrimAPI_MakeCylinder(18, 50).Shape(), 0.27f, 0.50f, 0.68f, "Cylinder");

    ImGui::End();
}

// ─── Helpers ────────────────────────────────────────────────────────────────

Handle(AIS_Shape) Viewer::makeColoredShape(const TopoDS_Shape& shape,
                                           float r, float g, float b,
                                           double tx, double ty, double tz) {
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

} // namespace OcctImGui
