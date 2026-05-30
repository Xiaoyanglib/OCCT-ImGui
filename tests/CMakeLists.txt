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

# Copy OCCT shared libs to test output dir so gtest_discover_tests can run
if(WIN32)
  get_filename_component(OCCT_ARCH_DIR ${OpenCASCADE_LIBRARY_DIR}/.. ABSOLUTE)
  set(OCCT_DLL_DIRS ${OCCT_ARCH_DIR}/bin ${OpenCASCADE_LIBRARY_DIR})
  set(OCCT_DLL_GLOB "*.dll")
elseif(APPLE)
  set(OCCT_DLL_DIRS ${OpenCASCADE_LIBRARY_DIR})
  set(OCCT_DLL_GLOB "*.dylib")
endif()

foreach(DIR ${OCCT_DLL_DIRS})
  file(GLOB OCCT_DLLS ${DIR}/${OCCT_DLL_GLOB})
  list(APPEND ALL_OCCT_DLLS ${OCCT_DLLS})
endforeach()

if(ALL_OCCT_DLLS)
  add_custom_command(TARGET OcctImGuiTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ALL_OCCT_DLLS} $<TARGET_FILE_DIR:OcctImGuiTests>
    COMMENT "Copying OCCT shared libraries for test discovery"
  )
endif()

include(GoogleTest)
gtest_discover_tests(OcctImGuiTests)
