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

#include "OcctWindow.h"

#if defined(__APPLE__)
  #define GLFW_EXPOSE_NATIVE_COCOA
  #define GLFW_EXPOSE_NATIVE_NSGL
#elif defined(_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
  #define GLFW_EXPOSE_NATIVE_WGL
#else
  #define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

OcctWindow::OcctWindow(GLFWwindow* theWin, int theWidth, int theHeight)
    : myWindow(theWin), myXLeft(0), myYTop(0),
      myXRight(theWidth), myYBottom(theHeight)
{
    if (myWindow) {
        int x, y;
        glfwGetWindowPos(myWindow, &x, &y);
        myXLeft  = x;
        myYTop   = y;
        myXRight = x + theWidth;
        myYBottom = y + theHeight;
    }
}

void OcctWindow::updateSize(int w, int h) {
    myXRight = myXLeft + w;
    myYBottom = myYTop + h;
}

Aspect_RenderingContext OcctWindow::NativeGlContext() const {
#if defined(__APPLE__)
    return (Aspect_RenderingContext)glfwGetNSGLContext(myWindow);
#elif defined(_WIN32)
    return glfwGetWGLContext(myWindow);
#else
    return glfwGetGLXContext(myWindow);
#endif
}

Aspect_Drawable OcctWindow::NativeHandle() const {
#if defined(__APPLE__)
    return (Aspect_Drawable)glfwGetCocoaWindow(myWindow);
#elif defined(_WIN32)
    return (Aspect_Drawable)glfwGetWin32Window(myWindow);
#else
    return (Aspect_Drawable)glfwGetX11Window(myWindow);
#endif
}

Aspect_TypeOfResize OcctWindow::DoResize() {
    if (myWindow && glfwGetWindowAttrib(myWindow, GLFW_VISIBLE)) {
        int x, y, w, h;
        glfwGetWindowPos(myWindow, &x, &y);
        glfwGetWindowSize(myWindow, &w, &h);
        myXLeft  = x;
        myYTop   = y;
        myXRight = x + w;
        myYBottom = y + h;
    }
    return Aspect_TOR_UNKNOWN;
}
