/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
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
	SDLAPI *sdlApi;
	DebugMessenger debugMessenger;
	ValidationLayers validationLayers;

public:
	void init(SDLAPI& sdlApi);
	void drawFrame();

private:
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();
	void updateUniformBuffer(uint32_t currentImage);
	vk::ShaderModule createShaderModule(const std::vector<char>& code);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device);
	bool isDeviceSuitable(const vk::PhysicalDevice& device);
	bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device);
	void cleanupSwapChain();
	void recreateSwapChain();
	void preRelease();
	void release();

friend class Platform;
};
