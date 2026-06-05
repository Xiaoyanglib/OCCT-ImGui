if(NOT TARGET glfw)
  include(FetchContent)

  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/third_party/glfw
  )

  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(glfw)
endif()
