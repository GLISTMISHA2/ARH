cmake_minimum_required(VERSION 3.10)
project(NeonVisualizer)

set(CMAKE_CXX_STANDARD 17)

# Авто-определение архитектуры
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mfpu=neon")
    message(STATUS "ARM Detected: Enabling NEON optimizations")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    message(STATUS "x86 Detected: NEON disabled")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
find_package(OpenGL REQUIRED)

include_directories(imgui imgui/backends implot)

set(SOURCES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    implot/implot.cpp
    implot/implot_items.cpp
)

add_executable(neongui ${SOURCES})
target_link_libraries(neongui ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES} dl pthread)