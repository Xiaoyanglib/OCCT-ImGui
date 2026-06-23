add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/googletest ${CMAKE_BINARY_DIR}/googletest
  EXCLUDE_FROM_ALL)

add_executable(OcctImGuiTests
  test_occt_imgui.cpp
)

target_include_directories(OcctImGuiTests PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(OcctImGuiTests PRIVATE
  OcctImGui
  gtest_main
  OpenCASCADE::OpenCASCADE
)

include(GoogleTest)

if(WIN32)
  cmake_path(GET OpenCASCADE_LIBRARY_DIR PARENT_PATH OCCT_ARCH_DIR)
  gtest_discover_tests(OcctImGuiTests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OCCT_ARCH_DIR}/bin"
  )
else()
  gtest_discover_tests(OcctImGuiTests)
endif()
