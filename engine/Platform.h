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
	void loadModel(const char* filePath);
	void loadTexture(const char* filePath);
	void updateDescriptorSets();
	void processInput(bool& stillRunning);
	void drawFrame();
	void processFrameEnd();
};
