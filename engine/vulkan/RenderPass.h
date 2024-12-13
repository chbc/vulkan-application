#pragma once

#include "vk_forward_declarations.h"
#include <memory>

class Swapchain;
class Devices;

namespace vk
{
	struct RenderPassBeginInfo;
}

class RenderPass
{
private:
	void init(Devices& devices, Swapchain& swapchain);
	vk::RenderPassBeginInfo createInfo(Swapchain& swapchain, const vk::Extent2D& extent, uint32_t imageIndex);
	vk::RenderPass& getRenderPassRef();
	void release(vk::Device* logicalDevice);

friend class VulkanAPI;
};
