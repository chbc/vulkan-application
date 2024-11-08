#include "Devices.h"
#include "ValidationLayers.h"

#include <vulkan/vulkan.hpp>
#include <set>

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

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
            physicalDevice.reset(new vk::PhysicalDevice{ device });
            break;
        }
    }

    if (!physicalDevice)
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

    vk::PhysicalDeviceFeatures deviceFeatures{};
    vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
        .setPQueueCreateInfos(queueCreateInfos.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledLayerNames(validationLayers.getData());

    logicalDevice.reset(new vk::Device{ physicalDevice->createDevice(createInfo) });

    if (!logicalDevice)
    {
        throw std::runtime_error("failed to create logical device!");
    }
}

vk::Device* Devices::getDevice()
{
    return this->logicalDevice.get();
}

void Devices::getQueues(vk::Queue& graphics, vk::Queue& present)
{
    graphics = this->logicalDevice->getQueue(familyIndices.graphicsFamily.value(), 0);
    present = this->logicalDevice->getQueue(familyIndices.presentFamily.value(), 0);
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
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, &device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    bool result = indices.isComplete() && extensionsSupported && swapChainAdequate;
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

SwapChainSupportDetails Devices::querySwapChainSupport(const vk::SurfaceKHR& surface)
{
    return this->querySwapChainSupport(surface, this->physicalDevice.get());
}

SwapChainSupportDetails Devices::querySwapChainSupport(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    SwapChainSupportDetails details;
    details.capabilities = device->getSurfaceCapabilitiesKHR(surface);
    details.formats = device->getSurfaceFormatsKHR(surface);
    details.presentModes = device->getSurfacePresentModesKHR(surface);

    return details;
}
