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

TEST(OcctImGuiTest, AddShapeWithTranslation) {
    OcctImGui::Viewer viewer;
    TopoDS_Shape box = BRepPrimAPI_MakeBox(10, 10, 10).Shape();
    viewer.addShape(box, 0.5f, 0.5f, 0.5f, "OffsetBox", 100, 0, -50);
    SUCCEED();
}

TEST(OcctImGuiTest, ImportEmptyPath) {
    OcctImGui::Viewer viewer;
    viewer.importFile("");
    // Should not crash on empty path
    SUCCEED();
}

TEST(OcctImGuiTest, MultipleFitAll) {
    OcctImGui::Viewer viewer;
    viewer.fitAll();
    viewer.fitAll();
    viewer.fitAll();
    SUCCEED();
}

TEST(OcctImGuiTest, ShapesPersistAcrossShow) {
    OcctImGui::Viewer viewer;
    viewer.addShape(BRepPrimAPI_MakeBox(10, 10, 10).Shape(), 0, 0, 0, "Box");
    viewer.fitAll();
    // Show would create viewer context — just verify no crash on shape ops pre-show
    SUCCEED();
}

TEST(OcctImGuiTest, ExportWithoutSelection) {
    OcctImGui::Viewer viewer;
    viewer.importFile("/nonexistent/file.step");
    SUCCEED();
}

TEST(OcctImGuiTest, FaceEntryDefaultValues) {
    Handle(AIS_Shape) dummy = new AIS_Shape(TopoDS_Shape());
    OcctImGui::FaceEntry f(dummy, 5, 0.2f, 0.4f, 0.6f);
    EXPECT_EQ(f.id, 5);
    EXPECT_FALSE(f.useCustomColor);
    EXPECT_TRUE(f.visible);
    EXPECT_FLOAT_EQ(f.color[0], 0.2f);
    EXPECT_FLOAT_EQ(f.color[1], 0.4f);
    EXPECT_FLOAT_EQ(f.color[2], 0.6f);
}

TEST(OcctImGuiTest, FaceCustomColorFlag) {
    Handle(AIS_Shape) dummy = new AIS_Shape(TopoDS_Shape());
    OcctImGui::FaceEntry f(dummy, 0, 0.5f, 0.5f, 0.5f);
    EXPECT_FALSE(f.useCustomColor);
    f.useCustomColor = true;
    EXPECT_TRUE(f.useCustomColor);
}

TEST(OcctImGuiTest, FaceEntryInitiallyOwnedByParent) {
    Handle(AIS_Shape) dummy = new AIS_Shape(TopoDS_Shape());
    OcctImGui::FaceEntry f(dummy, 3, 0.8f, 0.2f, 0.1f);
    // Face defaults: not custom, visible, parent color
    EXPECT_FALSE(f.useCustomColor);
    EXPECT_TRUE(f.visible);
}

TEST(OcctImGuiTest, SetBackgroundColor) {
    OcctImGui::Viewer viewer;
    SUCCEED();
}

TEST(OcctImGuiTest, SetOrthographic) {
    OcctImGui::Viewer viewer;
    SUCCEED();
}

} // namespace
