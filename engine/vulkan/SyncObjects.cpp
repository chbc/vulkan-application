#include "SyncObjects.h"

#include <vector>
#include <vulkan/vulkan.hpp>

std::vector<vk::Semaphore> imageAvailableSemaphores;
std::vector<vk::Semaphore> renderFinishedSemaphores;
std::vector<vk::Fence> inFlightFences;

void SyncObjects::init(vk::Device* logicalDevice, int maxFramesInFlight)
{
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled);

    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
        imageAvailableSemaphores[i] = logicalDevice->createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = logicalDevice->createSemaphore(semaphoreInfo);
        inFlightFences[i] = logicalDevice->createFence(fenceInfo);
    }
}

const vk::Semaphore& SyncObjects::getImageSemaphore(int currentFrame)
{
    return imageAvailableSemaphores[currentFrame];
}

const vk::Semaphore& SyncObjects::getRenderSemaphore(int currentFrame)
{
    return renderFinishedSemaphores[currentFrame];
}

const vk::Fence& SyncObjects::getInFlightFence(int currentFrame)
{
    return inFlightFences[currentFrame];
}

void SyncObjects::waitForFence(vk::Device* logicalDevice, int currentFrame)
{
    if (logicalDevice->waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't wait for fence!");
    }
}

void SyncObjects::resetFence(vk::Device* logicalDevice, int currentFrame)
{
    if (logicalDevice->resetFences(1, &inFlightFences[currentFrame]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't reset fence!");
    }
}

void SyncObjects::release(vk::Device* logicalDevice, int maxFramesInFlight)
{
    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
        logicalDevice->destroySemaphore(renderFinishedSemaphores[i]);
        logicalDevice->destroySemaphore(imageAvailableSemaphores[i]);
        logicalDevice->destroyFence(inFlightFences[i]);
    }
}
