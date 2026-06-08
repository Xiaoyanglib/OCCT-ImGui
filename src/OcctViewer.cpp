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

#ifdef __APPLE__
#ifndef __HISERVICES__
#define __HISERVICES__ 0
#endif
#endif

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "OcctViewer.h"
#include "OcctWindow.h"

#if defined(__APPLE__)
  #define GLFW_EXPOSE_NATIVE_COCOA
  #define GLFW_EXPOSE_NATIVE_NSGL
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#if defined(__APPLE__)
  #define GL_SILENCE_DEPRECATION
  #include <OpenGL/gl3.h>
#endif

#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#if defined(_WIN32)
  #include <commdlg.h>
#endif

#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <IGESControl_Controller.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <StlAPI_Reader.hxx>
#include <StlAPI_Writer.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Graphic3d_Camera.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <gp_Pln.hxx>

#include <cstdio>
#include <cstring>
#include <cmath>

// ─── OcctViewer ────────────────────────────────────────────────────────────

OcctViewer::OcctViewer(GLFWwindow* glfwWindow)
    : glfwWindow_(glfwWindow) {

    int fw, fh;
    glfwGetFramebufferSize(glfwWindow, &fw, &fh);

    Handle(OcctWindow) win = new OcctWindow(glfwWindow, fw, fh);
    nativeWin_ = win;

    Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(display, false);
    driver->SetBuffersNoSwap(true);

    viewer_ = new V3d_Viewer(driver);
    viewer_->SetDefaultLights();
    viewer_->SetLightOn();

    context_ = new AIS_InteractiveContext(viewer_);

    context_->HighlightStyle()->SetColor(Quantity_NOC_CYAN1);
    context_->SelectionStyle()->SetColor(Quantity_NOC_CYAN1);

    view_ = viewer_->CreateView();
    view_->SetWindow(win, win->NativeGlContext());
    view_->MustBeResized();

    view_->SetBackgroundColor(Quantity_NOC_WHITE);
    view_->MustBeResized();
    view_->Camera()->SetEyeAndCenter(gp_Pnt(200, 150, 200), gp_Pnt(0, 0, 0));
    view_->Camera()->SetUp(gp_Dir(0, 0, 1));
}

OcctViewer::~OcctViewer() {
    context_.Nullify();
    view_.Nullify();
    viewer_.Nullify();
    nativeWin_.Nullify();
}

void OcctViewer::redraw() {
    if (view_.IsNull()) return;

#if defined(__APPLE__)
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
#endif

    view_->Redraw();

#if defined(__APPLE__)
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);
#endif
}

void OcctViewer::updateViewer() {
    if (!context_.IsNull())
        context_->UpdateCurrentViewer();
}

void OcctViewer::resize(int width, int height) {
    if (view_.IsNull() || nativeWin_.IsNull()) return;
    Handle(OcctWindow)::DownCast(nativeWin_)->updateSize(width, height);
    view_->MustBeResized();
}

void OcctViewer::startRotation(int x, int y) {
    if (view_.IsNull()) return;
    view_->StartRotation(x, y);
}

void OcctViewer::rotation(int x, int y) {
    if (view_.IsNull()) return;
    view_->Rotation(x, y);
}

void OcctViewer::zoom(double factor) {
    if (view_.IsNull()) return;
    auto cam = view_->Camera();
    cam->SetScale(cam->Scale() * factor);
    view_->Redraw();
}

void OcctViewer::pan(int x, int y) {
    if (view_.IsNull()) return;
    view_->Pan(x, y);
}

void OcctViewer::fitAll() {
    if (view_.IsNull()) return;
    view_->FitAll();
    view_->Redraw();
}

void OcctViewer::displayShape(const Handle(AIS_Shape)& shape, bool update) {
    if (context_.IsNull()) return;
    context_->Display(shape, update);
}

void OcctViewer::removeShape(const Handle(AIS_Shape)& shape, bool update) {
    if (context_.IsNull()) return;
    context_->Remove(shape, update);
}

void OcctViewer::setShadedWithEdges() {
    if (context_.IsNull()) return;
    context_->SetDisplayMode(AIS_Shaded, true);
    context_->DefaultDrawer()->SetFaceBoundaryDraw(true);
    if (!view_.IsNull()) view_->Redraw();
}

