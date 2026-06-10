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

// imoguizmo MUST be included before any OCCT headers to avoid operator
// overload conflicts with ImVec2 arithmetic.
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imoguizmo.hpp"

#include "OcctImGui.h"
#include "OcctViewer.h"
#include "imgui.h"

#include <glad/glad.h>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pln.hxx>
#include <Graphic3d_ClipPlane.hxx>
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
        auto& entry = shapes_.emplace_back(ais, displayName, true, r, g, b, tx, ty, tz);
        extractFaces(entry, shape, r, g, b, tx, ty, tz);
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
            snprintf(statusMsg_, sizeof(statusMsg_), "Import failed: %s", p.c_str());
            statusTimer_ = 3.0f;
            return;
        }
        float r = 0.7f, g = 0.7f, b = 0.7f;
        Handle(AIS_Shape) ais = new AIS_Shape(shape);
        Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
        aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
        ais->Attributes()->SetShadingAspect(aspect);
        const char* name = strrchr(p.c_str(), '/');
#ifdef _WIN32
        const char* bs = strrchr(p.c_str(), '\\');
        if (bs > name) name = bs;
#endif
        name = name ? name + 1 : p.c_str();
        char buf[64];
        strncpy(buf, name, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        auto& entry = shapes_.emplace_back(ais, buf, true, r, g, b);
        extractFaces(entry, shape, r, g, b);
        if (!showFaces_)
            v->displayShape(ais);
        else
            for (auto& f : entry.faces) v->displayShape(f.aisFace, false);
        v->setSelectionMode(selectionMode_);
        snprintf(statusMsg_, sizeof(statusMsg_), "Imported %s (%d faces)", buf, (int)entry.faces.size());
        statusTimer_ = 4.0f;
        nextId_++;
    });
}

// ─── Protected Overrides ────────────────────────────────────────────────────

void Viewer::init() {
    viewer()->setShadedWithEdges();
    viewer()->setSelectionMode(selectionMode_);
    viewer()->setOrthographic(true);

    for (auto& ps : pendingShapes_) {
        Handle(AIS_Shape) ais = makeColoredShape(ps.shape, ps.r, ps.g, ps.b,
                                                 ps.tx, ps.ty, ps.tz);
        viewer()->displayShape(ais, false);
        auto& entry = shapes_.emplace_back(ais, ps.name, true, ps.r, ps.g, ps.b,
                                           ps.tx, ps.ty, ps.tz);
        extractFaces(entry, ps.shape, ps.r, ps.g, ps.b, ps.tx, ps.ty, ps.tz);
    }
    pendingShapes_.clear();

    if (!shapes_.empty())
        viewer()->fitAll();
}

void Viewer::drawGui() {
    if (showObjectPanel_) drawObjectPanel();
    if (showViewerPanel_) drawViewerPanel();  // Application::drawViewerPanel

    // ── Orientation gizmo (top-right corner) ──
    float viewMat[16], projMat[16];
    viewer()->getCameraMatrices(viewMat, projMat);

    float gizmoSize = 120.0f;
    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    ImOGuizmo::SetRect(vpSize.x - gizmoSize - 8.0f, 28.0f, gizmoSize);

    // Scale down axis length and drag sensitivity for our projection setup.
    // Default axisLengthScale=0.33 projects axes ~97 px (beyond the 60 px radius).
    // Default dragSensitivity=0.01 gives ~170°/sec at a 5 px/frame drag (far too fast).
    ImOGuizmo::config.axisLengthScale = 0.18f;
    ImOGuizmo::config.dragSensitivity = 0.003f;

    ImOGuizmo::BeginFrame();

    auto cam = viewer()->view()->Camera();
    float pivotDist = static_cast<float>(cam->Distance());

    if (ImOGuizmo::DrawGizmo(viewMat, projMat, pivotDist, ImOGuizmo::CoordinateSystem::YZX)) {
        viewer()->setCameraFromViewMatrix(viewMat);
    }
}

