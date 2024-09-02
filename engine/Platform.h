#pragma once

#include "VulkanAPI.h"

class Platform
{
private:
	SDLAPI sdlApi;
	VulkanAPI vulkanApi;

public:
	~Platform();
	void init();
	void processInput(bool& stillRunning);
	void processFrameEnd();
};
