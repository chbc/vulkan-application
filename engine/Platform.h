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
	uint16_t loadMesh();
	uint16_t loadTexture();
	void processInput(bool& stillRunning);
	void drawFrame();
	void processFrameEnd();
};
