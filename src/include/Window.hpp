#pragma once

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

#include <SDL2/SDL_video.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <bitset>
#include <string>
#include <cmath>

#include "Camera.hpp"
#include "UI.hpp"

#define CREATE_ERROR(msg, err) fprintf(stderr, "Window::init(): %s %i %c", msg, err, '\n'); return 0
#define CREATE_GL_ERROR(msg) fprintf(stderr, "Window::initOpenGL(): %s %c", msg, '\n'); return false
//#define KEYBOARD windowData.inputData.keyboardState

/// @brief Wrapper class for a window's state and settings flags
class WindowFlags {
	public:
		WindowFlags() : flags(0) {}
		WindowFlags(const WindowFlags& copy) : flags(copy.flags) {}
		~WindowFlags() = default;

		// Setters
		void vsync(const uint8_t& val) {
			if(val > 2){
				std::cerr << "Unable to set vsync: Invalid setting\n";
				return;
			}

			SDL_GL_SetSwapInterval(val);
			switch(val) {
				case(0):
					flags.reset(0);
					flags.reset(1);
					break;
				case(1):
					flags.set(0);
					flags.reset(1);
					break;
				case(2):
					flags.reset(0);
					flags.set(1);
					break;
			}
		}
		void paused(const bool& val) { flags.set(2, val); }
		void zBuffer(const bool& val) { flags.set(3, val); }
		void debugDraw(const bool& val) { flags.set(4, val); }

		// Getters
		uint8_t vsync() const { return flags.to_ulong() & 0b11; };
		bool paused() const { return flags.test(2); }
		bool zBuffer() const { return flags.test(3); }
		bool debugDraw() const { return flags.test(4); }

		const std::bitset<64>& getRaw() const {
			return flags;
		}
	private:
		std::bitset<64> flags;
};

/// @brief Data used to create a SDL window
/// @note SDL_WINDOW_OPENGL is implied in sdlFlags
struct WindowCreationData {
	std::string title = "Window";
	uint32_t x = SDL_WINDOWPOS_CENTERED;
	uint32_t y = SDL_WINDOWPOS_CENTERED;
	uint32_t width = 640;
	uint32_t height = 480;
	uint32_t sdlFlags = SDL_WINDOW_RESIZABLE;
	uint32_t sdlSubsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
	WindowFlags flags = WindowFlags();
	float minFrameTime = 16.66666667;
};

class Window {
	public:
		Window() : workingDir("") {}
		Window(const std::string_view& workingDir) : workingDir(workingDir) {}
		~Window() { SDL_DestroyWindow(window); }
		/// Initializes subsystems and creates the window and other resources
		int init(const WindowCreationData& creationData) {
			// Update windowData defaults
			windowData.title = creationData.title;
			windowData.width = creationData.width;
			windowData.height = creationData.height;
			windowData.minFrameTime = creationData.minFrameTime;
			windowData.flags = WindowFlags(creationData.flags);

			window = SDL_CreateWindow(
				creationData.title.c_str(),
				creationData.x,
				creationData.y,
				creationData.width,
				creationData.height,
				creationData.sdlFlags | SDL_WINDOW_OPENGL
			);
			if(window == nullptr){
				CREATE_ERROR("Unable to create the window, SDL_Error: ", SDL_GetError());
			}

			gContext = SDL_GL_CreateContext(window);
			if(gContext == nullptr){
				CREATE_ERROR("Unable to create OpenGL context, SDL_Error: ", SDL_GetError());
			}

			glewExperimental = GL_TRUE;
			GLenum glewStatus = glewInit();
			if (glewStatus != GLEW_OK){
				CREATE_ERROR("Unable to initialize GLEW, GLEW Error: ", glewGetErrorString(glewStatus));
			}

			glViewport(0, 0, windowData.width, windowData.height);

			// OpenGL settings, move to main?
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			GLenum err = glGetError();
			if(err != GL_NO_ERROR) {
				std::cerr << "Unhandled OpenGL Error: " << err << std::endl;
				return 0;
			}

			// Set VSync
			if(SDL_GL_SetSwapInterval(windowData.flags.vsync()) == -1) {
				std::cerr << "Warning: Unable to set VSync, SDL_Error: " << SDL_GetError() << std::endl;
			}

			windowData.id = SDL_GetWindowID(window);
			if(windowData.id == 0){
				CREATE_ERROR("Unable to get window id, SDL_Error: ", SDL_GetError());
				return 0;
			}

			return true;
		}
		int processEvent(const SDL_Event& event) {
			switch(event.type) {
				case SDL_WINDOWEVENT: {
					switch(event.window.event) {
						case SDL_WINDOWEVENT_CLOSE:	// Window receives close command
							return 1;
						case SDL_WINDOWEVENT_RESIZED:
							SDL_GL_GetDrawableSize(window, &windowData.width, &windowData.height);
							glViewport(0, 0, windowData.width, windowData.height);
						default:
							break;
					}
				}
				default:
					break;
			}
			return 0;
		}
		/// Render
		void render() {	}
		glm::vec2 getDimensions() const {
			return glm::vec2(windowData.width, windowData.height);
		}
		SDL_Window* getSDLWindow() {
		    return window;
		}
		/// --- Variables --- ///

		/// @brief Collection of window-specific data
		struct WindowData {
			// Time (seconds)
			uint64_t prevTime = 0;				// Time when last frame completed(ms)
			float deltaT = 0.f;					// Time between frames
			float frameTime = 0.f;				// Total time to process a frame
			float renderTime = 0.f;				// Time to render/draw a frame(including debug draw)
			float minFrameTime = 16.66666667f;	// The minimum time the frame should take(ms)

			// Window aspects
			int width = 640;
			int height = 480;
			std::string title = "Window";
			uint32_t id;
			WindowFlags flags;

			struct InputData {
				// Mouse
				uint32_t mbState = 0;
				int32_t xPos = 0;
				int32_t yPos = 0;
				float sensitivity = 0.1f;

				// Keyboard
				const uint8_t* keyboardState = nullptr;
			} inputData;
		} windowData;
	private:
		std::string workingDir;

		SDL_Window* window = NULL;
		SDL_GLContext gContext = NULL;
};
