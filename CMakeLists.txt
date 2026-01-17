cmake_minimum_required(VERSION 3.31)

project(Graphics_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Graphics_engine
        src/main.cpp
        src/gl_debug.cpp
        src/shader.cpp
        src/camera.cpp
        src/ui_text.cpp
        src/font_data.cpp
        src/texture2d.cpp
        src/texture_menu.cpp
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
