# Setup CMake
#set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_STANDARD 17)

cmake_minimum_required(VERSION 3.20)

# Project Setup
project(openglEngine VERSION 1.0)

set(SOURCES 
	"src/main.cpp"
	"src/include/Window.hpp"
	"src/include/Mesh.hpp"
	"src/include/Model.hpp"
	"src/include/ObjectHandler.hpp"
	"src/include/FileHandler.hpp"
	"src/include/GameObject.hpp"
	"src/include/StaticBody.hpp"
	"src/include/shader/BaseShader.hpp"
)

if(WIN32)
	set(CMAKE_CXX_COMPILER "C:/Users/JTK6759/Documents/apps/msys2/ucrt64/bin/g++.exe")
	set(COMPILE_FLAGS="-g -Wall -Dmain=SDL_main")

	add_link_options("-lmingw32" "-lSDL2" "-lSDL2main" "-lSOIL" "-lfreeglut" "-lglew32" "-lopengl32" "-lassimp")
	add_executable(openglEngine)

	target_compile_definitions(openglEngine PUBLIC DEBUG)
	target_link_libraries(openglEngine PUBLIC SDL2 SOIL opengl32 glew32 freeglut assimp)
else()
	set(CMAKE_CXX_COMPILER "/usr/bin/g++")
	set(COMPILE_FLAGS="-g -Wall")
	
	add_link_options("-lSDL2" "-lGLEW" "-lGL" "-lSOIL" "-lassimp")
	
	add_executable(openglEngine)
	
	target_compile_definitions(openglEngine PUBLIC DEBUG)
	target_link_libraries(openglEngine PUBLIC SDL2 GL GLEW SOIL assimp)
endif()

target_sources(openglEngine PUBLIC ${SOURCES})
target_include_directories(openglEngine PUBLIC "src/" "src/include/")