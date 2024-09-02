#include "VulkanAPI.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>

vk::SurfaceKHR surface = nullptr;
vk::Instance instance = nullptr;

unsigned extension_count;
std::vector<const char*> extensions;

void VulkanAPI::init(const SDLAPI& sdlApi)
{
    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi.window, &extension_count, NULL))
    {
        throw std::exception("Could not get the number of required instance extensions from SDL.");
    }
    
    extensions.resize(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi.window, &extension_count, extensions.data()))
    {
        throw std::exception("Could not get the names of required instance extensions from SDL.");
    }

    createInstance();

    // Create a Vulkan surface for rendering
    VkSurfaceKHR c_surface;
    if (!SDL_Vulkan_CreateSurface(sdlApi.window, static_cast<VkInstance>(instance), &c_surface))
    {
        throw std::exception("Could not create a Vulkan surface.");
    }

    surface = c_surface;
}

void VulkanAPI::createInstance()
{
    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_KHRONOS_validation");
    if (!checkValidationLayerSupport(layers))
    {
        throw std::runtime_error("Validation layers requested, bot not available!");
    }
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
    catch (const std::exception& e)
    {
        char message[200];
        std::sprintf(message, "Could not create a Vulkan instance: %s", e.what());
        throw std::exception(message);
        
    }
}

bool VulkanAPI::checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool result = false;

    for (const char* layerName : validationLayers)
    {
        result = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

void VulkanAPI::preRelease()
{
    if (instance != nullptr)
    {
        if (surface != nullptr)
        {
            instance.destroySurfaceKHR(surface);
        }
    }
}

void VulkanAPI::release()
{
    if (instance != nullptr)
    {
        instance.destroy();
    }
}
