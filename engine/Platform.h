#pragma once

#include "vulkan/VulkanAPI.h"

class Platform
{
private:
	SDLAPI sdlApi;
	VulkanAPI vulkanApi;

public:
	~Platform();
	void init();
	void processInput(bool& stillRunning);
	void drawFrame();
	void processFrameEnd();
};
