#include "DescriptorSets.h"

#include <vulkan/vulkan.hpp>

void DescriptorSets::initLayout(vk::Device* logicalDevice)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
    vk::DescriptorSetLayoutCreateInfo createInfo{ {}, uboLayoutBinding };
    vk::DescriptorSetLayout layout = logicalDevice->createDescriptorSetLayout(createInfo);
    descriptorSetLayout = std::make_shared<vk::DescriptorSetLayout>(layout);
}

void DescriptorSets::initPool(vk::Device* logicalDevice)
{
    vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
    vk::DescriptorPoolCreateInfo createInfo{ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSize };
    vk::DescriptorPool pool = logicalDevice->createDescriptorPool(createInfo);
    descriptorPool = std::make_shared<vk::DescriptorPool>(pool);
}

void DescriptorSets::initDescriptorSet(vk::Device* logicalDevice)
{
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ *descriptorPool.get(), *descriptorSetLayout.get() };
    vk::DescriptorSet set = logicalDevice->allocateDescriptorSets(descriptorSetAllocateInfo).front();
    descriptorSet = std::make_shared<vk::DescriptorSet>(set);
}

vk::DescriptorSet* DescriptorSets::getDescriptorSet()
{
    return this->descriptorSet.get();
}

vk::PipelineLayoutCreateInfo DescriptorSets::createPipelineLayoutInfo()
{
    vk::PipelineLayoutCreateInfo result = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(this->descriptorSetLayout.get())
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges(nullptr);

    return result;
}

void DescriptorSets::release(vk::Device* logicalDevice)
{
    logicalDevice->destroyDescriptorPool(*this->descriptorPool.get());
    logicalDevice->destroyDescriptorSetLayout(*this->descriptorSetLayout.get());
}
