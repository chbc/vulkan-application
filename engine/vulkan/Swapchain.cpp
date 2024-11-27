#include "Swapchain.h"
#include "Devices.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

std::vector<vk::Image> swapchainImages;
vk::Format swapchainImageFormat;
vk::Extent2D swapchainExtent;
std::vector<vk::ImageView> swapchainImageViews;
std::vector<vk::Framebuffer> swapchainFramebuffers;

void Swapchain::init(const vk::SurfaceKHR& surface, SDL_Window* window, Devices& devices)
{
    SwapChainSupportDetails swapchainSupport = this->querySwapChainSupport(surface, devices.getPhysicalDevice());

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if ((swapchainSupport.capabilities.maxImageCount > 0) && (imageCount > swapchainSupport.capabilities.maxImageCount))
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(imageCount)
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(swapchainSupport.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setClipped(vk::True)
        .setOldSwapchain(nullptr);

    QueueFamilyIndices indices = devices.findQueueFamilies(surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        createInfo.setQueueFamilyIndexCount(2);
        createInfo.setPQueueFamilyIndices(queueFamilyIndices);
    }
    else
    {
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        createInfo.setQueueFamilyIndexCount(0);
        createInfo.setPQueueFamilyIndices(nullptr);
    }

    vk::Device* logicalDevice = devices.getDevice();
    swapchainKHR = std::make_shared<vk::SwapchainKHR>(logicalDevice->createSwapchainKHR(createInfo));
    swapchainImages = logicalDevice->getSwapchainImagesKHR(*swapchainKHR);
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}

void Swapchain::createImageViews(Devices& devices)
{
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        vk::ComponentMapping components
        {
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
        };

        vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
            .setImage(swapchainImages[i])
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(swapchainImageFormat)
            .setComponents(components)
            .setSubresourceRange(subresourceRange);

        swapchainImageViews[i] = devices.getDevice()->createImageView(createInfo);
        if (swapchainImageViews[i] == nullptr)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void Swapchain::createFramebuffers(Devices& devices, vk::RenderPass& renderPass)
{
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++)
    {
        vk::ImageView attachments[] = { swapchainImageViews[i] };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
            .setRenderPass(renderPass)
            .setAttachmentCount(1)
            .setPAttachments(attachments)
            .setWidth(swapchainExtent.width)
            .setHeight(swapchainExtent.height)
            .setLayers(1);

        swapchainFramebuffers[i] = devices.getDevice()->createFramebuffer(framebufferInfo);
    }
}

vk::SwapchainKHR* Swapchain::getSwapchainKHR()
{
    return swapchainKHR.get();
}

vk::Format Swapchain::getImageFormat()
{
    return swapchainImageFormat;
}

vk::Extent2D Swapchain::getExtent()
{
    return swapchainExtent;
}

void Swapchain::getFramebuffer(uint32_t index, vk::Framebuffer& result)
{
    result = swapchainFramebuffers[index];
}

SwapChainSupportDetails Swapchain::querySwapChainSupport(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    SwapChainSupportDetails details;
    details.capabilities = device->getSurfaceCapabilitiesKHR(surface);
    details.formats = device->getSurfaceFormatsKHR(surface);
    details.presentModes = device->getSurfacePresentModesKHR(surface);

    return details;
}

bool Swapchain::isSwapChainAdequate(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(surface, device);
    bool result = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    return result;
}

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if ((availableFormat.format == vk::Format::eB8G8R8A8Srgb) &&
            (availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            )
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width = std::clamp
    (
        actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
    );

    actualExtent.height = std::clamp
    (
        actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
    );

    return actualExtent;
}

void Swapchain::recreate(const vk::SurfaceKHR& surface, SDL_Window* window, Devices& devices, vk::RenderPass& renderPass)
{
    devices.getDevice()->waitIdle();

    this->cleanup(devices);

    this->init(surface, window, devices);
    this->createImageViews(devices);
    this->createFramebuffers(devices, renderPass);
}

void Swapchain::cleanup(Devices& devices)
{
    vk::Device* logicalDevice = devices.getDevice();
    for (vk::Framebuffer& framebuffer : swapchainFramebuffers)
    {
        logicalDevice->destroyFramebuffer(framebuffer);
    }

    for (const vk::ImageView& imageView : swapchainImageViews)
    {
        logicalDevice->destroyImageView(imageView);
    }

    logicalDevice->destroySwapchainKHR(*this->swapchainKHR.get());
}
