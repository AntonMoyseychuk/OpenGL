#include "application.hpp"

int main(int argc, char* argv[]) {
	application& app = application::get("OpenGL Sandbox", 1280, 900);
	app.run();
	return 0;
}