void Viewer::preFrame() {
    Application::preFrame();
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

// ─── Import / Export (built-in) ────────────────────────────────────────────

void Viewer::onImport() {
    char path[1024] = {};
    if (!OcctViewer::openFileDialog(path, sizeof(path)))
        return;
    onImportFile(path);
}

void Viewer::onImportFile(const char* path) {
    importFile(path);
}

void Viewer::onExport() {
    NCollection_List<TopoDS_Shape> selected;
    viewer()->getSelectedShapes(selected);
    if (selected.IsEmpty()) {
        snprintf(statusMsg_, sizeof(statusMsg_), "No shapes selected for export");
        statusTimer_ = 3.0f;
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
    snprintf(statusMsg_, sizeof(statusMsg_), "Exported: %s", path);
    statusTimer_ = 4.0f;
}

// ─── Panels ─────────────────────────────────────────────────────────────────

void Viewer::drawObjectPanel() {
    ImGui::Begin("Objects", &showObjectPanel_);

    // ── View settings ──
    ImGui::Text("View");
    ImGui::Separator();

    // Display mode
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Display");
    ImGui::SameLine(80);
    ImGui::PushItemWidth(-1);
    const char* dispModes[] = { "Model", "Faces" };
    int dispCur = showFaces_ ? 1 : 0;
    if (ImGui::Combo("##display", &dispCur, dispModes, 2)) {
        showFaces_ = (dispCur == 1);
        syncShapeDisplay();
        viewer()->setSelectionMode(selectionMode_);
    }
    ImGui::PopItemWidth();

    // Selection mode
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Select");
    ImGui::SameLine(80);
    ImGui::PushItemWidth(-1);
    const char* selModes[] = { "Shape", "Face", "Edge", "Vertex" };
    int selVals[] = {
        AIS_Shape::SelectionMode(TopAbs_SHAPE),
        AIS_Shape::SelectionMode(TopAbs_FACE),
        AIS_Shape::SelectionMode(TopAbs_EDGE),
        AIS_Shape::SelectionMode(TopAbs_VERTEX)
    };
    int selCur = 0;
    for (int i = 0; i < 4; ++i) {
        if (selectionMode_ == selVals[i]) { selCur = i; break; }
    }
    if (ImGui::Combo("##selmode", &selCur, selModes, 4)) {
        selectionMode_ = selVals[selCur];
        viewer()->setSelectionMode(selectionMode_);
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Scene Objects");
    ImGui::Separator();

    if (shapes_.empty()) {
        ImGui::TextDisabled("No objects in scene");
    }

    for (int i = 0; i < (int)shapes_.size(); ++i) {
        auto& entry = shapes_[i];
        ImGui::PushID(i);

        // ── Parent model row ──
        bool expanded = entry.facesExpanded;
        bool hasFaces = !entry.faces.empty();
        if (hasFaces && showFaces_) {
            ImGui::AlignTextToFramePadding();
            if (ImGui::ArrowButton("##expand", expanded ?
                ImGuiDir_Down : ImGuiDir_Right)) {
                entry.facesExpanded = !expanded;
            }
            ImGui::SameLine();
        } else if (hasFaces) {
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
            ImGui::SameLine();
        } else {
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
            ImGui::SameLine();
        }

        if (ImGui::Checkbox("##vis", &entry.visible)) {
            bool vis = entry.visible;
            for (auto& f : entry.faces) f.visible = vis;
            auto shape = entry.aisShape;
            std::vector<Handle(AIS_Shape)> faceShapes;
            for (auto& f : entry.faces) faceShapes.push_back(f.aisFace);
            bool useModel = !showFaces_;
            pendingActions_.push_back([shape, faceShapes, vis, useModel](OcctViewer* v) {
                if (useModel) {
                    if (vis) v->displayShape(shape, false);
                    else     v->removeShape(shape, false);
                } else {
                    for (auto& fs : faceShapes) {
                        if (vis) v->displayShape(fs, false);
                        else     v->removeShape(fs, false);
                    }
                }
            });
        }
        ImGui::SameLine();
        // Rainbow if 2+ faces have different colors
        bool rainbow = false;
        if (entry.faces.size() >= 2) {
            float c0 = entry.faces[0].color[0], c1 = entry.faces[0].color[1], c2 = entry.faces[0].color[2];
            for (auto& f : entry.faces) {
                if (f.color[0] != c0 || f.color[1] != c1 || f.color[2] != c2) { rainbow = true; break; }
            }
        }
        ImVec2 swatchPos = ImGui::GetCursorScreenPos();
        float swatchH = ImGui::GetFrameHeight();
        if (ImGui::ColorEdit3("##color", entry.color,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder)) {
            float r = entry.color[0], g = entry.color[1], b = entry.color[2];
            auto shape = entry.aisShape;
            for (auto& f : entry.faces) {
                f.color[0] = r; f.color[1] = g; f.color[2] = b;
                f.useCustomColor = false;
            }
            std::vector<Handle(AIS_Shape)> faceShapes;
            for (auto& f : entry.faces) faceShapes.push_back(f.aisFace);
            pendingActions_.push_back([shape, faceShapes, r, g, b](OcctViewer* v) {
                Handle(Prs3d_ShadingAspect) a = new Prs3d_ShadingAspect();
                a->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
                shape->Attributes()->SetShadingAspect(a);
                shape->Redisplay(false);
                for (auto& fs : faceShapes) {
                    Handle(Prs3d_ShadingAspect) fa = new Prs3d_ShadingAspect();
                    fa->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
                    fs->Attributes()->SetShadingAspect(fa);
                    fs->Redisplay(false);
                }
            });
        }
        if (rainbow) {
            // Black rounded background (not via PushStyleColor)
            float r = ImGui::GetStyle().FrameRounding;
            ImVec2 swatchEnd(swatchPos.x + swatchH, swatchPos.y + swatchH);
            ImGui::GetWindowDrawList()->AddRectFilled(swatchPos, swatchEnd, IM_COL32(0,0,0,255), r);
            // Rainbow inset 2px
            ImU32 mixed[4] = {
                IM_COL32(0xFF, 0x33, 0x33, 0xFF),
                IM_COL32(0x33, 0x99, 0xFF, 0xFF),
                IM_COL32(0x33, 0xCC, 0x33, 0xFF),
                IM_COL32(0xFF, 0xCC, 0x00, 0xFF),
            };
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                ImVec2(swatchPos.x + 2, swatchPos.y + 2),
                ImVec2(swatchPos.x + swatchH - 2, swatchPos.y + swatchH - 2),
                mixed[0], mixed[1], mixed[2], mixed[3]);
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
                for (auto& f : entry.faces) {
                    auto ff = f.aisFace;
                    pendingActions_.push_back([ff](OcctViewer* v) {
                        v->removeShape(ff, false);
                    });
                }
                entry.aisShape.Nullify();
                entry.faces.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // ── Expand: face sub-objects ──
        if (showFaces_ && expanded) {
            for (auto& f : entry.faces) {
                ImGui::PushID(1000 + f.id);
                ImGui::AlignTextToFramePadding();
                ImGui::Indent(16.0f);
                char faceLabel[32];
                snprintf(faceLabel, sizeof(faceLabel), "Face %d", f.id);
                ImGui::TextUnformatted(faceLabel);
                ImGui::SameLine();
                if (ImGui::Checkbox("##fvis", &f.visible)) {
                    bool vis = f.visible;
                    pendingActions_.push_back([face = f.aisFace, vis](OcctViewer* v) {
                        if (vis) v->displayShape(face, false);
                        else     v->removeShape(face, false);
                    });
                }
                ImGui::SameLine();
                if (ImGui::ColorEdit3("##fcolor", f.color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                    f.useCustomColor = true;
                    entry.color[0] = f.color[0];
                    entry.color[1] = f.color[1];
                    entry.color[2] = f.color[2];
                    float fr = f.color[0], fg = f.color[1], fb = f.color[2];
                    pendingActions_.push_back([face = f.aisFace, fr, fg, fb](OcctViewer* v) {
                        Handle(Prs3d_ShadingAspect) a = new Prs3d_ShadingAspect();
                        a->SetColor(Quantity_Color(fr, fg, fb, Quantity_TOC_RGB));
                        face->Attributes()->SetShadingAspect(a);
                        face->Redisplay(false);
                    });
                }
                ImGui::Unindent(16.0f);
                ImGui::PopID();
            }
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
            for (auto& f : entry.faces) { f.visible = true; }
            std::vector<Handle(AIS_Shape)> faceShapes;
            for (auto& f : entry.faces) faceShapes.push_back(f.aisFace);
            pendingActions_.push_back([faceShapes](OcctViewer* v) {
                for (auto& fs : faceShapes) v->displayShape(fs, false);
            });
        }
    }

    if (ImGui::Button("Hide All", ImVec2(-1, 0))) {
        for (auto& entry : shapes_) {
            if (!entry.visible) continue;
            entry.visible = false;
            for (auto& f : entry.faces) { f.visible = false; }
            std::vector<Handle(AIS_Shape)> faceShapes;
            for (auto& f : entry.faces) faceShapes.push_back(f.aisFace);
            pendingActions_.push_back([faceShapes](OcctViewer* v) {
                for (auto& fs : faceShapes) v->removeShape(fs, false);
            });
        }
    }

    if (ImGui::Button("Delete All", ImVec2(-1, 0)))
        ImGui::OpenPopup("Delete All?");
    if (ImGui::BeginPopupModal("Delete All?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Remove all %d shapes?", (int)shapes_.size());
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            for (auto& entry : shapes_) {
                for (auto& f : entry.faces) {
                    auto ff = f.aisFace;
                    pendingActions_.push_back([ff](OcctViewer* v) {
                        v->removeShape(ff, false);
                    });
                }
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

    // ── Tool ──
    ImGui::Spacing();
    ImGui::Text("Tool");
    ImGui::Separator();

    if (ImGui::Checkbox("Clipping", &sectionOn_))
        pendingActions_.push_back([this](OcctViewer*) { updateClipPlane(); });

    if (sectionOn_) {
        if (ImGui::Checkbox("Edit Clip", &clipEditOn_))
            pendingActions_.push_back([this](OcctViewer*) { updateClipPlane(); });

        if (clipEditOn_) {
            OcctViewer* v = viewer();
            double xmin, ymin, zmin, xmax, ymax, zmax;
            v->getBoundingBox(&xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
            double diag = sqrt((xmax-xmin)*(xmax-xmin) + (ymax-ymin)*(ymax-ymin) + (zmax-zmin)*(zmax-zmin));
            float range = (float)(diag * 0.8);
            if (ImGui::SliderFloat("Position", &clipPos_, -range, range))
                pendingActions_.push_back([this](OcctViewer*) { updateClipPlane(); });
        }
    }

    ImGui::End();
}

// ─── Section / clip plane ─────────────────────────────────────────────────

void Viewer::updateClipPlane() {
    OcctViewer* v = viewer();
    if (!sectionOn_) {
        if (v->hasClipPlane(kSectionPlaneId))
            v->removeClipPlane(kSectionPlaneId);
        return;
    }

    if (!v->hasClipPlane(kSectionPlaneId)) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        v->getBoundingBox(&xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
        clipCenter_ = gp_Pnt((xmin+xmax)*0.5, (ymin+ymax)*0.5, (zmin+zmax)*0.5);
        gp_Pnt eye = v->view()->Camera()->Eye();
        clipNormal_ = gp_Dir(clipCenter_.X()-eye.X(), clipCenter_.Y()-eye.Y(), clipCenter_.Z()-eye.Z());
        gp_Dir camUp = v->view()->Camera()->Up();
        clipDx_ = gp_Dir(camUp.Y()*clipNormal_.Z()-camUp.Z()*clipNormal_.Y(),
                         camUp.Z()*clipNormal_.X()-camUp.X()*clipNormal_.Z(),
                         camUp.X()*clipNormal_.Y()-camUp.Y()*clipNormal_.X());
        clipDy_ = gp_Dir(clipNormal_.Y()*clipDx_.Z()-clipNormal_.Z()*clipDx_.Y(),
                         clipNormal_.Z()*clipDx_.X()-clipNormal_.X()*clipDx_.Z(),
                         clipNormal_.X()*clipDx_.Y()-clipNormal_.Y()*clipDx_.X());
        v->addClipPlane(kSectionPlaneId, gp_Pln(clipCenter_, clipNormal_));
    } else if (clipEditOn_) {
        gp_Pnt origin(clipCenter_.X() + clipNormal_.X()*clipPos_,
                      clipCenter_.Y() + clipNormal_.Y()*clipPos_,
                      clipCenter_.Z() + clipNormal_.Z()*clipPos_);
        v->setClipPlaneEquation(kSectionPlaneId, gp_Pln(origin, clipNormal_));
    }
}

void Viewer::drawOverlay() {
    if (!sectionOn_) return;

    static bool gladLoaded = false;
    if (!gladLoaded) {
        gladLoaded = true;
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    }

    // ── Lazy-init GL resources ──────────────────────────────────────────
    static GLuint prog = 0, vao = 0, vbo = 0;
    if (!prog) {
        const char* vsSrc = "#version 150\n"
            "uniform mat4 uMVP;\n"
            "in vec3 aPos;\n"
            "void main() { gl_Position = uMVP * vec4(aPos, 1.0); }";
        const char* fsSrc = "#version 150\n"
            "uniform vec3 uColor;\n"
            "out vec4 outColor;\n"
            "void main() { outColor = vec4(uColor, 1.0); }";

        auto compile = [](GLenum type, const char* src) {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            return s;
        };
        GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
        prog = glCreateProgram();
        glAttachShader(prog, vs); glAttachShader(prog, fs);
        glLinkProgram(prog);
        glDeleteShader(vs); glDeleteShader(fs);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    // ── Build world-space clip plane origin ─────────────────────────────
    OcctViewer* v = viewer();
    auto cam = v->view()->Camera();

    gp_Pnt planeOrigin(clipCenter_.X() + clipNormal_.X() * clipPos_,
                      clipCenter_.Y() + clipNormal_.Y() * clipPos_,
                      clipCenter_.Z() + clipNormal_.Z() * clipPos_);

    double xmin, ymin, zmin, xmax, ymax, zmax;
    v->getBoundingBox(&xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
    double diag = sqrt((xmax-xmin)*(xmax-xmin)+(ymax-ymin)*(ymax-ymin)+(zmax-zmin)*(zmax-zmin));
    float w = (float)(diag * 1.25);

    // ── MVP matrices ────────────────────────────────────────────────────
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    float aspect = (vp[2] > 0 && vp[3] > 0) ? (float)vp[2] / (float)vp[3] : 1.0f;

    // --- View matrix (lookAt) ---
    gp_Pnt eyeP = cam->Eye(), ctrP = cam->Center();
    gp_Dir upD = cam->Up();
    float fx = (float)(ctrP.X()-eyeP.X()), fy = (float)(ctrP.Y()-eyeP.Y()), fz = (float)(ctrP.Z()-eyeP.Z());
    float fl = sqrtf(fx*fx+fy*fy+fz*fz); if (fl>1e-10f) { fx/=fl; fy/=fl; fz/=fl; }
    float ux=(float)upD.X(), uy=(float)upD.Y(), uz=(float)upD.Z();
    float rx=fy*uz-fz*uy, ry=fz*ux-fx*uz, rz=fx*uy-fy*ux;
    float rl=sqrtf(rx*rx+ry*ry+rz*rz); if (rl>1e-10f) { rx/=rl; ry/=rl; rz/=rl; }
    ux=ry*fz-rz*fy; uy=rz*fx-rx*fz; uz=rx*fy-ry*fx;
    float ex=(float)eyeP.X(), ey=(float)eyeP.Y(), ez=(float)eyeP.Z();

    float view[16] = {
        rx, ux, -fx, 0,
        ry, uy, -fy, 0,
        rz, uz, -fz, 0,
        -(rx*ex+ry*ey+rz*ez), -(ux*ex+uy*ey+uz*ez), (fx*ex+fy*ey+fz*ez), 1
    };

    // --- Projection matrix (perspective) ---
    float fov = (float)cam->FOVy();
    if (fov < 0.01f || fov > 3.0f) fov = 0.785398163f; // ~45 deg
    float f = 1.0f / tanf(fov * 0.5f);
    float zn = 0.1f, zf = 10000.0f;
    float proj[16] = {
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, -(zf+zn)/(zf-zn), -1,
        0, 0, -(2*zf*zn)/(zf-zn), 0
    };

    // --- Model matrix (clip-plane local → world) ---
    float model[16] = {
        (float)(clipDx_.X()*w),    (float)(clipDx_.Y()*w),    (float)(clipDx_.Z()*w),    0,
        (float)(clipDy_.X()*w),    (float)(clipDy_.Y()*w),    (float)(clipDy_.Z()*w),    0,
        (float)(clipNormal_.X()),  (float)(clipNormal_.Y()),  (float)(clipNormal_.Z()),  0,
        (float)planeOrigin.X(),    (float)planeOrigin.Y(),    (float)planeOrigin.Z(),    1
    };

    // --- MVP = proj * view * model ---
    auto mul4 = [](const float* A, const float* B, float* R) {
        for (int i=0; i<4; ++i) {
            for (int j=0; j<4; ++j) {
                R[j*4+i] = A[0*4+i]*B[j*4+0] + A[1*4+i]*B[j*4+1]
                         + A[2*4+i]*B[j*4+2] + A[3*4+i]*B[j*4+3];
            }
        }
    };
    float vm[16], mvp[16];
    mul4(view, model, vm);
    mul4(proj, vm, mvp);

    // ── Draw ────────────────────────────────────────────────────────────
    // Unit-size cross + square in clip-plane XY (z=0)
    float lines[] = {
        -1,0,0, 1,0,0,   0,-1,0, 0,1,0,               // cross
        // square × 3 for thicker line appearance
        -1,-1,0, 1,-1,0, 1,-1,0, 1,1,0, 1,1,0, -1,1,0, -1,1,0, -1,-1,0,
        -1.01f,-1.01f,0, 1.01f,-1.01f,0, 1.01f,-1.01f,0, 1.01f,1.01f,0, 1.01f,1.01f,0, -1.01f,1.01f,0, -1.01f,1.01f,0, -1.01f,-1.01f,0,
        -1.02f,-1.02f,0, 1.02f,-1.02f,0, 1.02f,-1.02f,0, 1.02f,1.02f,0, 1.02f,1.02f,0, -1.02f,1.02f,0, -1.02f,1.02f,0, -1.02f,-1.02f,0,
    };

    // Save GL state
    GLint prevProg=0, prevVAO=0, prevBlend=0, prevDepth=0, prevCull=0;
    GLboolean prevLineSmooth=GL_FALSE;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    prevBlend = glIsEnabled(GL_BLEND);
    prevDepth = glIsEnabled(GL_DEPTH_TEST);
    prevCull  = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, GL_FALSE, mvp);
    glUniform3f(glGetUniformLocation(prog, "uColor"), 0.1f, 0.1f, 0.1f);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_LINES, 0, 28);

    // Restore GL state
    glBindVertexArray(prevVAO);
    glUseProgram(prevProg);
    if (!prevBlend) glDisable(GL_BLEND);
    if (!prevDepth) glDisable(GL_DEPTH_TEST);
    if (prevCull)   glEnable(GL_CULL_FACE);
}

void Viewer::syncShapeDisplay() {
    for (auto& entry : shapes_) {
        if (!showFaces_) {
            if (entry.visible) viewer()->displayShape(entry.aisShape, false);
            for (auto& f : entry.faces) viewer()->removeShape(f.aisFace, false);
        } else {
            viewer()->removeShape(entry.aisShape, false);
            for (auto& f : entry.faces)
                if (entry.visible && f.visible) viewer()->displayShape(f.aisFace, false);
        }
    }
    viewer()->updateViewer();
}

void Viewer::extractFaces(ShapeEntry& entry, const TopoDS_Shape& shape,
                           float r, float g, float b,
                           double tx, double ty, double tz) {
    entry.faces.clear();
    int id = 0;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        Handle(AIS_Shape) faceAis = new AIS_Shape(face);
        Handle(Prs3d_ShadingAspect) aspect = new Prs3d_ShadingAspect();
        aspect->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
        faceAis->Attributes()->SetShadingAspect(aspect);
        if (tx != 0 || ty != 0 || tz != 0) {
            gp_Trsf trsf;
            trsf.SetTranslation(gp_Vec(tx, ty, tz));
            faceAis->SetLocalTransformation(trsf);
        }
        entry.faces.emplace_back(faceAis, id++, r, g, b);
    }
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
