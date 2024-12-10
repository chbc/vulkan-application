#include "Devices.h"
#include "ValidationLayers.h"
#include "Swapchain.h"

#include <vulkan/vulkan.hpp>
#include <set>

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void Devices::init(const vk::Instance& instance, const vk::SurfaceKHR& surface, const ValidationLayers& validationLayers)
{
    this->pickPhysicalDevice(instance, surface);
    this->createLogicalDevice(validationLayers);
}

void Devices::pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface)
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const vk::PhysicalDevice& device : devices)
    {
        if (isDeviceSuitable(surface, device))
        {
            this->physicalDevice.reset(new vk::PhysicalDevice{ device });
            break;
        }
    }

    if (!this->physicalDevice)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Devices::createLogicalDevice(const ValidationLayers& validationLayers)
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { this->familyIndices.graphicsFamily.value(), this->familyIndices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queueFamily)
            .setQueueCount(1)
            .setQueuePriorities(queuePriority);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures()
        .setSamplerAnisotropy(vk::True);

    vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
        .setPQueueCreateInfos(queueCreateInfos.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledLayerNames(validationLayers.getData());

    this->logicalDevice.reset(new vk::Device{ physicalDevice->createDevice(createInfo) });

    if (!this->logicalDevice)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vk::Queue graphicsQueueValue = this->logicalDevice->getQueue(familyIndices.graphicsFamily.value(), 0);
    vk::Queue presentQueueValue = this->logicalDevice->getQueue(familyIndices.presentFamily.value(), 0);
    this->graphicsQueue = std::make_shared<vk::Queue>(graphicsQueueValue);
    this->presentQueue = std::make_shared<vk::Queue>(presentQueueValue);
}

vk::Device* Devices::getDevice()
{
    return this->logicalDevice.get();
}

vk::PhysicalDevice* Devices::getPhysicalDevice()
{
    return this->physicalDevice.get();
}

const vk::Queue* Devices::getGraphicsQueue()
{
    return this->graphicsQueue.get();
}

const vk::Queue* Devices::getPresentQueue()
{
    return this->presentQueue.get();
}

uint32_t Devices::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice->getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

bool Devices::isDeviceSuitable(const vk::SurfaceKHR& surface, const vk::PhysicalDevice& device)
{
    QueueFamilyIndices indices = findQueueFamilies(surface, &device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    if (extensionsSupported)
    {
        swapChainAdequate = Swapchain::isSwapChainAdequate(surface, &device);
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    bool result =   indices.isComplete() && extensionsSupported && 
                    swapChainAdequate && supportedFeatures.samplerAnisotropy;
    if (result)
    {
        this->familyIndices = indices;
    }

    return result;
}

QueueFamilyIndices Devices::findQueueFamilies(const vk::SurfaceKHR& surface)
{
    return this->findQueueFamilies(surface, this->physicalDevice.get());
}

QueueFamilyIndices Devices::findQueueFamilies(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device->getQueueFamilyProperties();

    for (int i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        if (device->getSurfaceSupportKHR(i, surface) > 0)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
    }

    return indices;
}

bool Devices::checkDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const vk::ExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

vk::ImageView Devices::createImageView(vk::Image& image, vk::Format format)
{
    vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
        .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    vk::ImageView result = this->logicalDevice->createImageView(viewInfo);

    if (result == nullptr)
    {
        throw std::runtime_error("Failed to create image views!");
    }

    return result;
}
