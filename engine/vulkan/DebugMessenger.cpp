#include "DebugMessenger.h"

#include <vulkan/vulkan_core.h>
#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult DebugMessenger::init(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
#if defined(_DEBUG)
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo;
    this->populateDebugMessengerCreateInfo(messengerCreateInfo);

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, &messengerCreateInfo, pAllocator, &this->debugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif

    return VK_SUCCESS;
}

void DebugMessenger::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo)
{
#if defined(_DEBUG)
    messengerCreateInfo = {};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = debugCallback;
#endif
}

void DebugMessenger::release(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
#if defined(_DEBUG)
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, this->debugMessenger, pAllocator);
    }
#endif
}
