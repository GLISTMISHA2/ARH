cmake_minimum_required(VERSION 3.10)
project(NeonVisualizer)

set(CMAKE_CXX_STANDARD 17)

# Настройка флагов оптимизации
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # Для Orange Pi 5 (64-бит) NEON встроен по умолчанию, флаг -mfpu вызывает ошибку
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
    message(STATUS "AArch64 Detected: NEON is active by default")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    # Для старых 32-битных систем
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mfpu=neon")
    message(STATUS "ARM 32-bit Detected: Enabling NEON via mfpu")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    message(STATUS "x86/Other Detected")
endif()

# Поиск библиотек
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
find_package(OpenGL REQUIRED)

# Пути к заголовочным файлам
include_directories(
    .
    imgui 
    imgui/backends 
    implot
)

# Список файлов проекта
set(SOURCES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    implot/implot.cpp
    implot/implot_items.cpp
    implot/implot_demo.cpp
)

# Создание исполняемого файла
add_executable(neongui ${SOURCES})

# Линковка библиотек
# Добавил X11 и другие зависимости, которые могут понадобиться на Linux
target_link_libraries(neongui 
    ${GLFW3_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    dl 
    pthread
    X11
)