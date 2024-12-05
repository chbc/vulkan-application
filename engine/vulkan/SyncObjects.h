#pragma once

namespace vk
{
	class Device;
	class Semaphore;
	class Fence;
}

class SyncObjects
{
private:
	void init(vk::Device* logicalDevice, int maxFramesInFlight);
	const vk::Semaphore& getImageSemaphore(int currentFrame);
	const vk::Semaphore& getRenderSemaphore(int currentFrame);
	const vk::Fence& getInFlightFence(int currentFrame);
	void waitForFence(vk::Device* logicalDevice, int currentFrame);
	void resetFence(vk::Device* logicalDevice, int currentFrame);
	void release(vk::Device* logicalDevice, int maxFramesInFlight);

friend class VulkanAPI;
};
