/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "SDLAPI.h"
#include <vector>

namespace vk
{
	class PhysicalDevice;
}

struct QueueFamilyIndices;

class VulkanAPI
{
private:
	std::vector<const char*> extensions;
	std::vector<const char*> validationLayers;

public:
	void init(const SDLAPI& sdlApi);

private:
	void createInstance();
	bool checkValidationLayerSupport();
	void getRequiredExtensions(const SDLAPI& sdlApi);
	void setupDebugMessenger();
	void createSurface(const SDLAPI& sdlApi);
	void pickPhysicalDevice();
	void createLogicalDevice();
	bool isDeviceSuitable(const vk::PhysicalDevice& device);
	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device);
	void preRelease();
	void release();

friend class Platform;
};
