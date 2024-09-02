#pragma once

#include "SDLAPI.h"

class VulkanAPI
{
public:
	void init(const SDLAPI& sdlApi);

private:
	void createInstance();
	void preRelease();
	void release();

friend class Platform;
};
