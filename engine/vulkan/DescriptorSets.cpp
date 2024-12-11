#include "DescriptorSets.h"

#include <vulkan/vulkan.hpp>

std::vector<vk::DescriptorSet> vkDescriptorSets;
vk::DescriptorSetLayout descriptorSetLayout;
vk::DescriptorPool descriptorPool;

void DescriptorSets::initLayout(vk::Device* logicalDevice)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
    vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };
    
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    
    vk::DescriptorSetLayoutCreateInfo LayoutInfo{ {}, bindings };

    descriptorSetLayout = logicalDevice->createDescriptorSetLayout(LayoutInfo);
}

void DescriptorSets::initPool(vk::Device* logicalDevice, uint32_t maxFramesInFlight)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes =
    { 
        vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, maxFramesInFlight },
        vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, maxFramesInFlight}
    };

    vk::DescriptorPoolCreateInfo poolInfo{ {} , maxFramesInFlight, poolSizes};
    descriptorPool = logicalDevice->createDescriptorPool(poolInfo);
}

void DescriptorSets::initDescriptorSet(vk::Device* logicalDevice, uint32_t maxFramesInFlight)
{
    std::vector<vk::DescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
    
    vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptorPool)
        .setDescriptorSetCount(maxFramesInFlight)
        .setSetLayouts(layouts);

    vkDescriptorSets = logicalDevice->allocateDescriptorSets(allocInfo);
}

vk::DescriptorSet& DescriptorSets::getDescriptorSet(uint32_t index)
{
    return vkDescriptorSets[index];
}

vk::PipelineLayoutCreateInfo DescriptorSets::createPipelineLayoutInfo()
{
    vk::PipelineLayoutCreateInfo result = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptorSetLayout)
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges(nullptr);

    return result;
}

void DescriptorSets::release(vk::Device* logicalDevice)
{
    logicalDevice->destroyDescriptorPool(descriptorPool);
    logicalDevice->destroyDescriptorSetLayout(descriptorSetLayout);
}
