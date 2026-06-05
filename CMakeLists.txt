###############################################################################
# CMake options
###############################################################################
cmake_minimum_required(VERSION 3.20)

project(occt-imgui VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

###############################################################################
# CMake module path
###############################################################################
list(PREPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_LIST_DIR}/cmake)

###############################################################################
# Set variables
###############################################################################
set(OpenCASCADE_DIR "$ENV{OpenCASCADE_DIR}" CACHE PATH "Directory where OpenCASCADE is installed.")

###############################################################################
# Dependencies
###############################################################################
include(Dependencies)

###############################################################################
# occt-imgui library
###############################################################################
set(LIB_SOURCES
  src/Application.cpp
  src/OcctViewer.cpp
  src/OcctWindow.cpp
  src/OcctImGui.cpp
)

set(LIB_HEADERS
  src/OcctImConfig.h
  src/Application.h
  src/OcctViewer.h
  src/OcctWindow.h
  src/OcctImGui.h
)

add_library(OcctImGui STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_compile_definitions(OcctImGui PRIVATE
  FONTS_DIR="${CMAKE_SOURCE_DIR}/src/fonts"
)

add_library(glad STATIC ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)

target_include_directories(OcctImGui PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/third_party/imoguizmo
)

target_link_libraries(OcctImGui PUBLIC
  imgui
  glfw
  glad
  OpenCASCADE::OpenCASCADE
)

if(APPLE)
  target_link_libraries(OcctImGui PUBLIC
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
  )
endif()

if(MSVC)
  target_compile_options(OcctImGui PRIVATE /EHsc)
else()
  target_compile_options(OcctImGui PRIVATE
    -fexceptions
    -Wno-elaborated-enum-base
  )
endif()

###############################################################################
# Demo executable
###############################################################################
add_executable(OcctImGuiDemo src/main.cpp)
target_link_libraries(OcctImGuiDemo PRIVATE OcctImGui)

###############################################################################
# Tests
###############################################################################
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
