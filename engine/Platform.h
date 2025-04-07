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
	size_t loadModel(const char* filePath);
	void loadTexture(const char* filePath);
	void updateDescriptorSets(size_t modelId);
	void processInput(bool& stillRunning);
	void beginDraw();
	void updateTransform(size_t modelId, const glm::mat4& transform);
	void drawItem(size_t itemId);
	void endDraw();
	void processFrameEnd();
};
