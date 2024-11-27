#pragma once

#include "vk_forward_declarations.h"

#include <memory>
#include <optional>

class ValidationLayers;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails;

class Devices
{
private:
	std::shared_ptr<vk::PhysicalDevice> physicalDevice;
	std::shared_ptr<vk::Device> logicalDevice;
    QueueFamilyIndices familyIndices;

private:
    void init(const vk::Instance& instance, const vk::SurfaceKHR& surface, const ValidationLayers& validationLayers);
    void pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface);
    void createLogicalDevice(const ValidationLayers& validationLayers);
    vk::Device* getDevice();
    vk::PhysicalDevice* getPhysicalDevice();
    void getQueues(vk::Queue& graphics, vk::Queue& present);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    bool isDeviceSuitable(const vk::SurfaceKHR& surface, const vk::PhysicalDevice& device);
    QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface);
    QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
    bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);

friend class VulkanAPI;
friend class Swapchain;
};
