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

#include <Aspect_RenderingContext.hxx>
#include <Aspect_Window.hxx>

struct GLFWwindow;

/// GLFW-to-OCCT window bridge. Implements OCCT's Aspect_Window interface
/// so the 3D view can render into a GLFW-managed OpenGL context.
class OcctWindow : public Aspect_Window
{
    DEFINE_STANDARD_RTTI_INLINE(OcctWindow, Aspect_Window)
public:
    OcctWindow(GLFWwindow* theWin, int theWidth, int theHeight);
    virtual ~OcctWindow() {}

    /// Update the window size in framebuffer pixels.
    void updateSize(int w, int h);

    Aspect_RenderingContext NativeGlContext() const;

    Aspect_Drawable NativeHandle() const override;
    Aspect_Drawable NativeParentHandle() const override { return 0; }

    /// Called by OCCT when the window needs to be resized.
    Aspect_TypeOfResize DoResize() override;

    bool IsMapped() const override { return true; }
    bool DoMapping() const override { return true; }
    void Map() const override {}
    void Unmap() const override {}

    void Position(int& theX1, int& theY1,
                  int& theX2, int& theY2) const override
    {
        theX1 = myXLeft; theX2 = myXRight; theY1 = myYTop; theY2 = myYBottom;
    }

    double Ratio() const override
    {
        return double(myXRight - myXLeft) / double(myYBottom - myYTop);
    }

    void Size(int& theWidth, int& theHeight) const override
    {
        theWidth = myXRight - myXLeft; theHeight = myYBottom - myYTop;
    }

    Aspect_FBConfig NativeFBConfig() const override { return nullptr; }

private:
    GLFWwindow* myWindow;
    int myXLeft = 0, myYTop = 0;
    int myXRight = 0, myYBottom = 0;
};
