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

#include "ObjectHandler.hpp"
#include "shader/BaseShader.hpp"
#include "Camera.hpp"
#include "UI.hpp"

class Window {
	public:
		/**
		 * @brief Default constructor
		 * @note Sets window to 640x480
		 * @note Sets sensitivity to 0.1
		 * @note Sets frame time to 16.6667
		*/
		Window() : SENSITIVITY(0.1f), MIN_FRAME_TIME(16.66666667f) {
			width = 640;
			height = 480;
			zBuffer = false;
		}
		/**
		 * @brief Constructor
		 * @param width The width of the window in pixels
		 * @param height The height of the window in pixels
		 * @note Sets sensitivity to 0.1
		 * @note Sets frame time to 16.6667
		*/
		Window(const int width, const int height) : SENSITIVITY(0.1f), MIN_FRAME_TIME(16.66666667f) {
			this->width = width;
			this->height = height;
			zBuffer = false;
		}
		/**
		 * @brief Constructor
		 * @param width The width of the window in pixels
		 * @param height The height of the window in pixels
		 * @param sensitivity The sensitivity of the mouse
		 * @param frameTime The minimum frame time in miliseconds
		*/
		Window(const int width, const int height, const float sensitivity, const float frameTime) 
			: SENSITIVITY(sensitivity), MIN_FRAME_TIME(frameTime){
			this->width = width;
			this->height = height;
		}
		~Window() {
			baseShader.freeProgram();
			SDL_DestroyWindow(window);		// Delete window

			SDL_Quit();	// Quit SDL
		}
		/**
		 * @brief Initializes subsystems and creates the window and other resources
		 * @return A bool whether the creation was successful or not
		 */		
		bool create() {
			// Init Video and Events Subsystems
			if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0){
				std::cout << "Unable to initialize SDL_VIDEO or SDL_EVENTS, SDL_Error: " << SDL_GetError() << std::endl;
				return false;
			}
			
			// Use OpenGL 3.1 core
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			
			// Other OpenGL settings
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

			// Create window 
			window = SDL_CreateWindow(
				"Physics Engine", 
				SDL_WINDOWPOS_UNDEFINED, 
				SDL_WINDOWPOS_UNDEFINED, 
				width, 
				height, 
				SDL_WINDOW_SHOWN |
				SDL_WINDOW_RESIZABLE |
				SDL_WINDOW_OPENGL 
			);
			if(window == NULL){
				std::cout << "Unable to create window, SDL_Error: " << SDL_GetError() << std::endl;
				return false;
			}

			// Create OpenGL context
			gContext = SDL_GL_CreateContext(window);
			if(gContext == NULL){
				std::cout << "Unable to create OpenGL context, SDL_Error: " << SDL_GetError() << std::endl;
				return false;
			}
			
			// Initialize Glew
			glewExperimental = GL_TRUE;
			GLenum glewStatus = glewInit();
			if (glewStatus != GLEW_OK){
				std::cout << "Unable to initialize GLEW, GLEW Error: " << glewGetErrorString(glewStatus) << std::endl;
				return false;
			}

			// TODO Creat program args function to enable vsync
			// Enable VSync
			SDL_GL_SetSwapInterval(1);
			if(SDL_GL_GetSwapInterval() != 1){
				std::cout << "Warning: Unable to enable VSync, SDL_Error: " << SDL_GetError() << std::endl;
			}

			// Initialize shared variables
			delta_t.reset(new Uint32(0));
			pause.reset(new bool(true));

			// Initialize OpenGL
			if(!initOpenGL()){
				std::cout << "Unable to initialize OpenGL" << std::endl;
				return false;
			}
			
			keyboard = SDL_GetKeyboardState(NULL);	// Get pointer to internal keyboard state

			paused = true;
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_ShowCursor(SDL_TRUE);

