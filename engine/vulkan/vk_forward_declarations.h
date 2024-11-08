#pragma once
#include <stdint.h>

namespace vk
{
	class Instance;
	class Device;
	class PhysicalDevice;
	class ShaderModule;
	class CommandBuffer;
	class SurfaceKHR;
	class Buffer;
	class DeviceMemory;
	class Queue;

	struct SurfaceFormatKHR;
	struct Extent2D;
	struct SurfaceCapabilitiesKHR;

	enum class PresentModeKHR;

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
