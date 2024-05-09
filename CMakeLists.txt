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
	"src/include/UI.hpp"
	"src/include/Mesh.hpp"
	"src/include/Model.hpp"
	"src/include/ObjectHandler.hpp"
	"src/include/FileHandler.hpp"
	"src/include/GameObject.hpp"
	"src/include/StaticBody.hpp"
	"src/include/shader/BaseShader.hpp"
	"src/include/shader/TextShader.hpp"
)

if(WIN32)
	set(CMAKE_CXX_COMPILER "C:/Users/JTK6759/Documents/apps/msys2/ucrt64/bin/g++.exe")
	set(COMPILE_FLAGS="-g -Wall -Dmain=SDL_main")

	add_link_options("-lmingw32" "-lSDL2" "-lSDL2main" "-lSOIL" "-lfreeglut" "-lglew32" "-lopengl32" "-lassimp" "-lfreetype")
	add_executable(openglEngine)

	target_compile_definitions(openglEngine PRIVATE DEBUG)
	target_link_libraries(openglEngine PRIVATE SDL2 SOIL opengl32 glew32 freeglut assimp freetype)
else()
	set(CMAKE_CXX_COMPILER "/usr/bin/g++")
	set(COMPILE_FLAGS="-g -Wall")

	add_link_options("-lSDL2" "-lGLEW" "-lGL" "-lSOIL" "-lassimp" "-lfreetype")
	
	add_executable(openglEngine)
	
	# If your distro used freetype2 for freetype 2.x
	if(NOT EXISTS "/usr/include/freetype/")
		target_include_directories(openglEngine PRIVATE "/usr/include/freetype2/")
	endif()
	target_compile_definitions(openglEngine PRIVATE DEBUG)
	target_link_libraries(openglEngine PRIVATE SDL2 GL GLEW SOIL assimp freetype)
endif()

target_sources(openglEngine PRIVATE ${SOURCES})
target_include_directories(openglEngine PRIVATE "src/" "src/include/")