			return true;
		}
		/// Initializes OpenGL components
		bool initOpenGL() {
			GLint status = GL_FALSE;

			glViewport(0, 0, width, height);
			glEnable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(!baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag")){
				std::cerr << "Unable to load model shader" << std::endl;
				return false;
			}
			if(!textShader.loadProgram("../shaders/text.vert", "../shaders/text.frag")){
				std::cerr << "Unable to load UI text shader" << std::endl;
				return false;
			}

			ObjectHandler::newGameObject<GameObject>("../assets/backpack/backpack.obj", {});
			
			ui = new UI();
			ui->addTextElement(std::make_unique<Text>(Text("help me", { 320.f, 240.f }, { 1.f, 0.f, 0.f }, 1.f, true)));
			ui->addTextElement(std::make_unique<DynamicText>(Text("Frametime: <%>", { 1.f, 2.f }, { 1.f, 1.f, 1.f }, 0.25f, true), delta_t, [](DynamicText& e){ }));
			ui->addTextElement(std::make_unique<DynamicText>(
				Text("Paused", { 1.f, 15.f }, { 1.f, 0.f, 0.f }, 0.25f, true), 
				pause, 
				[](DynamicText& e){
					if(e.dynamicVal.expired()) return;
					e.visible = *std::static_pointer_cast<bool>(e.dynamicVal.lock());
				}
			));

			GLenum err = glGetError(); 
			if(err != GL_NO_ERROR) {
				std::cout << "Unhandled OpenGL Error: " << err << std::endl;
				return false;
			}
			return true;
		}
		/// Main loop
		void loop() {
			SDL_Event event;
			Uint32 windowID = SDL_GetWindowID(window);

			// Main Event Loop
			while(true) {
				while(SDL_PollEvent(&event)) {
					// Main Event Handler
					switch(event.type) {
						case SDL_WINDOWEVENT: {
							// Window event is for this window(useful if application uses more than one window)
							if(event.window.windowID == windowID){
								switch(event.window.event) {
									case SDL_WINDOWEVENT_CLOSE:	// Window receives close command
										// Destroy objects
										baseShader.freeProgram();
										SDL_DestroyWindow(window);

										// Push quit message
										event.type = SDL_QUIT;
										SDL_PushEvent(&event);
										break;
									case SDL_WINDOWEVENT_EXPOSED:
										//render();	// Render
										//SDL_GL_SwapWindow(window);	// Update
										break;
									case SDL_WINDOWEVENT_RESIZED:
										resize();
									default:
										break;
								}
							}
							break;
						} case SDL_KEYDOWN: {
							// Toggle mouse visibility and capture state with escape key
							if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
								paused = !paused;
								*pause = !(*pause);
								
								SDL_SetRelativeMouseMode((SDL_bool)(!paused));
								SDL_ShowCursor((SDL_bool)(paused));
							}
							if(event.key.keysym.scancode == SDL_SCANCODE_LALT && !paused){
								zBuffer = !zBuffer;
								if(zBuffer){
									if(!baseShader.loadProgram("../shaders/texture.vert", "../shaders/zBuffer.frag")){
										std::cerr << "Unable to load z-buffer shader" << std::endl;
										baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag");
										zBuffer = false;
									}
								} else {
									baseShader.loadProgram("../shaders/texture.vert", "../shaders/pureTexture.frag");
								}
							}
							break;
						} case SDL_MOUSEMOTION: {
							if(paused) break;
							float offsetX = event.motion.xrel;
							float offsetY = event.motion.yrel;

							// Adjust for sensitivity
							offsetX *= SENSITIVITY;
							offsetY *= SENSITIVITY;

							camera.incYaw(offsetX);
							camera.incPitch(-offsetY);
						} case SDL_MOUSEBUTTONDOWN: {
							mouseButtonState = SDL_GetMouseState(NULL, NULL);	// Get buttons

							switch(mouseButtonState) {
								case SDL_BUTTON(1):
									break;
								case SDL_BUTTON(2):
									break;
								case SDL_BUTTON(3):
									break;
							}
							break;

						} case SDL_MOUSEWHEEL: {
							if(paused) break;
							
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
				*delta_t = currentTime - prevTime;
				if(SDL_GL_GetSwapInterval() != 1){
					SDL_Delay((*delta_t < MIN_FRAME_TIME) ? MIN_FRAME_TIME - *delta_t : 0);
				}
				prevTime = SDL_GetTicks();
			}
		}
		/// Resize Window
		// TODO Implement real resizing/keep ratio of drawable items
		void resize() {
			SDL_GL_GetDrawableSize(window, &width, &height);	// Set 'width' and 'height'

			glViewport(0, 0, width, height);	// Update OpenGL viewport
		}
		/// Run Logic
		void tick() {
			// Constant Logic
			
			// Runtime Logic
			if(paused){	// Paused Logic

			} else {	// Runtime Logic
				if(keyboard[SDL_SCANCODE_E]){
					camera.incRoll(1.5f * *delta_t / 1000);		// Roll right(increase)
				} else if(keyboard[SDL_SCANCODE_Q]) {
					camera.incRoll(-1.5f * *delta_t / 1000);	// Roll left(decrease)
				}
				// Roll
				if(keyboard[SDL_SCANCODE_E]){
					camera.incRoll(1.5f * *delta_t / 1000);		// Roll right(increase)
				} else if(keyboard[SDL_SCANCODE_Q]) {
					camera.incRoll(-1.5f * *delta_t / 1000);	// Roll left(decrease)
				}

				// Camera position for view calculations
				camera.updateCameraPosition(
					keyboard[SDL_SCANCODE_W], 
					keyboard[SDL_SCANCODE_S], 
					keyboard[SDL_SCANCODE_A], 
					keyboard[SDL_SCANCODE_D], 
					keyboard[SDL_SCANCODE_SPACE], 
					keyboard[SDL_SCANCODE_LCTRL], 
					*delta_t
				);
				camera.updateCameraDirection();
			}
		}
		
		/// Render
		void render() {
			glClearColor(0.02f, 0.02f, 0.02f, 0.f);	// Set clear color
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Draw all objects
			for(long long unsigned int i = 0; i < ObjectHandler::objectList.size(); i++) {
				ObjectHandler::objectList[i].get()->draw(baseShader, camera.calcCameraView(), camera.getFOV());
			}
						
			ui->drawTextElements(textShader);
			
			glUseProgram(0);	// Unbind
			
			glFlush();
			SDL_GL_SwapWindow(window);	// Update
			glFinish();
		}
	private:
		SDL_Window* window = NULL;		// The window
		SDL_Renderer* renderer = NULL;	// The renderer for the window, uses hardware acceleration
		SDL_GLContext gContext = NULL;	// The OpenGL context

		// Running Window Variables
		int width;			// The running drawable window width
		int height;			// The running drawable window width
		Uint32 prevTime;	// The running time from init last frame was
		bool zBuffer;
		bool paused;

		// Shared variables
		std::shared_ptr<Uint32> delta_t;	// The running time between frames
		std::shared_ptr<bool> pause;

		// Running Mouse Variables
		Uint32 mouseButtonState;	// Mouse buttons state

		// Constants
		const Uint8* keyboard;		// The running state of the keyboard
		const float SENSITIVITY;	// Mouse sensitivity
		const float MIN_FRAME_TIME;	// Minimum frame time in ms


		// Objects
		BaseShader baseShader;
		TextShader textShader;
		Camera camera;
		UI* ui;
		short uid;
};