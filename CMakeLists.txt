cmake_minimum_required(VERSION 3.25)
project(GC_Lab4_FlatShading)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL2_INCLUDE_DIR "C:/Libs/SDL/include")
set(SDL2_LIB_DIR "C:/Libs/SDL/lib/x64")

# Agregar la ruta de inclusión de la biblioteca glm
set(GLM_INCLUDE_DIR "C:/Users/50242/Documents/Universidad/TercerAño/GraficasComputadora/GC-Lab4-FlatShading/glm")

# Utiliza GLM_INCLUDE_DIR en lugar de CMAKE_INCLUDE_PATH
include_directories(${SDL2_INCLUDE_DIR} ${GLM_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

add_executable(GC_Lab4_FlatShading main.cpp Vertices.h Fragment.h Uniforms.h Color.h Triangle.h Print.h Shaders.h)
target_link_libraries(GC_Lab4_FlatShading SDL2main SDL2)
