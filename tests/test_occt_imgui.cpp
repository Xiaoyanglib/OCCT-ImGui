#include "OcctImGui.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopoDS_Shape.hxx>
#include <gtest/gtest.h>

namespace {

TEST(OcctImGuiTest, LibraryLinks) {
    OcctImGui::Viewer viewer;
    SUCCEED();
}

TEST(OcctImGuiTest, AddShapeBeforeShow) {
    OcctImGui::Viewer viewer;
    TopoDS_Shape box = BRepPrimAPI_MakeBox(10, 20, 30).Shape();
    viewer.addShape(box, 0.3f, 0.6f, 0.5f, "TestBox");
    // Should not crash — shapes are buffered before show()
    SUCCEED();
}

TEST(OcctImGuiTest, AddShapeAutoName) {
    OcctImGui::Viewer viewer;
    TopoDS_Shape s = BRepPrimAPI_MakeSphere(15).Shape();
    viewer.addShape(s, 0.8f, 0.2f, 0.2f);
    // nullptr name → auto-generated
    SUCCEED();
}

TEST(OcctImGuiTest, AddMultipleShapes) {
    OcctImGui::Viewer viewer;
    viewer.addShape(BRepPrimAPI_MakeBox(10, 10, 10).Shape(),      1, 0, 0, "Red");
    viewer.addShape(BRepPrimAPI_MakeSphere(10).Shape(),           0, 1, 0, "Green");
    viewer.addShape(BRepPrimAPI_MakeBox(20, 5, 5).Shape(),        0, 0, 1, "Blue");
    viewer.addShape(BRepPrimAPI_MakeSphere(5).Shape(),            0.5f, 0.5f, 0.5f);
    SUCCEED();
}

TEST(OcctImGuiTest, FitAll) {
    OcctImGui::Viewer viewer;
    viewer.fitAll();
    // Queued as pending action, should not crash
    SUCCEED();
}

} // namespace
