/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
#include "Devices.h"
#include "SwapChain.h"

#include <vector>

struct SwapChainSupportDetails;

class VulkanAPI
{
private:
	SDLAPI *sdlApi;
	DebugMessenger debugMessenger;
	ValidationLayers validationLayers;
	Devices devices;
	Swapchain swapchain;

public:
	void init(SDLAPI& sdlApi);
	void drawFrame();

private:
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, 
		vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void createRenderPass();
	void createGraphicsPipeline();

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
	void preRelease();
	void release();

friend class Platform;
};
