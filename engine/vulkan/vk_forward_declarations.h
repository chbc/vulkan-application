#pragma once
#include <stdint.h>

namespace vk
{
	class Instance;
	class Device;
	class PhysicalDevice;
	class ShaderModule;
	class CommandBuffer;
	class SwapchainKHR;
	class SurfaceKHR;
	class Buffer;
	class DeviceMemory;
	class Queue;
	class RenderPass;
	class Framebuffer;

	struct SurfaceFormatKHR;
	struct Extent2D;
	struct SurfaceCapabilitiesKHR;

	enum class PresentModeKHR;
	enum class Format;

	template <typename BitType>	class Flags;
	typedef uint32_t VkFlags;
	typedef VkFlags VkBufferUsageFlags;
	typedef VkFlags VkMemoryPropertyFlags;

	enum class MemoryPropertyFlagBits : VkMemoryPropertyFlags;
	enum class BufferUsageFlagBits : VkBufferUsageFlags;

	using MemoryPropertyFlags = Flags<MemoryPropertyFlagBits>;
	using BufferUsageFlags = Flags<BufferUsageFlagBits>;

	using DeviceSize = uint64_t;
}