void OcctViewer::moveTo(int x, int y) {
    if (context_.IsNull() || view_.IsNull()) return;
    context_->MoveTo(x, y, view_, true);
}

void OcctViewer::deselectOrClear() {
    if (context_.IsNull()) return;
    context_->InitDetected();
    if (!context_->MoreDetected()) { context_->ClearSelected(true); return; }
    Handle(SelectMgr_EntityOwner) owner = context_->DetectedCurrentOwner();
    if (owner.IsNull()) { context_->ClearSelected(true); return; }
    bool alreadySelected = false;
    for (context_->InitSelected(); context_->MoreSelected(); context_->NextSelected()) {
        if (context_->SelectedOwner() == owner) { alreadySelected = true; break; }
    }
    if (alreadySelected) context_->AddOrRemoveSelected(owner, false);
    else { context_->ClearSelected(false); context_->AddOrRemoveSelected(owner, true); }
}

bool OcctViewer::shiftSelect(int, int) {
    if (context_.IsNull() || view_.IsNull()) return false;
    context_->SelectDetected(AIS_SelectionScheme_XOR);
    context_->InitSelected();
    return context_->MoreSelected();
}

void OcctViewer::selectRectangle(int x1, int y1, int x2, int y2) {
    if (context_.IsNull() || view_.IsNull()) return;
    context_->SelectRectangle(NCollection_Vec2<int>(std::min(x1, x2), std::min(y1, y2)),
                              NCollection_Vec2<int>(std::max(x1, x2), std::max(y1, y2)),
                              view_);
}

void OcctViewer::shiftSelectRectangle(int x1, int y1, int x2, int y2) {
    if (context_.IsNull() || view_.IsNull()) return;
    context_->SelectRectangle(NCollection_Vec2<int>(std::min(x1, x2), std::min(y1, y2)),
                              NCollection_Vec2<int>(std::max(x1, x2), std::max(y1, y2)),
                              view_, AIS_SelectionScheme_XOR);
}

void OcctViewer::clearSelection() {
    if (context_.IsNull()) return;
    context_->ClearSelected(true);
}

void OcctViewer::setSelectionMode(int mode) {
    if (context_.IsNull()) return;
    context_->Deactivate();
    context_->Activate(mode);
}

void OcctViewer::setBackgroundColor(float r, float g, float b) {
    if (view_.IsNull()) return;
    view_->SetBackgroundColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
    view_->Redraw();
}

void OcctViewer::setOrthographic(bool ortho) {
    if (view_.IsNull()) return;
    auto cam = view_->Camera();
    if (ortho)
        cam->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
    else
        cam->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
    view_->Redraw();
}

// ─── Camera matrix helpers ────────────────────────────────────────────

