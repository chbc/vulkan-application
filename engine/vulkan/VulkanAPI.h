/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "engine/sdl/SDLAPI.h"
#include <vector>

namespace vk
{
	class PhysicalDevice;
	struct SurfaceFormatKHR;
	enum class PresentModeKHR;
	struct Extent2D;
	struct SurfaceCapabilitiesKHR;
	class ShaderModule;
	class CommandBuffer;
}

struct QueueFamilyIndices;
struct SwapChainSupportDetails;

class VulkanAPI
{
private:
	std::vector<const char*> extensions;
	std::vector<const char*> validationLayers;

public:
	void init(const SDLAPI& sdlApi);
	void drawFrame();

private:
	void createInstance();
	void setupDebugMessenger();
	void createSurface(const SDLAPI& sdlApi);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain(const SDLAPI& sdlApi);
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffer();
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();
	vk::ShaderModule createShaderModule(const std::vector<char>& code);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const SDLAPI& sdlApi, const vk::SurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device);
	bool isDeviceSuitable(const vk::PhysicalDevice& device);
	bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device);
	void getRequiredExtensions(const SDLAPI& sdlApi);
	bool checkValidationLayerSupport();
	void preRelease();
	void release();

friend class Platform;
};
