#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <iostream>
#include <memory>

#include "Window.hpp"
#include "ecs/ECS.hpp"

enum STATUS {
	SUCCESS = 0,
	WINDOW_INIT_FAIL = 1,
};

class Engine {
	public:
		Engine() : mainWindow(std::make_unique<Window>()) {}
		~Engine() = default;
		int init(const WindowCreationData& mainWindowData = WindowCreationData()) {
			if(!mainWindow->init(mainWindowData)){
				return WINDOW_INIT_FAIL;
			}

			return SUCCESS;
		}
		void mainLoop() {
			mainWindow->loop();
		}
	private:
		std::unique_ptr<Window> mainWindow;
		std::vector<std::unique_ptr<Window>> windows;

		// ECS modules
		EntityManager entityManager;
		ComponentManager compManager;
		SystemManager sysManager;

};
