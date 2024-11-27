#pragma once

#include "vk_forward_declarations.h"
#include <vector>
#include <memory>

class Devices;
struct SDL_Window;
struct SwapChainSupportDetails;

class Swapchain
{
private:
    std::shared_ptr<vk::SwapchainKHR> swapchainKHR;

private:
	void init(const vk::SurfaceKHR& surface, SDL_Window* window, Devices& devices);
    void createImageViews(Devices& devices);
    void createFramebuffers(Devices& devices, vk::RenderPass& renderPass);
    vk::SwapchainKHR* getSwapchainKHR();
    vk::Format getImageFormat();
    vk::Extent2D getExtent();
    void getFramebuffer(uint32_t index, vk::Framebuffer& result);
    static SwapChainSupportDetails querySwapChainSupport(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
    static bool isSwapChainAdequate(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
    void cleanup(Devices& devices);

friend class VulkanAPI;
friend class Devices;
};
