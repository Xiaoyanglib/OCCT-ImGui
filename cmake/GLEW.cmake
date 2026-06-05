if(NOT TARGET glew)
  include(FetchContent)

  FetchContent_Declare(
    glew
    URL      https://github.com/nigels-com/glew/releases/download/glew-2.3.1/glew-2.3.1.tgz
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/glew
  )
  FetchContent_Populate(glew)

  add_library(glew STATIC ${glew_SOURCE_DIR}/src/glew.c)
  target_include_directories(glew PUBLIC ${glew_SOURCE_DIR}/include)
  target_compile_definitions(glew PRIVATE GLEW_STATIC)
endif()
