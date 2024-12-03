#pragma once

#include <vector>
#include <memory>
#include "Vertex.h"
#include "vk_forward_declarations.h"

class Devices;

class CommandBuffers
{
private:
	std::shared_ptr<vk::CommandPool> commandPool;
	std::vector<std::shared_ptr<vk::CommandBuffer>> commandBuffers;
	uint32_t currentFrame = 0;
	const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
	const std::vector<Vertex> vertices =
	{
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

private:
	void init(const vk::SurfaceKHR& surface, Devices& devices, int maxFramesInFlight);
	void createUniformBuffers(Devices& devices, int maxFramesInFlight);
	void createVertexBuffer(Devices& devices);
	void createIndexBuffer(Devices& devices);
	void createCommandBuffers(Devices& devices, int maxFramesInFlight);
	uint32_t getCurrentFrameIndex();
	const vk::CommandBuffer* getCurrentCommandBuffer();
	void copyBuffer(Devices& devices, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size);
	void createBuffer(Devices& devices, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	void recordCommandBuffer(const vk::Extent2D& swapchainExtent, const vk::RenderPassBeginInfo& renderPassInfo,
		const vk::Pipeline& graphicsPipeline, const vk::PipelineLayout& pipelineLayout, const vk::DescriptorSet* descriptorSets);
	void updateUniformBuffer(const vk::Extent2D& swapchainExtent);
	void increaseFrame(int maxFramesInFlight);
	vk::DescriptorBufferInfo createDescriptorBufferInfo(size_t index);
	void releaseUniformBuffers(vk::Device* logicalDevice, size_t maxFramesInFlight);
	void release(vk::Device* logicalDevice);

friend class VulkanAPI;
};
