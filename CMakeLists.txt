cmake_minimum_required(VERSION 3.31)
project(stream_draw C)

set(CMAKE_C_STANDARD 23)

file(GLOB SOURCE "Source/*.c")
file(GLOB UI "UI/*.c")
file(GLOB DS "Data-Structures/*.c")

add_library(glad STATIC External/Source/glad.c)
target_include_directories(glad PUBLIC External/Include)

add_executable(stream_draw
        ${SOURCE}
        ${UI}
        ${DS}
        entry.c
)

target_include_directories(stream_draw SYSTEM PRIVATE
        External/Include
        UI
        Include
        Data-Structures
)

set_target_properties(stream_draw PROPERTIES
        WIN32_EXECUTABLE FALSE
)

target_compile_definitions(stream_draw PRIVATE
        APP=1
        GLFW_STATIC
        PROJECT_NAME=\"sDraw-\"
        UNICODE _UNICODE
)

find_package(OpenGL REQUIRED)

target_link_libraries(stream_draw PRIVATE
        glad
        OpenGL::GL
        "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/glfw3.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/stb_image.lib"
        opengl32
        gdi32
        user32
        shell32
)


