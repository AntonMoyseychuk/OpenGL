#include "application.hpp"

int main(int argc, char* argv[]) {
	application& app = application::get("OpenGL Sandbox", 720, 640);
	app.run();
	return 0;
}