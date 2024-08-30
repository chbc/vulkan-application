#pragma once

#include <SDL2/SDL.h>

class SDLAPI
{
private:
	static SDL_Window* window;

private:
	static bool init(SDL_WindowFlags windowFlags);
	static void processInput(bool& stillRunning);
	static void processFrameEnd();
	static void release();

friend class Platform;
friend class VulkanAPI;
};
