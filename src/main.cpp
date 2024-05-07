#include <iostream>

#include "include/Window.hpp"

int main(int argc, char *argv[]) {
	std::cout << "start" << std::endl;

	Window* win = new Window(640, 480);
	if(!win->create()) return 1;
	win->loop();
	delete win;
	
	std::cout << "end" <<std::endl;
	return 0;

}