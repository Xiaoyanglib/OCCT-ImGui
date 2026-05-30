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
)

include(GoogleTest)

# Copy OCCT shared libs to test output dir so gtest_discover_tests can run
get_filename_component(OCCT_PREFIX ${OpenCASCADE_DIR}/../../.. ABSOLUTE)
if(WIN32)
  set(OCCT_BIN_DIR ${OCCT_PREFIX}/bin)
  set(OCCT_DLL_SUFFIX dll)
elseif(APPLE)
  set(OCCT_BIN_DIR ${OCCT_PREFIX}/lib)
  set(OCCT_DLL_SUFFIX dylib)
endif()

file(GLOB OCCT_DLLS ${OCCT_BIN_DIR}/*.${OCCT_DLL_SUFFIX})
if(OCCT_DLLS)
  add_custom_command(TARGET OcctImGuiTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OCCT_DLLS} $<TARGET_FILE_DIR:OcctImGuiTests>
    COMMENT "Copying OCCT shared libraries for test discovery"
  )
endif()

include(GoogleTest)
gtest_discover_tests(OcctImGuiTests)
