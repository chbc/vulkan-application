#pragma once

#include "vk_forward_declarations.h"

#include <memory>

class DescriptorSets
{
private:
	std::shared_ptr<vk::DescriptorSetLayout> descriptorSetLayout;
	std::shared_ptr<vk::DescriptorPool> descriptorPool;
	std::shared_ptr<vk::DescriptorSet> descriptorSet;

private:
	void initLayout(vk::Device* logicalDevice);
	void initPool(vk::Device* logicalDevice);
	void initDescriptorSet(vk::Device* logicalDevice);
	vk::DescriptorSet* getDescriptorSet();
	vk::PipelineLayoutCreateInfo createPipelineLayoutInfo();
	void release(vk::Device* logicalDevice);

friend class VulkanAPI;
};
