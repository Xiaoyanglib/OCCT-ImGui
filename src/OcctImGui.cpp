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

#include "../include/OcctImGui/OcctImGui.h"
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
#include <XCAFPrs_AISObject.hxx>
#include <TDocStd_Document.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_VisMaterial.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

namespace OcctImGui {

// ─── XCAF face-color helper ─────────────────────────────────────────────────

static void setXcafFaceColor(const Viewer::ShapeEntry& entry,
                              const TopoDS_Face& face,
                              float r, float g, float b) {
    if (entry.xcafDoc.IsNull() || entry.xcafShapeLabel.IsNull()) return;
    Handle(XCAFDoc_ShapeTool) st =
        XCAFDoc_DocumentTool::ShapeTool(entry.xcafDoc->Main());
    Handle(XCAFDoc_ColorTool) ct =
        XCAFDoc_DocumentTool::ColorTool(entry.xcafDoc->Main());
    TDF_Label fl = st->AddSubShape(entry.xcafShapeLabel, face);
    ct->SetColor(fl,
        Quantity_Color(r, g, b, Quantity_TOC_RGB), XCAFDoc_ColorSurf);
    Handle(XCAFDoc_VisMaterialTool) mt =
        XCAFDoc_DocumentTool::VisMaterialTool(entry.xcafDoc->Main());
    TDF_Label baseMat;
    if (mt->GetShapeMaterial(entry.xcafShapeLabel, baseMat))
        mt->SetShapeMaterial(fl, baseMat);
}

// ─── Lifecycle ──────────────────────────────────────────────────────────────

Viewer::Viewer() {}

Viewer::~Viewer() {}

// ─── Public API ─────────────────────────────────────────────────────────────

int Viewer::addShape(const TopoDS_Shape& shape, float r, float g, float b,
                      const char* name, double tx, double ty, double tz) {
    std::string displayName;
    if (name && name[0])
        displayName = name;
    else {
        displayName = "Shape " + std::to_string(nextId_);
    }
    nextId_++;

    if (viewer()) {
        Handle(TDocStd_Document) doc;
        TDF_Label shapeLabel;
        Handle(AIS_Shape) ais = makeColoredShape(shape, r, g, b, tx, ty, tz, &doc, &shapeLabel);
        viewer()->displayShape(ais);
        auto& entry = shapes_.emplace_back(ais, displayName, true, r, g, b, tx, ty, tz);
        entry.xcafDoc = doc;
        entry.xcafShapeLabel = shapeLabel;
        extractFaces(entry, shape, r, g, b, tx, ty, tz);
        return (int)shapes_.size() - 1;
    } else {
        auto& entry = shapes_.emplace_back(Handle(AIS_Shape)(), displayName, true, r, g, b, tx, ty, tz);
        extractFaces(entry, shape, r, g, b, tx, ty, tz);
        int idx = (int)shapes_.size() - 1;
        pendingShapes_.push_back({shape, r, g, b, displayName, tx, ty, tz, (size_t)idx});
        return idx;
    }
}

void Viewer::setFaceColor(int shapeIdx, int faceId, float r, float g, float b) {
    if (shapeIdx < 0 || shapeIdx >= (int)shapes_.size()) return;
    auto& entry = shapes_[shapeIdx];
    for (auto& f : entry.faces) {
        if (f.id == faceId) {
            f.color[0]=r; f.color[1]=g; f.color[2]=b; f.useCustomColor=true;
            setXcafFaceColor(entry, f.topoFace, r, g, b);
            return;
        }
    }
}

void Viewer::fitAll() {
    pendingActions_.push_back([](OcctViewer* v) { v->fitAll(); });
}

static Handle(AIS_ColoredShape) makeAisForLabel(Handle(TDocStd_Document)& doc,
                                                   const TDF_Label& label) {
    XCAFDoc_VisMaterialCommon cm;
    cm.DiffuseColor  = Quantity_Color(0.7f, 0.7f, 0.7f, Quantity_TOC_RGB);
    cm.AmbientColor  = Quantity_Color(0.14f, 0.14f, 0.14f, Quantity_TOC_RGB);
    cm.SpecularColor = Quantity_Color(0.5f, 0.5f, 0.5f, Quantity_TOC_RGB);
    cm.Shininess     = 0.6f;
    Handle(XCAFDoc_VisMaterial) mat = new XCAFDoc_VisMaterial();
    mat->SetCommonMaterial(cm);
    Handle(XCAFDoc_VisMaterialTool) mt =
        XCAFDoc_DocumentTool::VisMaterialTool(doc->Main());
    TDF_Label ml = mt->AddMaterial(mat, "Plastic");
    mt->SetShapeMaterial(label, ml);
    Handle(AIS_ColoredShape) a = new XCAFPrs_AISObject(label);
    a->Attributes()->SetFaceBoundaryDraw(true);
    a->Attributes()->SetFaceBoundaryAspect(
        new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0));
    return a;
}