void OcctViewer::getCameraMatrices(float viewMat[16], float projMat[16]) const {
    if (view_.IsNull()) return;
    auto cam = view_->Camera();
    if (cam.IsNull()) return;

    // Build a standard OpenGL lookAt view matrix from the camera's eye/center/up.
    // We build this manually (rather than using OCCT's OrientationMatrixF()) because
    // OCCT uses a non-standard translation convention that breaks the round-trip,
    // and the camera scale baked into OrientationMatrixF() amplifies axis projections.
    gp_Pnt eyeP = cam->Eye();
    gp_Pnt ctrP = cam->Center();
    gp_Dir upDir = cam->Up();

    float fx = static_cast<float>(ctrP.X() - eyeP.X());
    float fy = static_cast<float>(ctrP.Y() - eyeP.Y());
    float fz = static_cast<float>(ctrP.Z() - eyeP.Z());
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);
    if (flen > 1e-10f) { fx /= flen; fy /= flen; fz /= flen; }

    float ux = static_cast<float>(upDir.X());
    float uy = static_cast<float>(upDir.Y());
    float uz = static_cast<float>(upDir.Z());

    // right = normalize(cross(f, up))
    float rx = fy * uz - fz * uy;
    float ry = fz * ux - fx * uz;
    float rz = fx * uy - fy * ux;
    float rlen = sqrtf(rx * rx + ry * ry + rz * rz);
    if (rlen > 1e-10f) { rx /= rlen; ry /= rlen; rz /= rlen; }

    // re-orthogonalize up = cross(right, f)
    ux = ry * fz - rz * fy;
    uy = rz * fx - rx * fz;
    uz = rx * fy - ry * fx;

    float ex = static_cast<float>(eyeP.X());
    float ey = static_cast<float>(eyeP.Y());
    float ez = static_cast<float>(eyeP.Z());

    // Row-major OpenGL view matrix (RHS, camera looks along -Z in view space)
    viewMat[0]  = rx;  viewMat[4]  = ry;  viewMat[8]  = rz;  viewMat[12] = -(rx * ex + ry * ey + rz * ez);
    viewMat[1]  = ux;  viewMat[5]  = uy;  viewMat[9]  = uz;  viewMat[13] = -(ux * ex + uy * ey + uz * ez);
    viewMat[2]  = -fx; viewMat[6]  = -fy; viewMat[10] = -fz; viewMat[14] =  (fx * ex + fy * ey + fz * ez);
    viewMat[3]  = 0;   viewMat[7]  = 0;   viewMat[11] = 0;   viewMat[15] = 1;

    // Build projection matrix manually — OCCT's ProjectionMatrixF() uses a
    // non-standard FOV that causes axis projections to collapse.
    // Use aspect=1 since the gizmo draws in a 120x120 square — the main viewport's
    // non-square aspect would otherwise make the aspect correction in the gizmo
    // library apply unevenly to X vs Y axis projection (only VP[0] and VP[8] get
    // the aspect multiplication, leaving VP[4] (Y-axis X-component) uncorrected).
    float fov = static_cast<float>(cam->FOVy());
    if (fov < 0.01f || fov > 3.0f) fov = 45.0f * 3.14159265f / 180.0f;
    float f = 1.0f / tanf(fov * 0.5f);
    float zn = 0.1f, zf = 10000.0f;

    // Row-major perspective projection matrix (square aspect for the gizmo)
    projMat[0] = f;       projMat[4] = 0;           projMat[8]  = 0;                         projMat[12] = 0;
    projMat[1] = 0;       projMat[5] = f;           projMat[9]  = 0;                         projMat[13] = 0;
    projMat[2] = 0;       projMat[6] = 0;           projMat[10] = -(zf + zn) / (zf - zn);    projMat[14] = -1;
    projMat[3] = 0;       projMat[7] = 0;           projMat[11] = -(2 * zf * zn) / (zf - zn); projMat[15] = 0;
}

void OcctViewer::setCameraFromViewMatrix(const float viewMat[16]) {
    if (view_.IsNull()) return;
    auto cam = view_->Camera();
    if (cam.IsNull()) return;

    // The view matrix is row-major OpenGL RHS lookAt matching our getCameraMatrices:
    //   col0 = right       → viewMat[0],  viewMat[4],  viewMat[8]
    //   col1 = up          → viewMat[1],  viewMat[5],  viewMat[9]
    //   col2 = -forward    → viewMat[2],  viewMat[6],  viewMat[10]
    //   col3 = translation → viewMat[12], viewMat[13], viewMat[14]
    // where translation = -R * eye (R = [right | up | -forward])
    // eye = -R^T * translation

    // The imoguizmo library stores the view matrix in a row-major layout where
    // each ROW contains one basis vector: row0=right, row1=up, row2=-forward.
    // The translation is in row3 (elements 12-14). So we extract eye by dotting
    // each basis vector with the translation — using elements on the SAME row
    // (not same column) since that's how the gizmo's lookAt builds the matrix.
    float tx = viewMat[12], ty = viewMat[13], tz = viewMat[14];
    gp_Pnt eye(
        -(viewMat[0] * tx + viewMat[1] * ty + viewMat[2]  * tz),
        -(viewMat[4] * tx + viewMat[5] * ty + viewMat[6]  * tz),
        -(viewMat[8] * tx + viewMat[9] * ty + viewMat[10] * tz));

    // Forward direction (eye → center): -column2
    gp_Dir forward(-viewMat[2], -viewMat[6], -viewMat[10]);
    // Up direction: column1
    gp_Dir up(viewMat[1], viewMat[5], viewMat[9]);

    double dist = cam->Distance();
    gp_Pnt center(eye.X() + forward.X() * dist,
                  eye.Y() + forward.Y() * dist,
                  eye.Z() + forward.Z() * dist);

    cam->SetEyeAndCenter(eye, center);
    cam->SetUp(up);
    view_->Redraw();
}

