if(NOT TARGET glfw)
  include(GLFW)
endif()

if(NOT TARGET imgui)
  include(ImGui)
endif()

if(WIN32 AND NOT TARGET glew)
  include(GLEW)
endif()

if(NOT TARGET OpenCASCADE::OpenCASCADE)
  include(OpenCASCADE)
endif()
