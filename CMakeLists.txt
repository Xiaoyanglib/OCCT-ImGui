###############################################################################
# CMake options
###############################################################################
cmake_minimum_required(VERSION 3.20)

project(occt-imgui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
# occt-imgui executable
###############################################################################
set(APP_SOURCES
  src/main.cpp
  src/Application.cpp
  src/OcctViewer.cpp
  src/OcctWindow.cpp
)

set(APP_HEADERS
  src/OcctImConfig.h
  src/Application.h
  src/OcctViewer.h
  src/OcctWindow.h
)

add_executable(occt-imgui ${APP_SOURCES} ${APP_HEADERS})


target_compile_definitions(occt-imgui PRIVATE
  FONTS_DIR="${CMAKE_SOURCE_DIR}/src/fonts"
)

target_include_directories(occt-imgui PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(occt-imgui PRIVATE
  imgui
  glfw
  OpenCASCADE::OpenCASCADE
)

if(APPLE)
  target_link_libraries(occt-imgui PRIVATE
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
  )
endif()

if(MSVC)
  target_compile_options(occt-imgui PRIVATE /EHsc)
else()
  target_compile_options(occt-imgui PRIVATE
    -fexceptions
    -Wno-elaborated-enum-base
  )
endif()
