cmake_minimum_required(VERSION 3.31)

project(Graphics_engine VERSION 0.19.4 LANGUAGES CXX)

include(CTest)

option(GRAPHICS_ENGINE_WARNINGS_AS_ERRORS
       "Treat project compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GRAPHICS_ENGINE_SOURCES
        src/main.cpp
        src/core/application.cpp
        src/core/asset_paths.cpp
        src/core/benchmark_report.cpp
        src/core/editor_camera.cpp
        src/core/editor_gizmo.cpp
        src/core/editor_layout.cpp
        src/core/editor_placement.cpp
        src/core/editor_transform.cpp
        src/core/scene_document.cpp
        src/core/scene_history.cpp
        src/core/spatial_query.cpp
        src/core/window_settings.cpp
        src/core/resource_manager.cpp
        src/renderer/renderer.cpp
        src/renderer/coordinate_grid.cpp
        src/renderer/editor_chrome.cpp
        src/renderer/selection_outline.cpp
        src/renderer/framebuffer.cpp
        src/renderer/post_processor.cpp
        src/renderer/material.cpp
        src/renderer/model_loader.cpp
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
find_package(nlohmann_json CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)

function(graphics_engine_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /EHsc
            /external:anglebrackets
            /external:W0
            $<$<BOOL:${GRAPHICS_ENGINE_WARNINGS_AS_ERRORS}>:/WX>
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<BOOL:${GRAPHICS_ENGINE_WARNINGS_AS_ERRORS}>:-Werror>
        )
    endif()
endfunction()

add_executable(Graphics_engine ${GRAPHICS_ENGINE_SOURCES})

target_link_libraries(Graphics_engine
        PRIVATE
        glfw
        glad::glad
        glm::glm
        nlohmann_json::nlohmann_json
        assimp::assimp
)

target_compile_definitions(Graphics_engine PRIVATE
        GLFW_INCLUDE_NONE
        $<$<CONFIG:Debug>:GRAPHICS_ENGINE_SOURCE_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets">
)

target_include_directories(Graphics_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(Graphics_engine SYSTEM PRIVATE ${Stb_INCLUDE_DIR})
graphics_engine_enable_warnings(Graphics_engine)

file(GLOB_RECURSE GRAPHICS_ENGINE_ASSET_FILES CONFIGURE_DEPENDS
     LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/assets/*")
set_property(TARGET Graphics_engine APPEND PROPERTY
             LINK_DEPENDS "${GRAPHICS_ENGINE_ASSET_FILES}")

# Recreate the runtime asset tree after relinking so removed source files
# cannot survive as stale build artifacts.
add_custom_command(TARGET Graphics_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf
            "$<TARGET_FILE_DIR:Graphics_engine>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Graphics_engine>/assets"
    COMMENT "Copying assets to build directory"
    VERBATIM
)

if(WIN32)
    add_custom_command(TARGET Graphics_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t
                "$<TARGET_FILE_DIR:Graphics_engine>"
                $<TARGET_RUNTIME_DLLS:Graphics_engine>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLL dependencies"
        VERBATIM
    )
endif()

if(BUILD_TESTING)
    add_executable(Graphics_engine_core_tests
        tests/core_unit_tests.cpp
        src/core/benchmark_report.cpp
        src/core/editor_camera.cpp
        src/core/editor_gizmo.cpp
        src/core/editor_layout.cpp
        src/core/editor_placement.cpp
        src/core/editor_transform.cpp
        src/core/scene_document.cpp
        src/core/scene_history.cpp
        src/core/spatial_query.cpp
        src/core/window_settings.cpp
    )
    target_include_directories(Graphics_engine_core_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(Graphics_engine_core_tests PRIVATE
        glm::glm
        nlohmann_json::nlohmann_json
    )
    graphics_engine_enable_warnings(Graphics_engine_core_tests)
    add_test(NAME core.unit COMMAND Graphics_engine_core_tests)
    set_tests_properties(core.unit PROPERTIES TIMEOUT 120)
endif()

if(WIN32)
    install(TARGETS Graphics_engine
        RUNTIME DESTINATION "."
        COMPONENT Runtime
    )
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        file(GLOB GRAPHICS_ENGINE_VCPKG_RELEASE_DLLS
             "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/*.dll")
        file(GLOB GRAPHICS_ENGINE_VCPKG_DEBUG_DLLS
             "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/*.dll")
        install(FILES ${GRAPHICS_ENGINE_VCPKG_RELEASE_DLLS}
                DESTINATION "." COMPONENT Runtime
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
        install(FILES ${GRAPHICS_ENGINE_VCPKG_DEBUG_DLLS}
                DESTINATION "." COMPONENT Runtime
                CONFIGURATIONS Debug)
    endif()
else()
    install(TARGETS Graphics_engine
        RUNTIME DESTINATION "."
        COMPONENT Runtime
    )
endif()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/"
        DESTINATION assets COMPONENT Runtime)

if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(GRAPHICS_ENGINE_REDIST_ARCH "x64")
    else()
        set(GRAPHICS_ENGINE_REDIST_ARCH "x86")
    endif()
    file(TO_CMAKE_PATH "$ENV{VCToolsRedistDir}" GRAPHICS_ENGINE_REDIST_ROOT)
    file(GLOB GRAPHICS_ENGINE_CRT_DIRECTORIES LIST_DIRECTORIES true
         "${GRAPHICS_ENGINE_REDIST_ROOT}/${GRAPHICS_ENGINE_REDIST_ARCH}/Microsoft.VC*.CRT")
    list(SORT GRAPHICS_ENGINE_CRT_DIRECTORIES ORDER DESCENDING)
    if(GRAPHICS_ENGINE_CRT_DIRECTORIES)
        list(GET GRAPHICS_ENGINE_CRT_DIRECTORIES 0 GRAPHICS_ENGINE_CRT_DIRECTORY)
        install(DIRECTORY "${GRAPHICS_ENGINE_CRT_DIRECTORY}/"
                DESTINATION "." COMPONENT Runtime
                FILES_MATCHING PATTERN "*.dll")
    else()
        message(WARNING
            "MSVC redistributable directory was not found; the ZIP will require "
            "the matching Visual C++ Redistributable to be installed")
    endif()
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "Graphics-engine")
set(CPACK_PACKAGE_VENDOR "Graphics Engine")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Interactive OpenGL graphics engine and GPU benchmark")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_MONOLITHIC_INSTALL ON)
if(WIN32)
    set(CPACK_SYSTEM_NAME "windows-x64")
endif()
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")

include(CPack)
