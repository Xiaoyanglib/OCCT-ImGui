if(NOT TARGET imgui)
  include(FetchContent)

  cmake_policy(SET CMP0169 OLD)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/third_party/imgui
  )
  FetchContent_Populate(imgui)

  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )

  target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )

  target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="OcctImConfig.h"
    GLFW_INCLUDE_NONE
  )

  target_link_libraries(imgui PUBLIC glfw)
endif()
