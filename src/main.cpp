#include <iostream>
#include <string_view>

#include "include/Window.hpp"
#include "include/Util.hpp"

int main(int argc, char** argv) {
	std::cout << "start" << std::endl;

	WindowCreationData windowData = {
		.minFrameTime = 0,
	};

	{	// Command line arguement parsing
		Util::CMDParser cmdArgs(argc, argv);

		// Vsync
		if(cmdArgs.hasOption("--vsync")){
			windowData.flags.vsync(1);
		} else {
			windowData.flags.vsync(0);
		}

		// Width
		std::string_view option = cmdArgs.getOption(std::pair(
			std::string_view("--width"),
			std::string_view("-w")
		));
		if(option.compare("") != 0){
			windowData.width = std::stoi(option.data());
		}

		// Height
		option = cmdArgs.getOption(std::pair(
			std::string_view("--height"),
			std::string_view("-h")
		));
		if(option.compare("") != 0){
			windowData.height = std::stoi(option.data());
		}
	}

	Window* win = new Window(Util::getWorkingDir());
	if(!win->init(windowData)) return 1;
	win->loop();
	delete win;

	std::cout << "end" <<std::endl;
	return 0;
}
