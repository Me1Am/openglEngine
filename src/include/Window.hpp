#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <cmath>

#include "PhysicsEngine.hpp"
#include "ObjectHandler.hpp"
#include "shader/BaseShader.hpp"
#include "Camera.hpp"
#include "UI.hpp"
#include "Heightmap.hpp"

#include "ecs/ECS.hpp"

#define CREATE_ERROR(msg, err) fprintf(stderr, "Window::init(): %s %i %c", msg, err, '\n'); return 0
#define CREATE_GL_ERROR(msg) fprintf(stderr, "Window::initOpenGL(): %s %c", msg, '\n'); return false
#define KEYBOARD windowData.inputData.keyboardState

/// @brief Data used to create a SDL window
/// @note SDL_WINDOW_OPENGL is implied in sdlFlags
struct WindowCreationData {
	std::string title = "Window";
	int x = SDL_WINDOWPOS_CENTERED;
	int y = SDL_WINDOWPOS_CENTERED;
	int width = 640;
	int height = 480;
	uint32_t sdlFlags = SDL_WINDOW_RESIZABLE;
	uint32_t sdlSubsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
};

class Window {
	public:
		Window() {}
		~Window() {
			baseShader.freeProgram();
			heightmap.freeProgram();
			textShader.freeProgram();

			SDL_DestroyWindow(window);

			SDL_Quit();	// Quit SDL

			delete physicsEngine;
			delete ui;
			delete heightfield;
		}
		/// Initializes subsystems and creates the window and other resources
		int init(const WindowCreationData& creationData) {
			// Update windowData defaults
			windowData.title = creationData.title;
			windowData.width = creationData.width;
			windowData.height = creationData.height;

			// Init Video and Events Subsystems
			if(SDL_Init(creationData.sdlSubsystems) < 0){
				CREATE_ERROR("Unable to initialize subsystems, SDL_Error: ", SDL_GetError());
			}

			// Use OpenGL 4.1 core
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

			// Other OpenGL settings
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

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

			// TODO Creat program args function to enable vsync
			// Enable VSync
			SDL_GL_SetSwapInterval(1);
			if(SDL_GL_GetSwapInterval() != 1){
				std::cerr << "Warning: Unable to enable VSync, SDL_Error: " << SDL_GetError() << std::endl;
			}

			// Initialize shared variables
			delta_t.reset(new Uint32(0));
			debugDrawTime.reset(new Uint32(0));
			pause.reset(new bool(true));

			physicsEngine = new PhysicsEngine();
			if(!physicsEngine->init()){
				std::cerr << "Unable to initialize physics engine" << std::endl;
				return 0;
			}

			if(!initOpenGL()){
				std::cerr << "Unable to initialize OpenGL" << std::endl;
				return 0;
			}

			KEYBOARD = SDL_GetKeyboardState(NULL);	// Get pointer to internal keyboard state

			windowData.id = SDL_GetWindowID(window);
			if(windowData.id == 0){
				CREATE_ERROR("Unable to get window id, SDL_Error: ", SDL_GetError());
			}

			windowData.paused(true);
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_ShowCursor(SDL_TRUE);

			return true;
		}
		/// Runs main loop
		void loop() {
			SDL_Event event;

			// Main Event Loop
			while(true) {
				while(SDL_PollEvent(&event)) {
					// Main Event Handler
					switch(event.type) {
						case SDL_WINDOWEVENT: {
							if(event.window.windowID == windowData.id){
								switch(event.window.event) {
									case SDL_WINDOWEVENT_CLOSE:	// Window receives close command
										return;
									case SDL_WINDOWEVENT_RESIZED:
										resize();
									default:
										break;
								}
							}
							break;
						} case SDL_KEYDOWN: {
							// Pause
							if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
								windowData.paused(!windowData.paused());
								*pause = !(*pause);

								SDL_SetRelativeMouseMode((SDL_bool)(!windowData.paused()));
								SDL_ShowCursor((SDL_bool)(windowData.paused()));
							// Show z-buffer
							} else if(event.key.keysym.scancode == SDL_SCANCODE_LALT && !windowData.paused()){
								windowData.zBuffer(!windowData.zBuffer());
								if(windowData.zBuffer()){
									if(!baseShader.loadProgram("../shaders/texture.vert", "../shaders/zBuffer.frag", "", "")){
										std::cerr << "Unable to load z-buffer shader" << std::endl;
										baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "");
										windowData.zBuffer(false);
									}
								} else {
									baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "");
								}
							// Reset physics sim
							} else if(event.key.keysym.scancode == SDL_SCANCODE_R) {
								physicsEngine->reset();
							// Enable physics debug draw
							} else if(event.key.keysym.scancode == SDL_SCANCODE_F1) {
								windowData.debugDraw(!windowData.debugDraw());

								if(!windowData.debugDraw())
									*debugDrawTime = 0;
							// Save the physics engine state
							} else if(event.key.keysym.scancode == SDL_SCANCODE_F5) {
								physicsEngine->saveState("./saves/savedState.bin");

								#ifdef DEBUG
									std::cout << "Saved physics state to file: ./saves/savedState.bin\n";
								#endif
							// Load a physics engine state file
							} else if(event.key.keysym.scancode == SDL_SCANCODE_F6) {
								physicsEngine->loadState("./saves/savedState.bin");

								#ifdef DEBUG
									std::cout << "Loaded physics state from file: ./saves/savedState.bin\n";
								#endif
							}
							break;
						} case SDL_MOUSEMOTION: {
							if(windowData.paused()) break;
							float offsetX = event.motion.xrel;
							float offsetY = event.motion.yrel;

							// Adjust for sensitivity
							offsetX *= windowData.inputData.sensitivity;
							offsetY *= windowData.inputData.sensitivity;

							camera.incYaw(offsetX);
							camera.incPitch(-offsetY);
						} case SDL_MOUSEBUTTONDOWN: {
							windowData.inputData.mbState = SDL_GetMouseState(&windowData.inputData.xPos, &windowData.inputData.yPos);

							switch(windowData.inputData.mbState) {
								case SDL_BUTTON(1): {	// LMB - Cast ray to pointer(from center of the screen)
									if(!windowData.paused())
										break;

									// Normalize mouse position to [-1, 1]
									glm::vec3 rayNDS = glm::vec3(
										(2.f * windowData.inputData.xPos) / windowData.width - 1.f, 
										1.f - (2.f * windowData.inputData.yPos) / windowData.height, 
										1.f
									);

									glm::vec4 rayClip = glm::vec4(rayNDS.x, rayNDS.y, -1.f, 1.f);
									glm::vec4 rayEye = glm::inverse(glm::perspective(glm::radians(camera.getFOV()), (float)(windowData.width / windowData.height), 0.1f, 1000.f)) * rayClip;
									rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);

									glm::vec3 rayWorld = glm::vec3(glm::inverse(camera.calcCameraView()) * rayEye);
									rayWorld = glm::normalize(rayWorld);

									glm::vec3 rayOrigin = glm::vec3(glm::inverse(camera.calcCameraView()) * glm::vec4(0, 0, 0, 1));

									physicsEngine->castRay(rayOrigin, rayWorld, 100.f);

									break;
								} case SDL_BUTTON(2):	// MMB
									break;
								case SDL_BUTTON(3):		// RMB
									break;
							}
							break;
						} case SDL_MOUSEWHEEL: {
							if(windowData.paused()) break;

							camera.incFOV(-event.wheel.y * 2.5f);
							break;
						} 
						case SDL_QUIT:	// Quit window
							SDL_Quit();	// Cleanup subsystems
							return;	// Exit loop
						default:
							break;
					}
				}

				tick();
				render();

				// Framerate Handling
				Uint32 currentTime = SDL_GetTicks();
				*delta_t = currentTime - windowData.prevTime;
				if(SDL_GL_GetSwapInterval() != 1){
					SDL_Delay((*delta_t < windowData.minFrameTime) ? windowData.minFrameTime - *delta_t : 0);
				}
				windowData.prevTime = SDL_GetTicks();
			}
		}
		/// Resize Window
		// TODO Implement real resizing/keep ratio of drawable items
		void resize() {
			SDL_GL_GetDrawableSize(window, &windowData.width, &windowData.height);
			glViewport(0, 0, windowData.width, windowData.height);
		}
		/// Run Logic
		void tick() {
			// Constant Logic

			// Runtime Logic
			if(windowData.paused()){	// Paused Logic

			} else {
				// Camera
				if(KEYBOARD[SDL_SCANCODE_E]){
					camera.incRoll(1.5f * (*delta_t) / 1000);	// Roll right(increase)
				} else if(KEYBOARD[SDL_SCANCODE_Q]) {
					camera.incRoll(-1.5f * (*delta_t) / 1000);	// Roll left(decrease)
				}
				if(KEYBOARD[SDL_SCANCODE_EQUALS] && camera.getSpeed() < 1.f){
					camera.setSpeed(camera.getSpeed() + (0.00005f * (*delta_t)));
				} else if(KEYBOARD[SDL_SCANCODE_MINUS]){
					camera.setSpeed(camera.getSpeed() - (0.00005f * (*delta_t)));
				}

				// Camera position for view calculations
				camera.updateCameraPosition(
					KEYBOARD[SDL_SCANCODE_W], 
					KEYBOARD[SDL_SCANCODE_S], 
					KEYBOARD[SDL_SCANCODE_A], 
					KEYBOARD[SDL_SCANCODE_D], 
					KEYBOARD[SDL_SCANCODE_SPACE], 
					KEYBOARD[SDL_SCANCODE_LCTRL], 
					*delta_t
				);
				camera.updateCameraDirection();

				physicsEngine->tick((*delta_t) / 1000.f);
			}
		}
		/// Render
		void render() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Draw all objects
			for(long long unsigned int i = 0; i < ObjectHandler::objectList.size(); i++) {
				ObjectHandler::objectList[i].get()->draw(baseShader, camera.calcCameraView(), camera.getFOV());
			}

			heightfield->draw(heightmap, camera.calcCameraView(), camera.getFOV(), false);

			if(windowData.debugDraw()){
				Uint32 currentTime = SDL_GetTicks();
				physicsEngine->debugDraw(camera.calcCameraView(), camera.getFOV(), btIDebugDraw::DBG_DrawWireframe);
				*debugDrawTime = SDL_GetTicks() - currentTime;

				if(*debugDrawTime == 0) *debugDrawTime = 1;	// Make sure it at least shows a value when drawing
			}

			ui->drawTextElements(textShader);

			glUseProgram(0);
			SDL_GL_SwapWindow(window);
		}

		/// --- Variables --- ///

		// TODO Assimilate these into WindowData when revamping UI.hpp
		std::shared_ptr<Uint32> delta_t;		// The running time between frames
		std::shared_ptr<Uint32> debugDrawTime;	// The time it takes to draw physics colliders
		std::shared_ptr<bool> pause;

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

			// State flags
			std::bitset<64> flags = 0;

			void paused(const bool& val) { flags.set(0, val); }
			void zBuffer(const bool& val) { flags.set(1, val); }
			void debugDraw(const bool& val) { flags.set(2, val); }

			bool paused() { return flags.test(0); }
			bool zBuffer() { return flags.test(1); }
			bool debugDraw() { return flags.test(2); }

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
		/// @brief Initializes OpenGL components
		bool initOpenGL() {
			GLint status = GL_FALSE;

			glViewport(0, 0, windowData.width, windowData.height);
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(!baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag", "", "")){
				CREATE_GL_ERROR("Unable to load model shader");
			}

			if(!heightmap.loadProgram("../shaders/heightmap.vert", "../shaders/heightmap.frag", "../shaders/heightmap.tesc", "../shaders/heightmap.tese")){
				CREATE_GL_ERROR("Unable to load heightmap shader");
			}
			if(!textShader.loadProgram("../shaders/text.vert", "../shaders/text.frag")){
				CREATE_GL_ERROR("Unable to load UI text shader");
			}

			heightfield = new Heightmap("../assets/heightmap.png");
			physicsEngine->addRigidBody(heightfield->getRigidBody());

			ui = new UI();
			ui->addTextElement(std::make_unique<DynamicText>(
				Text("Frametime: <%>", { 1.f, 2.f }, { 1.f, 1.f, 1.f }, 0.25f, true), 
				delta_t, 
				[](DynamicText& e){ }
			));
			ui->addTextElement(std::make_unique<DynamicText>(
				Text("Debugtime: <%>", { 1.f, 15.f }, { 1.f, 1.f, 1.f }, 0.25f, true), 
				debugDrawTime, 
				[](DynamicText& e){
					if(e.dynamicVal.expired()) return;
					e.visible = (*std::static_pointer_cast<Uint32>(e.dynamicVal.lock())) > 0;
				}
			));
			ui->addTextElement(std::make_unique<DynamicText>(
				Text("Paused", { 1.f, 28.f }, { 1.f, 0.f, 0.f }, 0.25f, true), 
				pause, 
				[](DynamicText& e){
					if(e.dynamicVal.expired()) return;
					e.visible = *std::static_pointer_cast<bool>(e.dynamicVal.lock());
				}
			));

			GLenum err = glGetError(); 
			if(err != GL_NO_ERROR) {
				std::cerr << "Unhandled OpenGL Error: " << err << std::endl;
				return false;
			}
			return true;
		}

		SDL_Window* window = NULL;
		SDL_GLContext gContext = NULL;

		// Components
		PhysicsEngine* physicsEngine;
		UI* ui;
		Camera camera;
		Heightmap* heightfield;

		// Shaders
		BaseShader baseShader;
		BaseShader heightmap;
		TextShader textShader;
};
