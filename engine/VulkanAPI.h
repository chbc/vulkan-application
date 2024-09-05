/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "SDLAPI.h"
#include <vector>

typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

struct QueueFamilyIndices;

class VulkanAPI
{
private:
	std::vector<const char*> extensions;
	VkPhysicalDevice physicalDevice;

public:
	void init(const SDLAPI& sdlApi);

private:
	void createInstance();
	bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
	void getRequiredExtensions(const SDLAPI& sdlApi);
	void setupDebugMessenger();
	void pickPhysicalDevice();
	bool isDeviceSuitable(const VkPhysicalDevice& device);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
	void preRelease();
	void release();

friend class Platform;
};
