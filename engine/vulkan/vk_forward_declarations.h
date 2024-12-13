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
	class CommandPool;
	class Pipeline;
	class PipelineLayout;
	class DescriptorSetLayout;
	class DescriptorPool;
	class DescriptorSet;
	class Image;
	class ImageView;
	
	struct PipelineLayoutCreateInfo;
	struct SurfaceFormatKHR;
	struct Extent2D;
	struct SurfaceCapabilitiesKHR;
	struct RenderPassBeginInfo;
	struct DescriptorBufferInfo;
	struct DescriptorImageInfo;

	enum class PresentModeKHR;
	enum class Format;
	enum class ImageLayout;
	enum class ImageTiling;

	template <typename BitType>	class Flags;
	typedef uint32_t VkFlags;
	typedef VkFlags VkBufferUsageFlags;
	typedef VkFlags VkMemoryPropertyFlags;
	typedef VkFlags VkImageUsageFlags;
	typedef VkFlags VkFormatFeatureFlags;
	typedef VkFlags VkImageAspectFlags;

	enum class MemoryPropertyFlagBits : VkMemoryPropertyFlags;
	enum class BufferUsageFlagBits : VkBufferUsageFlags;
	enum class ImageUsageFlagBits : VkImageUsageFlags;
	enum class FormatFeatureFlagBits : VkFormatFeatureFlags;
	enum class ImageAspectFlagBits : VkImageAspectFlags;

	using MemoryPropertyFlags = Flags<MemoryPropertyFlagBits>;
	using BufferUsageFlags = Flags<BufferUsageFlagBits>;
	using ImageUsageFlags = Flags<ImageUsageFlagBits>;
	using FormatFeatureFlags = Flags<FormatFeatureFlagBits>;
	using ImageAspectFlags = Flags<ImageAspectFlagBits>;

	using DeviceSize = uint64_t;
}
