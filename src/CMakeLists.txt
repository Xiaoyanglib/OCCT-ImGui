set(CURRENT_LIB_NAME OcctImGui)

add_library(${CURRENT_LIB_NAME})

file(GLOB LIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx)
target_sources(${CURRENT_LIB_NAME}
        PRIVATE
        ${LIB_SOURCES}
)

target_include_directories(${CURRENT_LIB_NAME}
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include/
)
target_include_directories(${CURRENT_LIB_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include/OcctImGui/
        ${PROJECT_SOURCE_DIR}/src/
        ${PROJECT_SOURCE_DIR}/third_party/imoguizmo/
)

target_link_libraries(${CURRENT_LIB_NAME}
        PUBLIC
        imgui
        glfw
        glad
        OpenCASCADE::OpenCASCADE
)

target_compile_definitions(${CURRENT_LIB_NAME}
        PRIVATE
        FONTS_DIR="${PROJECT_SOURCE_DIR}/src/fonts"
)

if(WIN32)
    target_compile_definitions(${CURRENT_LIB_NAME}
            PRIVATE
            NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(APPLE)
    target_link_libraries(${CURRENT_LIB_NAME}
            PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
    )
endif()

if(MSVC)
    target_compile_options(${CURRENT_LIB_NAME}
            PRIVATE
            /EHsc)
else()
    target_compile_options(${CURRENT_LIB_NAME}
            PRIVATE
            -fexceptions
            -Wno-elaborated-enum-base
    )
endif()