void Viewer::importFile(const char* path) {
    std::string p(path);
    snprintf(statusMsg_, sizeof(statusMsg_), "Importing %s...", path);
    pendingActions_.push_back([this, p](OcctViewer* v) {
        std::vector<FaceColorInfo> faceColors;
        TDF_Label xcafLabel;
        Handle(TDocStd_Document) xcafDoc;
        std::vector<TDF_Label> solidLabels;
        std::vector<int> solidFaceCounts;
        std::vector<std::vector<FaceColorInfo>> solidColors;
        TopoDS_Shape shape = v->importShape(p.c_str(), &faceColors, &xcafLabel, &xcafDoc,
                                             &solidLabels, &solidFaceCounts, &solidColors);
        if (shape.IsNull()) {
            snprintf(statusMsg_, sizeof(statusMsg_), "Import failed: %s", p.c_str());
            return;
        }
        if (solidLabels.empty()) {
            solidLabels.push_back(xcafLabel);
            solidFaceCounts.push_back((int)faceColors.size());
            solidColors.push_back(faceColors);
        }
        const char* name = strrchr(p.c_str(), '/');
#ifdef _WIN32
        const char* bs = strrchr(p.c_str(), '\\');
        if (bs > name) name = bs;
#endif
        name = name ? name + 1 : p.c_str();
        char buf[64];
        strncpy(buf, name, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        if (solidLabels.size() <= 1) {
            // Single solid — flat entry
            auto a = makeAisForLabel(xcafDoc, solidLabels[0]);
            v->displayShape(a);
            auto& entry = shapes_.emplace_back(a, buf, true, 0.7f, 0.7f, 0.7f);
            entry.xcafDoc = xcafDoc;
            entry.xcafShapeLabel = solidLabels[0];
            extractFaces(entry, shape, 0.7f, 0.7f, 0.7f);
            for (size_t i = 0; i < faceColors.size() && i < entry.faces.size(); i++) {
                entry.faces[i].color[0] = faceColors[i].r;
                entry.faces[i].color[1] = faceColors[i].g;
                entry.faces[i].color[2] = faceColors[i].b;
                if (faceColors[i].r != 0.7f || faceColors[i].g != 0.7f || faceColors[i].b != 0.7f)
                    entry.faces[i].useCustomColor = true;
            }
        } else {
            // Multi-solid — parent (no AIS) + child per solid
            int parentIdx = (int)shapes_.size();
            auto& parent = shapes_.emplace_back(Handle(AIS_Shape)(), buf, true, 0.7f, 0.7f, 0.7f);
            parent.xcafDoc = xcafDoc;
            for (size_t si = 0; si < solidLabels.size(); si++) {
                auto a = makeAisForLabel(xcafDoc, solidLabels[si]);
                v->displayShape(a);
                char sn[32]; snprintf(sn, sizeof(sn), "Solid %d", (int)si+1);
                auto& child = shapes_.emplace_back(a, sn, true, 0.7f, 0.7f, 0.7f);
                child.parentIdx = parentIdx;
                child.xcafDoc = xcafDoc;
                child.xcafShapeLabel = solidLabels[si];
                Handle(XCAFDoc_ShapeTool) st2 =
                    XCAFDoc_DocumentTool::ShapeTool(xcafDoc->Main());
                TopoDS_Shape ss = st2->GetShape(solidLabels[si]);
                extractFaces(child, ss, 0.7f, 0.7f, 0.7f);
                int idOff = 0;
                for (size_t sj = 0; sj < si; sj++) idOff += solidFaceCounts[sj];
                for (auto& f : child.faces) f.id += idOff;
                auto& sc = (si < solidColors.size()) ? solidColors[si] : faceColors;
                for (size_t i = 0; i < sc.size() && i < child.faces.size(); i++) {
                    child.faces[i].color[0] = sc[i].r;
                    child.faces[i].color[1] = sc[i].g;
                    child.faces[i].color[2] = sc[i].b;
                    if (sc[i].r != 0.7f || sc[i].g != 0.7f || sc[i].b != 0.7f)
                        child.faces[i].useCustomColor = true;
                }
            }
        }
        v->setSelectionMode(selectionMode_);
        snprintf(statusMsg_, sizeof(statusMsg_), "Imported %s (%d solids, %d faces)",
                 buf, (int)solidLabels.size(), (int)faceColors.size());
        nextId_++;
    });
}

// ─── Protected Overrides ────────────────────────────────────────────────────

void Viewer::init() {
    viewer()->setShadedWithEdges();
    viewer()->setSelectionMode(selectionMode_);
    viewer()->setOrthographic(true);

    for (auto& ps : pendingShapes_) {
        Handle(TDocStd_Document) doc;
        TDF_Label shapeLabel;
        Handle(AIS_Shape) ais = makeColoredShape(ps.shape, ps.r, ps.g, ps.b,
                                                 ps.tx, ps.ty, ps.tz, &doc, &shapeLabel);
        auto& entry = shapes_[ps.shapeIdx];
        entry.aisShape = ais;
        entry.xcafDoc = doc;
        entry.xcafShapeLabel = shapeLabel;
        // Apply saved custom face colors BEFORE display
        auto cs = Handle(AIS_ColoredShape)::DownCast(ais);
        if (!cs.IsNull()) {
            for (auto& f : entry.faces) {
                if (f.useCustomColor)
                    setXcafFaceColor(entry, f.topoFace, f.color[0], f.color[1], f.color[2]);
            }
        }
        viewer()->displayShape(ais, false);
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
    ImOGuizmo::SetRect(vpSize.x - gizmoSize - 8.0f, ImGui::GetFrameHeight() + 12.0f, gizmoSize);

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
                       [&](const ShapeEntry& e) {
                           if (!e.aisShape.IsNull()) return false;
                           int idx = (int)(&e - shapes_.data());
                           for (auto& o : shapes_)
                               if (o.parentIdx == idx) return false;
                           return true;
                       }),
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
        return;
    }

    char path[1024] = {};
    if (!OcctViewer::saveFileDialog(path, sizeof(path)))
        return;

    // Collect per-face custom colors from selected shapes
    std::vector<FaceColorInfo> faceColors;
    for (auto& entry : shapes_) {
        if (entry.aisShape.IsNull()) continue;
        bool inSelection = false;
        for (auto& sel : selected) {
            if (entry.aisShape->Shape().IsSame(sel)) { inSelection = true; break; }
        }
        if (!inSelection) continue;
        for (auto& f : entry.faces) {
            if (f.useCustomColor)
                faceColors.push_back({f.topoFace, f.color[0], f.color[1], f.color[2]});
        }
    }

    if (selected.Size() == 1) {
        viewer()->exportShape(selected.First(), path,
            faceColors.empty() ? nullptr : &faceColors);
    } else {
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (auto& s : selected)
            builder.Add(compound, s);
        viewer()->exportShape(compound, path);
    }
    snprintf(statusMsg_, sizeof(statusMsg_), "Exported: %s", path);
}