static const char* fileExt(const char* path) {
    const char* e = strrchr(path, '.');
    return e ? e : "";
}

static bool extIs(const char* ext, const char* a, const char* b = nullptr) {
#if defined(_WIN32)
    return _stricmp(ext, a) == 0 || (b && _stricmp(ext, b) == 0);
#else
    return strcasecmp(ext, a) == 0 || (b && strcasecmp(ext, b) == 0);
#endif
}

TopoDS_Shape OcctViewer::importShape(const char* path) {
    TopoDS_Shape result;
    const char* ext = fileExt(path);
    if (!*ext) return result;

    if (extIs(ext, ".step", ".stp")) {
        STEPControl_Reader reader;
        if (reader.ReadFile(path) != IFSelect_RetDone) return result;
        reader.TransferRoots();
        result = reader.OneShape();
    } else if (extIs(ext, ".iges", ".igs")) {
        IGESControl_Reader reader;
        if (reader.ReadFile(path) != IFSelect_RetDone) return result;
        reader.TransferRoots();
        result = reader.OneShape();
    } else if (extIs(ext, ".brep")) {
        BRep_Builder builder;
        BRepTools::Read(result, path, builder);
    } else if (extIs(ext, ".stl")) {
        StlAPI_Reader reader;
        reader.Read(result, path);
    }
    return result;
}

bool OcctViewer::exportShape(const TopoDS_Shape& shape, const char* path) {
    if (shape.IsNull()) return false;
    const char* ext = fileExt(path);
    if (!*ext) return false;

    if (extIs(ext, ".step", ".stp")) {
        STEPControl_Writer writer;
        if (!writer.Transfer(shape, STEPControl_AsIs)) return false;
        return writer.Write(path) == IFSelect_RetDone;
    } else if (extIs(ext, ".iges", ".igs")) {
        IGESControl_Controller::Init();
        IGESControl_Writer writer;
        if (!writer.AddShape(shape)) return false;
        writer.ComputeModel();
        return writer.Write(path);
    } else if (extIs(ext, ".brep")) {
        return BRepTools::Write(shape, path);
    } else if (extIs(ext, ".stl")) {
        StlAPI_Writer writer;
        return writer.Write(shape, path);
    }
    return false;
}

void OcctViewer::getSelectedShapes(NCollection_List<TopoDS_Shape>& outList) {
    if (context_.IsNull()) return;
    for (context_->InitSelected(); context_->MoreSelected(); context_->NextSelected()) {
        TopoDS_Shape s = context_->SelectedShape();
        if (!s.IsNull()) outList.Append(s);
    }
}

// ─── Clip / section planes ─────────────────────────────────────────────────

void OcctViewer::addClipPlane(int id, const gp_Pln& plane) {
    if (view_.IsNull()) return;
    Handle(Graphic3d_ClipPlane) clip = new Graphic3d_ClipPlane(plane);
    clip->SetOn(true);
    clip->SetCapping(true);
    clip->CappingAspect()->ChangeFrontMaterial().SetTransparency(1.0f);
    view_->AddClipPlane(clip);
    clipPlanes_.Bind(id, clip);
    view_->Redraw();
}

void OcctViewer::removeClipPlane(int id) {
    if (view_.IsNull()) return;
    Handle(Graphic3d_ClipPlane) clip;
    if (clipPlanes_.Find(id, clip)) {
        clip->SetOn(false);
        view_->RemoveClipPlane(clip);
        clipPlanes_.UnBind(id);
        view_->Redraw();
    }
}

void OcctViewer::setClipPlaneEquation(int id, const gp_Pln& plane) {
    Handle(Graphic3d_ClipPlane) clip;
    if (clipPlanes_.Find(id, clip)) {
        clip->SetEquation(plane);
        view_->Redraw();
    }
}

