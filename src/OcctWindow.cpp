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
