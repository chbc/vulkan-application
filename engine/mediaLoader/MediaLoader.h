#pragma once

#include "engine/vulkan/Model.h"

namespace MediaLoader
{
	void loadModel(const char* filePath, Model& result);
	unsigned char* loadTexture(const char* filePath, int& texWidth, int& texHeight);
	void freePixels(void* pixels);
}
