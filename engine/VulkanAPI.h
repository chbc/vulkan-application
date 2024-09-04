#pragma once

#include "SDLAPI.h"
#include <vector>

class VulkanAPI
{
private:
	std::vector<const char*> extensions;

public:
	void init(const SDLAPI& sdlApi);

private:
	void createInstance();
	bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
	void getRequiredExtensions(const SDLAPI& sdlApi);
	void setupDebugMessenger();
	void preRelease();
	void release();

friend class Platform;
};
