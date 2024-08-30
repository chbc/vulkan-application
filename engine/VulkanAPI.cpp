#include "VulkanAPI.h"

#include "SDLAPI.h"
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>

vk::SurfaceKHR surface;
vk::Instance instance;

unsigned extension_count;
std::vector<const char*> extensions;

bool VulkanAPI::init()
{
    if (!SDLAPI::init(SDL_WINDOW_VULKAN))
    {
        return false;
    }

    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    if (!SDL_Vulkan_GetInstanceExtensions(SDLAPI::window, &extension_count, NULL))
    {
        std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
        return false;
    }
    
    extensions.resize(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(SDLAPI::window, &extension_count, extensions.data()))
    {
        std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
        return false;
    }

    if (!createInstance())
    {
        return false;
    }

    // Create a Vulkan surface for rendering
    VkSurfaceKHR c_surface;
    if (!SDL_Vulkan_CreateSurface(SDLAPI::window, static_cast<VkInstance>(instance), &c_surface))
    {
        std::cout << "Could not create a Vulkan surface." << std::endl;
        return false;
    }

    surface = c_surface;

    return true;
}

bool VulkanAPI::createInstance()
{
    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
        .setPApplicationName("Vulkan C++ Windowed Program Template")
        .setApplicationVersion(1)
        .setPEngineName("LunarG SDK")
        .setEngineVersion(1)
        .setApiVersion(VK_API_VERSION_1_0);

    // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
        .setFlags(vk::InstanceCreateFlags())
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data())
        .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
        .setPpEnabledLayerNames(layers.data());

    // Create the Vulkan instance.
    try
    {
        instance = vk::createInstance(createInfo);
    }
    catch (const std::exception& e) {
        std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void VulkanAPI::release()
{
    instance.destroySurfaceKHR(surface);
    SDLAPI::release();
    instance.destroy();
}
