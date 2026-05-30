include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/deps/googletest
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(OcctImGuiTests
  test_occt_imgui.cpp
)

target_include_directories(OcctImGuiTests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(OcctImGuiTests PRIVATE
  OcctImGui
  GTest::gtest_main
  OpenCASCADE::OpenCASCADE
)

include(GoogleTest)

if(WIN32)
  # DLLs are in <prefix>/win64/vc14/bin, sibling of lib/
  cmake_path(GET OpenCASCADE_LIBRARY_DIR PARENT_PATH OCCT_ARCH_DIR)
  gtest_discover_tests(OcctImGuiTests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OCCT_ARCH_DIR}/bin"
  )
else()
  gtest_discover_tests(OcctImGuiTests)
endif()
