###############################################################################
# CMake options
###############################################################################
cmake_minimum_required(VERSION 3.20)

project(occt-imgui VERSION 0.1.0 LANGUAGES C CXX)

option(BUILD_TESTS "Build tests" ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # standalone
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug or Release)" FORCE)
  endif()
else()
  # as submodule
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Force Release build" FORCE)
  set(BUILD_TESTS OFF)
endif()
message(STATUS CMAKE_BUILD_TYPE:${CMAKE_BUILD_TYPE})

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
add_subdirectory(src)
add_subdirectory(third_party)

###############################################################################
# Demo executable
###############################################################################
add_subdirectory(app)

###############################################################################
# Tests
###############################################################################
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
