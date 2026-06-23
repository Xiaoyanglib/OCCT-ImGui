set(CURRENT_APP_NAME OcctImGuiDemo)

add_executable(${CURRENT_APP_NAME}
        main.cpp
)
target_link_libraries(${CURRENT_APP_NAME}
        PRIVATE
        OcctImGui
)