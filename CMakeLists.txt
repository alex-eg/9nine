cmake_minimum_required(VERSION 3.21)

project(9nine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "./*.cpp" "./*.hpp")
add_executable(9nine ${SOURCES} "vert.glsl" "frag.glsl")
target_compile_options(9nine PRIVATE -Wall -Wextra -Wpedantic -Werror -Wsuggest-override  -Wno-deprecated-declarations)

set(SDL2_LIBRARY SDL2-static CACHE STRING "" FORCE)
add_subdirectory(modules/sdl)
target_compile_options(SDL2-static PRIVATE "-w")
add_dependencies(9nine SDL2-static)
target_link_libraries(9nine SDL2-static)
target_link_libraries(9nine SDL2main)

set(DISABLE_GLES_1 TRUE CACHE BOOL "Disable GLES1" FORCE)
set(INSTALL_LIBRARY FALSE CACHE BOOL "Disable installing" FORCE)
set(BUILD_STATIC TRUE CACHE BOOL "Enable static" FORCE)
set(BUILD_SHARED FALSE CACHE BOOL "Disable shared" FORCE)
set(BUILD_DEMOS FALSE CACHE BOOL "Disable demos" FORCE)
add_subdirectory(modules/sdl-gpu)
target_compile_options(SDL_gpu PRIVATE "-w")
add_dependencies(9nine SDL_gpu)
target_link_libraries(9nine SDL_gpu)
target_include_directories(SDL_gpu PUBLIC modules/sdl-gpu/include)

target_include_directories(9nine PRIVATE modules modules/imgui)

find_package(OpenGL REQUIRED OpenGL)
target_link_libraries(9nine OpenGL::GL)
include_directories(${OPENGL_INCLUDE_DIR})

add_library(imgui STATIC
    modules/imgui/imgui.cpp
    modules/imgui/imgui_draw.cpp
    modules/imgui/imgui_tables.cpp
    modules/imgui/imgui_widgets.cpp
    modules/imgui/backends/imgui_impl_sdl.cpp)
target_include_directories(imgui PRIVATE modules/imgui modules/sdl/include)
target_compile_options(imgui PRIVATE "-w")
target_link_libraries(9nine imgui)
