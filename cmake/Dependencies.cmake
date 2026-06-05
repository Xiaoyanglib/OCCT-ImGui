if(NOT TARGET glfw)
  include(GLFW)
endif()

if(NOT TARGET imgui)
  include(ImGui)
endif()

if(NOT TARGET OpenCASCADE::OpenCASCADE)
  include(OpenCASCADE)
endif()
