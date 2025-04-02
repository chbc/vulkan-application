#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
#include "vk_forward_declarations.h"

#include <vector>
#include <memory>

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
	void drawFrame();

private:
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void createGraphicsPipeline();
	
	void createDescriptorSets();
	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	// Devices
	void Devices_init(const vk::Instance& instance, const vk::SurfaceKHR& surface, const ValidationLayers& validationLayers);
	void Devices_pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface);
	void Devices_createLogicalDevice(const ValidationLayers& validationLayers);
	vk::Device* Devices_getDevice();
	vk::PhysicalDevice* Devices_getPhysicalDevice();
	const vk::Queue* Devices_getGraphicsQueue();
	const vk::Queue* Devices_getPresentQueue();
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
	void Swapchain_createFramebuffers(vk::RenderPass& renderPass, vk::ImageView& depthImageView);
	vk::Format Swapchain_getImageFormat();
	const vk::Extent2D& Swapchain_getExtent();
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
	vk::RenderPassBeginInfo RenderPass_createInfo(const vk::Extent2D& extent, uint32_t imageIndex);
	vk::RenderPass& RenderPass_getRenderPassRef();
	void RenderPass_release(vk::Device* logicalDevice);
	//

	// DescriptorSets
	void DescriptorSets_initLayout(vk::Device* logicalDevice);
	void DescriptorSets_initPool(vk::Device* logicalDevice, uint32_t maxFramesInFlight);
	void DescriptorSets_initDescriptorSet(vk::Device* logicalDevice, uint32_t maxFramesInFlight);
	vk::DescriptorSet& DescriptorSets_getDescriptorSet(uint32_t index);
	vk::PipelineLayoutCreateInfo DescriptorSets_createPipelineLayoutInfo();
	void DescriptorSets_release(vk::Device* logicalDevice);
	//

	// CommandBuffers
	size_t loadModel(const char* filePath);
	void loadTexture(const char* filePath);
	void updateDescriptorSets();
	void CommandBuffers_init(const vk::SurfaceKHR& surface);
	void CommandBuffers_createCommandPool(vk::Device* logicalDevice, uint32_t queueFamilyIndex);
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
	void CommandBuffers_createUniformBuffers();
	void CommandBuffers_createCommandBuffers(vk::Device* logicalDevice, int maxFramesInFlight);
	uint32_t CommandBuffers_getCurrentFrameIndex();
	vk::ImageView& CommandBuffers_getDepthImageView();
	const vk::CommandBuffer* CommandBuffers_getCurrentCommandBuffer();
	void CommandBuffers_copyBuffer(vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size);
	void CommandBuffers_createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void CommandBuffers_recordCommandBuffer(const vk::Extent2D& swapchainExtent, vk::RenderPassBeginInfo& renderPassInfo,
		const vk::Pipeline& graphicsPipeline, const vk::PipelineLayout& pipelineLayout, const vk::DescriptorSet* descriptorSets);
	void CommandBuffers_updateUniformBuffer(const vk::Extent2D& swapchainExtent);
	vk::CommandBuffer CommandBuffers_beginSingleTimeCommands(vk::Device* logicalDevice);
	void CommandBuffers_endSingleTimeCommands(vk::CommandBuffer& commandBuffer);
	void CommandBuffers_increaseFrame(int maxFramesInFlight);
	void CommandBuffers_createDescriptorsBufferInfo(size_t index, vk::DescriptorBufferInfo& bufferInfo, vk::DescriptorImageInfo& imageInfo);
	void CommandBuffers_releaseUniformBuffers(vk::Device* logicalDevice, size_t maxFramesInFlight);
	void CommandBuffers_release(vk::Device* logicalDevice);
	void CommandBuffers_releaseDepthImages(vk::Device* logicalDevice);
	//

	// SyncObjects
	void SyncObjects_init(vk::Device* logicalDevice, int maxFramesInFlight);
	const vk::Semaphore& SyncObjects_getImageSemaphore(int currentFrame);
	const vk::Semaphore& SyncObjects_getRenderSemaphore(int currentFrame);
	const vk::Fence& SyncObjects_getInFlightFence(int currentFrame);
	void SyncObjects_waitForFence(vk::Device* logicalDevice, int currentFrame);
	void SyncObjects_resetFence(vk::Device* logicalDevice, int currentFrame);
	void SyncObjects_release(vk::Device* logicalDevice, int maxFramesInFlight);
	//

	void preRelease();
	void release();

friend class Platform;
};
