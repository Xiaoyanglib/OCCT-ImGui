set(CURRENT_LIB_NAME glad)

add_library(${CURRENT_LIB_NAME})

target_sources(${CURRENT_LIB_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

target_include_directories(${CURRENT_LIB_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)