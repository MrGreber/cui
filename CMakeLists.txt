cmake_minimum_required(VERSION 3.31)
project(cui C)

set(CMAKE_C_STANDARD 23)

if(MSVC)
    set(COMMON_FLAGS /O2;/utf-8;/arch:AVX2)
else()
    set(COMMON_FLAGS -O3;-mavx2;-mfma;-msse4.2)
endif()

add_compile_options(${COMMON_FLAGS})

file(GLOB SOURCE "Source/*.c")
file(GLOB UI "UI/*.c")
file(GLOB DS "Data-Structures/*.c")

add_library(glad STATIC External/Source/glad.c)
target_include_directories(glad PUBLIC External/Include)

add_executable(cui
        ${SOURCE}
        ${UI}
        ${DS}
        test.c
        entry.c
)

target_include_directories(cui SYSTEM PRIVATE
        External/Include
        UI
        Include
        Data-Structures
)

set_target_properties(cui PROPERTIES
        WIN32_EXECUTABLE FALSE
)

target_compile_definitions(cui PRIVATE
        SIMD
        GLFW_STATIC
        UNICODE _UNICODE
        __DIR__="${CMAKE_CURRENT_SOURCE_DIR}"
)

find_package(OpenGL REQUIRED)

include(CheckCCompilerFlag)

if(MSVC)
    set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/glfw3.lib")
else()
    set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/External/Library/libglfw3.a")
endif()

target_link_libraries(cui PRIVATE
        glad
        OpenGL::GL
        ${GLFW_LIBRARY}
        opengl32
        gdi32
        user32
        shell32
        shlwapi
)