void OcctViewer::setClipPlaneOn(int id, bool on) {
    Handle(Graphic3d_ClipPlane) clip;
    if (clipPlanes_.Find(id, clip)) {
        clip->SetOn(on);
        view_->Redraw();
    }
}

void OcctViewer::setClipPlaneCapping(int id, bool enable, Quantity_Color color, float alpha) {
    Handle(Graphic3d_ClipPlane) clip;
    if (clipPlanes_.Find(id, clip)) {
        clip->SetCapping(enable);
        clip->SetCappingColor(color);
        if (alpha > 0.0f)
            clip->CappingAspect()->ChangeFrontMaterial().SetTransparency(alpha);
        view_->Redraw();
    }
}

bool OcctViewer::hasClipPlane(int id) const {
    return clipPlanes_.IsBound(id);
}

void OcctViewer::getBoundingBox(double* xmin, double* ymin, double* zmin,
                                 double* xmax, double* ymax, double* zmax) const {
    if (context_.IsNull()) {
        *xmin = *ymin = *zmin = 0;
        *xmax = *ymax = *zmax = 100;
        return;
    }
    Bnd_Box box;
    NCollection_List<Handle(AIS_InteractiveObject)> objs;
    context_->DisplayedObjects(objs);
    for (auto& obj : objs) {
        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
        if (!aisShape.IsNull())
            BRepBndLib::Add(aisShape->Shape(), box);
    }
    if (box.IsVoid()) {
        *xmin = *ymin = *zmin = 0;
        *xmax = *ymax = *zmax = 100;
        return;
    }
    box.Get(*xmin, *ymin, *zmin, *xmax, *ymax, *zmax);
}

// ─── File dialogs ─────────────────────────────────────────────────────────

#if defined(_WIN32)

#include <windows.h>
#include <commdlg.h>

static const wchar_t* fileFilter = L"CAD Files\0*.step;*.stp;*.iges;*.igs;*.brep;*.stl\0All\0*.*\0";

static void toWide(const char* src, wchar_t* dst, int len) {
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, len);
}
static void toNarrow(const wchar_t* src, char* dst, int len) {
    WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, len, nullptr, nullptr);
}

bool OcctViewer::openFileDialog(char* outPath, int maxLen) {
    OPENFILENAMEW ofn = {};
    wchar_t buf[1024] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = fileFilter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = 1024;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) { toNarrow(buf, outPath, maxLen); return true; }
    return false;
}

bool OcctViewer::saveFileDialog(char* outPath, int maxLen) {
    OPENFILENAMEW ofn = {};
    wchar_t buf[1024] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = fileFilter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = 1024;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) { toNarrow(buf, outPath, maxLen); return true; }
    return false;
}

#elif defined(__APPLE__)

#include <cstdio>

bool OcctViewer::openFileDialog(char* outPath, int maxLen) {
    FILE* fp = popen(
        "osascript -e 'POSIX path of (choose file "
        "with prompt \"Open CAD File\" "
        "of type {\"public.data\"})' 2>/dev/null",
        "r");
    if (!fp) return false;
    char* ok = fgets(outPath, maxLen, fp);
    pclose(fp);
    if (!ok) return false;
    size_t len = strlen(outPath);
    while (len > 0 && (outPath[len-1] == '\n' || outPath[len-1] == '\r'))
        outPath[--len] = '\0';
    return len > 0;
}

bool OcctViewer::saveFileDialog(char* outPath, int maxLen) {
    FILE* fp = popen(
        "osascript -e 'POSIX path of (choose file name "
        "with prompt \"Save CAD File\")' 2>/dev/null",
        "r");
    if (!fp) return false;
    char* ok = fgets(outPath, maxLen, fp);
    pclose(fp);
    if (!ok) return false;
    size_t len = strlen(outPath);
    while (len > 0 && (outPath[len-1] == '\n' || outPath[len-1] == '\r'))
        outPath[--len] = '\0';
    return len > 0;
}

#else

bool OcctViewer::openFileDialog(char*, int) { return false; }
bool OcctViewer::saveFileDialog(char*, int) { return false; }

#endif
