#pragma once

class SDLAPI
{
private:
	struct SDL_Window* window;
	bool initialized = false;

private:
	void init(int windowFlags);
	void processInput(bool& stillRunning);
	void processFrameEnd();

	void release();

friend class Platform;
friend class VulkanAPI;
};
