cmake_minimum_required(VERSION 3.31)

project(Graphics_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Graphics_engine
        src/main.cpp
        src/core/application.cpp
        src/renderer/renderer.cpp
        src/renderer/material.cpp
        src/renderer/mesh/vertex_buffer.cpp
        src/renderer/mesh/index_buffer.cpp
        src/renderer/mesh/vertex_array.cpp
        src/renderer/mesh/mesh.cpp
        src/renderer/mesh/mesh_factory.cpp
        src/gl_debug.cpp
        src/shader.cpp
        src/camera.cpp
        src/ui_text.cpp
        src/font_data.cpp
        src/texture2d.cpp
        src/menu.cpp
)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(Graphics_engine
        PRIVATE
        glfw
        glad::glad
        glm::glm
)

target_compile_definitions(Graphics_engine PRIVATE GLFW_INCLUDE_NONE)

# Copy assets to build directory
add_custom_command(TARGET Graphics_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Graphics_engine>/assets
    COMMENT "Copying assets to build directory"
)
