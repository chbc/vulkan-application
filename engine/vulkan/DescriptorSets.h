#pragma once

#include "vk_forward_declarations.h"

#include <memory>
#include <vector>

class DescriptorSets
{
private:
	void initLayout(vk::Device* logicalDevice);
	void initPool(vk::Device* logicalDevice, uint32_t maxFramesInFlight);
	void initDescriptorSet(vk::Device* logicalDevice, uint32_t maxFramesInFlight);
	vk::DescriptorSet& getDescriptorSet(uint32_t index);
	vk::PipelineLayoutCreateInfo createPipelineLayoutInfo();
	void release(vk::Device* logicalDevice);

friend class VulkanAPI;
};