// ─── Panels ─────────────────────────────────────────────────────────────────

void Viewer::drawObjectPanel() {
    ImGui::Begin("Objects", &showObjectPanel_);

    // ── View settings ──
    ImGui::Text("View");
    ImGui::Separator();

    // Display mode combo removed — AIS_ColoredShape handles both Model and Faces

    // Selection mode
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Select");
    ImGui::SameLine(80);
    ImGui::PushItemWidth(-1);
    const char* selModes[] = { "Shape", "Solid", "Face", "Edge", "Vertex" };
    int selVals[] = {
        AIS_Shape::SelectionMode(TopAbs_SHAPE),
        AIS_Shape::SelectionMode(TopAbs_SOLID),
        AIS_Shape::SelectionMode(TopAbs_FACE),
        AIS_Shape::SelectionMode(TopAbs_EDGE),
        AIS_Shape::SelectionMode(TopAbs_VERTEX)
    };
    int selCur = 0;
    for (int i = 0; i < 5; ++i) {
        if (selectionMode_ == selVals[i]) { selCur = i; break; }
    }
    if (ImGui::Combo("##selmode", &selCur, selModes, 5)) {
        selectionMode_ = selVals[selCur];
        int mode = selectionMode_;
        pendingActions_.push_back([mode](OcctViewer* v) {
            v->setSelectionMode(mode);
        });
        snprintf(statusMsg_, sizeof(statusMsg_), "Select: %s", selModes[selCur]);
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Scene Objects");
    ImGui::Separator();

    if (shapes_.empty()) {
        ImGui::TextDisabled("No objects in scene");
    }

    // Collect children per parent
    std::vector<std::vector<int>> kids(shapes_.size());
    for (int ci = 0; ci < (int)shapes_.size(); ++ci) {
        int p = shapes_[ci].parentIdx;
        if (p >= 0 && p < (int)kids.size()) kids[p].push_back(ci);
    }

    // Aggregate colors and visibility (bottom-up feedback)
    for (int i = 0; i < (int)shapes_.size(); ++i) {
        auto& e = shapes_[i];
        if (i < (int)kids.size() && !kids[i].empty()) {
            int nFaces=0; bool same=true;
            float c0r=0,c0g=0,c0b=0;
            bool anyChildVis=false;
            for (int ci : kids[i]) {
                if (shapes_[ci].visible) anyChildVis=true;
                for (auto& f : shapes_[ci].faces) {
                    if (nFaces==0) {c0r=f.color[0];c0g=f.color[1];c0b=f.color[2];}
                    else if (f.color[0]!=c0r||f.color[1]!=c0g||f.color[2]!=c0b) same=false;
                    nFaces++;
                }
            }
            if (nFaces>0 && same) {e.color[0]=c0r;e.color[1]=c0g;e.color[2]=c0b;}
            e.visible = anyChildVis;
        }
    }
    for (int i = 0; i < (int)shapes_.size(); ++i) {
        auto& ch = shapes_[i];
        if (!ch.faces.empty()) {
            bool same=true;
            auto& f0=ch.faces[0];
            bool anyFaceVis=false;
            for (auto& f : ch.faces) {
                if(f.visible) anyFaceVis=true;
                if(f.color[0]!=f0.color[0]||f.color[1]!=f0.color[1]||f.color[2]!=f0.color[2]) same=false;
            }
            if (same) {ch.color[0]=f0.color[0];ch.color[1]=f0.color[1];ch.color[2]=f0.color[2];}
            ch.visible = anyFaceVis;
        }
    }

    for (int i = 0; i < (int)shapes_.size(); ++i) {
        auto& entry = shapes_[i];
        if (entry.parentIdx >= 0) continue;
        ImGui::PushID(i);

        // ── Parent model row ──
        bool expanded = entry.facesExpanded;
        bool hasFaces = !entry.faces.empty();
        bool hasKids = i < (int)kids.size() && !kids[i].empty();
        if (hasFaces || hasKids) {
            ImGui::AlignTextToFramePadding();
            bool isExp = hasKids ? entry.childrenExpanded : expanded;
            if (ImGui::ArrowButton("##expand", isExp ?
                ImGuiDir_Down : ImGuiDir_Right)) {
                if (hasKids) entry.childrenExpanded = !isExp;
                else entry.facesExpanded = !isExp;
            }
            ImGui::SameLine();
        } else {
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
            ImGui::SameLine();
        }

        if (ImGui::Checkbox("##vis", &entry.visible)) {
            bool vis = entry.visible;
            for (auto& f : entry.faces) f.visible = vis;
            auto s = entry.aisShape;
            std::vector<int> cids;
            if (i < (int)kids.size()) cids = kids[i];
            for (int ci : cids) {
                shapes_[ci].visible = vis;
                for (auto& f : shapes_[ci].faces) f.visible = vis;
            }
            pendingActions_.push_back([this, s, cids, vis](OcctViewer* v) {
                if (!s.IsNull()) {
                    if (vis) v->displayShape(s, false);
                    else v->context()->Erase(s, false);
                }
                for (int ci : cids) {
                    auto& ch = shapes_[ci];
                    if (vis) v->displayShape(ch.aisShape, false);
                    else v->context()->Erase(ch.aisShape, false);
                }
            });
        }
        ImGui::SameLine();
        // Rainbow if faces differ, or if children differ
        bool rainbow = false;
        std::vector<std::reference_wrapper<FaceEntry>> allFacesForParent;
        if (hasKids) {
            for (int ci : kids[i])
                for (auto& f : shapes_[ci].faces)
                    allFacesForParent.push_back(f);
        } else {
            for (auto& f : entry.faces)
                allFacesForParent.push_back(f);
        }
        if (allFacesForParent.size() >= 2) {
            auto& f0 = allFacesForParent[0].get();
            float c0=f0.color[0],c1=f0.color[1],c2=f0.color[2];
            for (auto& fr : allFacesForParent) {
                auto& f = fr.get();
                if (f.color[0]!=c0||f.color[1]!=c1||f.color[2]!=c2){rainbow=true;break;}
            }
        }
        ImVec2 swatchPos = ImGui::GetCursorScreenPos();
        float swatchH = ImGui::GetFrameHeight();
        if (ImGui::ColorEdit3("##color", entry.color,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoBorder)) {
            float r = entry.color[0], g = entry.color[1], b = entry.color[2];
            std::vector<int> cidsToUpdate;
            if (hasKids) {
                for (int ci : kids[i]) cidsToUpdate.push_back(ci);
            } else {
                cidsToUpdate.push_back(i);
            }
            int parentI = i;
            pendingActions_.push_back([this, cidsToUpdate, r, g, b](OcctViewer* v) {
                for (int si : cidsToUpdate) {
                    auto& ent = shapes_[si];
                    for (auto& f : ent.faces) {
                        f.color[0]=r;f.color[1]=g;f.color[2]=b;f.useCustomColor=false;
                        setXcafFaceColor(ent, f.topoFace, r, g, b);
                    }
                    if (!ent.aisShape.IsNull())
                        v->context()->Redisplay(ent.aisShape, true);
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
                std::vector<int> cids;
                if (i < (int)kids.size()) cids = kids[i];
                pendingActions_.push_back([this, shape, cids](OcctViewer* v) {
                    if (!shape.IsNull()) v->removeShape(shape, true);
                    for (int ci : cids) {
                        auto& ch = shapes_[ci];
                        if (!ch.aisShape.IsNull()) v->removeShape(ch.aisShape, true);
                        ch.aisShape.Nullify(); ch.faces.clear();
                    }
                });
                // face removal handled by parent AIS_ColoredShape
                snprintf(statusMsg_, sizeof(statusMsg_), "Deleted '%s'", entry.name.c_str());
                entry.aisShape.Nullify();
                entry.faces.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // ── Expand: children (solids) ──
        if (hasKids && entry.childrenExpanded) {
                for (int ci : kids[i]) {
                    auto& ch = shapes_[ci];
                    bool chExp = ch.facesExpanded;
                    bool chHasFaces = !ch.faces.empty();
                    ImGui::PushID(10000 + ci);
                    ImGui::Indent(16.0f);
                    // Face expand arrow (matching single-solid layout)
                    if (chHasFaces) {
                        ImGui::AlignTextToFramePadding();
                        if (ImGui::ArrowButton("##cexp", chExp ? ImGuiDir_Down : ImGuiDir_Right))
                            ch.facesExpanded = !chExp;
                        ImGui::SameLine();
                    } else {
                        ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
                        ImGui::SameLine();
                    }
                    // Visibility
                    if (ImGui::Checkbox("##cvis", &ch.visible)) {
                        bool vis = ch.visible;
                        for (auto& f : ch.faces) f.visible = vis;
                        auto cs = ch.aisShape;
                        pendingActions_.push_back([cs, vis](OcctViewer* v) {
                            if (vis) v->displayShape(cs, false);
                            else v->context()->Erase(cs, false);
                        });
                    }
                    ImGui::SameLine();
                    // Color swatch + rainbow
                    bool cRainbow = false;
                    if (ch.faces.size() >= 2) {
                        auto& f0 = ch.faces[0];
                        for (auto& f : ch.faces)
                            if (f.color[0]!=f0.color[0]||f.color[1]!=f0.color[1]||f.color[2]!=f0.color[2])
                                {cRainbow=true;break;}
                    }
                    ImVec2 cSwPos = ImGui::GetCursorScreenPos();
                    float cSwH = ImGui::GetFrameHeight();
                    if (ImGui::ColorEdit3("##csc", ch.color,
                        ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_NoBorder)) {
                        float cr=ch.color[0],cg=ch.color[1],cb=ch.color[2];
                        int cii=ci;
                        pendingActions_.push_back([this,cii,cr,cg,cb](OcctViewer* v){
                            auto& ent=shapes_[cii];
                            for(auto& f:ent.faces){
                                f.color[0]=cr;f.color[1]=cg;f.color[2]=cb;f.useCustomColor=false;
                                setXcafFaceColor(ent,f.topoFace,cr,cg,cb);
                            }
                            v->context()->Redisplay(ent.aisShape,true);
                        });
                    }
                    if (cRainbow) {
                        ImGui::GetWindowDrawList()->AddRectFilled(cSwPos,ImVec2(cSwPos.x+cSwH,cSwPos.y+cSwH),IM_COL32(0,0,0,255),ImGui::GetStyle().FrameRounding);
                        ImU32 mc[4]={IM_COL32(0xFF,0x33,0x33,0xFF),IM_COL32(0x33,0x99,0xFF,0xFF),IM_COL32(0x33,0xCC,0x33,0xFF),IM_COL32(0xFF,0xCC,0x00,0xFF)};
                        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(cSwPos.x+2,cSwPos.y+2),ImVec2(cSwPos.x+cSwH-2,cSwPos.y+cSwH-2),mc[0],mc[1],mc[2],mc[3]);
                    }
                    ImGui::SameLine();
                    ImGui::TextUnformatted(ch.name.c_str());
                    // Face rows (matching single-solid face layout)
                    if (chExp) {
                        for (auto& f : ch.faces) {
                            ImGui::PushID(20000+f.id);
                            ImGui::Indent(16.0f);
                            char fl[32];snprintf(fl,sizeof(fl),"Face %d",f.id);
                            ImGui::TextUnformatted(fl);
                            ImGui::SameLine();ImGui::Checkbox("##fv2",&f.visible);
                            ImGui::SameLine();
                            if (ImGui::ColorEdit3("##fc2",f.color,ImGuiColorEditFlags_NoInputs|ImGuiColorEditFlags_NoLabel)){
                                f.useCustomColor=true;ch.color[0]=f.color[0];ch.color[1]=f.color[1];ch.color[2]=f.color[2];
                                float fr2=f.color[0],fg2=f.color[1],fb2=f.color[2];
                                int cii2=ci,fi2=f.id;
                                pendingActions_.push_back([this,cii2,fi2,fr2,fg2,fb2](OcctViewer*v){
                                    auto& ent2=shapes_[cii2];
                                    for(auto& ff:ent2.faces)if(ff.id==fi2){setXcafFaceColor(ent2,ff.topoFace,fr2,fg2,fb2);v->context()->Redisplay(ent2.aisShape,true);return;}
                                });
                            }
                            ImGui::Unindent(16.0f);ImGui::PopID();
                        }
                    }
                    ImGui::Unindent(16.0f);ImGui::PopID();
                }
            }

        // ── Expand: face sub-objects ──
        if (expanded) {
            for (auto& f : entry.faces) {
                ImGui::PushID(1000 + f.id);
                ImGui::AlignTextToFramePadding();
                ImGui::Indent(16.0f);
                char faceLabel[32];
                snprintf(faceLabel, sizeof(faceLabel), "Face %d", f.id);
                ImGui::TextUnformatted(faceLabel);
                ImGui::SameLine();
                ImGui::Checkbox("##fvis", &f.visible);
                ImGui::SameLine();
                if (ImGui::ColorEdit3("##fcolor", f.color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                    f.useCustomColor = true;
                    entry.color[0] = f.color[0];
                    entry.color[1] = f.color[1];
                    entry.color[2] = f.color[2];
                    float fr = f.color[0], fg = f.color[1], fb = f.color[2];
                    int si2 = i, fi = f.id;
                    pendingActions_.push_back([this, si2, fi, fr, fg, fb](OcctViewer* v) {
                        auto& ent = shapes_[si2];
                        for (auto& ff : ent.faces) {
                            if (ff.id == fi) {
                                setXcafFaceColor(ent, ff.topoFace, fr, fg, fb);
                                v->context()->Redisplay(ent.aisShape, true);
                                return;
                            }
                        }
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

    if (ImGui::Button("Fit All", ImVec2(-1, 0))) {
        fitAll();
        snprintf(statusMsg_, sizeof(statusMsg_), "Fit All");
    }

    if (ImGui::Button("Show All", ImVec2(-1, 0))) {
        for (auto& entry : shapes_) {
            if (entry.parentIdx >= 0) continue;
            if (entry.visible) continue;
            entry.visible = true;
            for (auto& f : entry.faces) f.visible = true;
            pendingActions_.push_back([shape = entry.aisShape](OcctViewer* v) {
                v->displayShape(shape, false);
            });
            if (entry.parentIdx < 0) {
                for (int ci : kids[&entry - shapes_.data()]) {
                    auto& ch = shapes_[ci];
                    ch.visible = true;
                    for (auto& f : ch.faces) f.visible = true;
                    pendingActions_.push_back([cs = ch.aisShape](OcctViewer* v) {
                        v->displayShape(cs, false);
                    });
                }
            }
        }
        snprintf(statusMsg_, sizeof(statusMsg_), "Show All");
    }

    if (ImGui::Button("Hide All", ImVec2(-1, 0))) {
        for (auto& entry : shapes_) {
            if (entry.parentIdx >= 0) continue;
            if (!entry.visible) continue;
            entry.visible = false;
            for (auto& f : entry.faces) f.visible = false;
            pendingActions_.push_back([shape = entry.aisShape](OcctViewer* v) {
                v->context()->Erase(shape, false);
            });
            int idx = (int)(&entry - shapes_.data());
            if (idx < (int)kids.size()) {
                for (int ci : kids[idx]) {
                    auto& ch = shapes_[ci];
                    ch.visible = false;
                    for (auto& f : ch.faces) f.visible = false;
                    pendingActions_.push_back([cs = ch.aisShape](OcctViewer* v) {
                        v->context()->Erase(cs, false);
                    });
                }
            }
        }
        snprintf(statusMsg_, sizeof(statusMsg_), "Hide All");
    }

    if (ImGui::Button("Delete All", ImVec2(-1, 0)))
        ImGui::OpenPopup("Delete All?");
    if (ImGui::BeginPopupModal("Delete All?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        int nRoots = 0;
        for (auto& e : shapes_) if (e.parentIdx < 0) nRoots++;
        ImGui::Text("Remove all %d shapes?", nRoots);
        if (ImGui::Button("Yes", ImVec2(80, 0))) {
            snprintf(statusMsg_, sizeof(statusMsg_), "Deleted %d shape(s)", (int)shapes_.size());
            for (auto& entry : shapes_) {
                // face removal handled by parent AIS_ColoredShape
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

    if (ImGui::Checkbox("Clipping", &sectionOn_)) {
        pendingActions_.push_back([this](OcctViewer*) { updateClipPlane(); });
        snprintf(statusMsg_, sizeof(statusMsg_), sectionOn_ ? "Clipping: On" : "Clipping: Off");
    }

    if (sectionOn_) {
        if (ImGui::Checkbox("Edit Clip", &clipEditOn_)) {
            pendingActions_.push_back([this](OcctViewer*) { updateClipPlane(); });
            snprintf(statusMsg_, sizeof(statusMsg_), clipEditOn_ ? "Edit Clip: On" : "Edit Clip: Off");
        }

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

void Viewer::extractFaces(ShapeEntry& entry, const TopoDS_Shape& shape,
                           float r, float g, float b,
                           double, double, double) {
    entry.faces.clear();
    int id = 0;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        entry.faces.emplace_back(face, id++, r, g, b);
    }
}

// ─── Helpers ────────────────────────────────────────────────────────────────

Handle(AIS_ColoredShape) Viewer::makeColoredShape(const TopoDS_Shape& shape,
                                                  float r, float g, float b,
                                                  double tx, double ty, double tz,
                                                  Handle(TDocStd_Document)* outDoc,
                                                  TDF_Label* outShapeLabel) {
    Handle(TDocStd_Document) doc = new TDocStd_Document("MDTV-XCAF");
    Handle(XCAFDoc_ShapeTool) st = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_Label shapeLabel = st->AddShape(shape);
    XCAFDoc_VisMaterialCommon cm;
    cm.DiffuseColor  = Quantity_Color(r, g, b, Quantity_TOC_RGB);
    cm.AmbientColor  = Quantity_Color(r*0.2f, g*0.2f, b*0.2f, Quantity_TOC_RGB);
    cm.SpecularColor = Quantity_Color(0.5f, 0.5f, 0.5f, Quantity_TOC_RGB);
    cm.Shininess     = 0.6f;
    Handle(XCAFDoc_VisMaterial) mat = new XCAFDoc_VisMaterial();
    mat->SetCommonMaterial(cm);
    Handle(XCAFDoc_VisMaterialTool) mt =
        XCAFDoc_DocumentTool::VisMaterialTool(doc->Main());
    TDF_Label matLabel = mt->AddMaterial(mat, "Default");
    mt->SetShapeMaterial(shapeLabel, matLabel);

    Handle(AIS_ColoredShape) ais = new XCAFPrs_AISObject(shapeLabel);
    ais->Attributes()->SetFaceBoundaryDraw(true);
    ais->Attributes()->SetFaceBoundaryAspect(
        new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0));
    if (tx != 0 || ty != 0 || tz != 0) {
        gp_Trsf trsf;
        trsf.SetTranslation(gp_Vec(tx, ty, tz));
        ais->SetLocalTransformation(trsf);
    }
    if (outDoc) *outDoc = doc;
    if (outShapeLabel) *outShapeLabel = shapeLabel;
    return ais;
}

} // namespace OcctImGui
