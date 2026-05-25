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

#include <cstdio>
#include <cstring>

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
    view_->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.08);

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

// ─── Import / Export ──────────────────────────────────────────────────────

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
