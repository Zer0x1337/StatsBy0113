cmake_minimum_required(VERSION 3.10)
project(StatsBy0113 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ImGui sources
set(IMGUI_DIR "imgui-1.92.1")

# Find ImGui sources
file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
)

# Find ImGui backends
file(GLOB IMGUI_BACKENDS
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

# Find GLFW and OpenGL
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(StatsBy0113
    main.cpp
    SystemMonitor.cpp
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
)

# Include directories
target_include_directories(StatsBy0113 PRIVATE
    .
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    ${GLFW3_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(StatsBy0113 PRIVATE
    glfw
    OpenGL::GL
    X11
    Xrandr
    Xinerama
    Xi
    Xcursor
    pthread
    m
    dl
    rt
)