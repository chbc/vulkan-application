#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
#include "vk_forward_declarations.h"

#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>

struct SwapChainSupportDetails;
struct QueueFamilyIndices;
struct Model;
struct ModelBuffers;

class VulkanAPI
{
private:
	SDLAPI *sdlApi;
	DebugMessenger debugMessenger;
	ValidationLayers validationLayers;
	std::vector<std::shared_ptr<ModelBuffers>> modelBuffersMap;

	bool framebufferResized = false;

public:
	void init(SDLAPI& sdlApi);
	void beginDraw();
	void updateTransform(size_t modelId, const glm::mat4& transform);
	void drawItem(size_t modelId);
	void endDraw();

private:
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void createGraphicsPipeline();
	
	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	// Devices
	void Devices_pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface);
	void Devices_createLogicalDevice(const ValidationLayers& validationLayers);
	uint32_t Devices_findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	vk::Format Devices_findDepthFormat();
	vk::Format Devices_findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	bool Devices_hasStencilComponent(vk::Format format);
	bool Devices_isDeviceSuitable(const vk::SurfaceKHR& surface, const vk::PhysicalDevice& device);
	QueueFamilyIndices Devices_findQueueFamilies(const vk::SurfaceKHR& surface);
	QueueFamilyIndices Devices_findQueueFamilies(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
	bool Devices_checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
	vk::ImageView Devices_createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	//

	// Swapchain
	void Swapchain_init(const vk::SurfaceKHR& surface, SDL_Window* window);
	void Swapchain_createImageViews();
	void Swapchain_getFramebuffer(uint32_t index, vk::Framebuffer& result);
	static SwapChainSupportDetails Swapchain_querySwapChainSupport(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
	static bool Swapchain_isSwapChainAdequate(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
	vk::SurfaceFormatKHR Swapchain_chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR Swapchain_chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D Swapchain_chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
	void Swapchain_recreate(const vk::SurfaceKHR& surface, SDL_Window* window);
	void Swapchain_release();
	//

	// RenderPass
	void RenderPass_init();
	//

	// DescriptorSets
	void DescriptorSets_initLayout();
	void DescriptorSets_initPool();
	void DescriptorSets_initDescriptorSet();
	vk::PipelineLayoutCreateInfo DescriptorSets_createPipelineLayoutInfo();
	void DescriptorSets_release();
	//

	// CommandBuffers
	size_t loadModel(const char* filePath);
	void loadTexture(const char* filePath);
	void updateDescriptorSets(size_t modelId);
	void CommandBuffers_init(const vk::SurfaceKHR& surface);
	void CommandBuffers_createCommandPool(uint32_t queueFamilyIndex);
	void CommandBuffers_createDepthResources();
	void CommandBuffers_recreateDepthResources();
	void CommandBuffers_createImage(uint32_t widith, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory);
	void CommandBuffers_transitionImageLayout(vk::Image& image, vk::Format format,
		vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CommandBuffers_copyBufferToImage(vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height);
	void CommandBuffers_createTextureSampler();
	void CommandBuffers_createVertexBuffer(const Model& model, ModelBuffers* modelBuffers);
	void CommandBuffers_createIndexBuffer(const Model& model, ModelBuffers* modelBuffers);
	void CommandBuffers_createUniformBuffers(ModelBuffers* modelBuffers);
	void CommandBuffers_createCommandBuffers();
	uint32_t CommandBuffers_getCurrentFrameIndex();
	void CommandBuffers_copyBuffer(vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size);
	void CommandBuffers_createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	vk::CommandBuffer CommandBuffers_beginSingleTimeCommands();
	void CommandBuffers_endSingleTimeCommands(vk::CommandBuffer& commandBuffer);
	void CommandBuffers_increaseFrame(int maxFramesInFlight);
	void CommandBuffers_releaseUniformBuffers();
	void CommandBuffers_release();
	void CommandBuffers_releaseDepthImages();
	//

	// SyncObjects
	void SyncObjects_init();
	void SyncObjects_waitForFence(int currentFrame);
	void SyncObjects_resetFence(int currentFrame);
	void SyncObjects_release();
	//

	void preRelease();
	void release();

friend class Platform;
};
