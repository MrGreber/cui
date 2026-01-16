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
        __DIR__="${CMAKE_CURRENT_SOURCE_DIR}"
)

find_package(OpenGL REQUIRED)

include(CheckCCompilerFlag)

check_c_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
if(COMPILER_SUPPORTS_AVX2)
    message(STATUS "Enabling AVX/AVX2 SIMD instructions")
    target_compile_options(stream_draw PRIVATE -mavx2 -mfma -msse4.2)
endif()

if(MSVC)
    set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/glfw3.lib")
else()
    set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/libglfw3.a")
endif()

target_link_libraries(stream_draw PRIVATE
        glad
        OpenGL::GL
        ${GLFW_LIBRARY}
        opengl32
        gdi32
        user32
        shell32
        shlwapi
)


