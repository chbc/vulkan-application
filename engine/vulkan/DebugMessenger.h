#pragma once

enum VkResult;
typedef struct VkInstance_T* VkInstance;
typedef struct VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;
struct VkDebugUtilsMessengerCreateInfoEXT;
struct VkAllocationCallbacks;

class DebugMessenger
{
private:
	VkDebugUtilsMessengerEXT debugUtilsMessenger;

private:
	VkResult init(VkInstance instance, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerCreateInfo);
	void release(VkInstance instance, const VkAllocationCallbacks* pAllocator);

friend class VulkanAPI;
};
