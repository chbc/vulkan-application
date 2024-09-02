#pragma once

#include "SDLAPI.h"
#include <vector>

class VulkanAPI
{
public:
	void init(const SDLAPI& sdlApi);

private:
	void createInstance();
	bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
	void preRelease();
	void release();

friend class Platform;
};
