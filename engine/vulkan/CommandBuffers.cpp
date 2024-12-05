#include "CommandBuffers.h"
#include "Devices.h"

#include <vulkan/vulkan.hpp>
#include <chrono>

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

vk::Buffer vertexBuffer;
vk::DeviceMemory vertexBufferMemory;
vk::Buffer indexBuffer;
vk::DeviceMemory indexBufferMemory;

std::vector<vk::Buffer> uniformBuffers;
std::vector<vk::DeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;

void CommandBuffers::init(const vk::SurfaceKHR& surface, Devices& devices, int maxFramesInFlight)
{
    QueueFamilyIndices queueFamilyIndices = devices.findQueueFamilies(surface);

    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());

    vk::CommandPool commandPoolValue = devices.getDevice()->createCommandPool(poolInfo);
    this->commandPool = std::make_shared<vk::CommandPool>(commandPoolValue);

    this->createVertexBuffer(devices);
    this->createIndexBuffer(devices);
    this->createUniformBuffers(devices, maxFramesInFlight);
}

void CommandBuffers::createUniformBuffers(Devices& devices, int maxFramesInFlight)
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(maxFramesInFlight);
    uniformBuffersMemory.resize(maxFramesInFlight);
    uniformBuffersMapped.resize(maxFramesInFlight);

    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
        this->createBuffer(devices, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);

        uniformBuffersMapped[i] = devices.getDevice()->mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

void CommandBuffers::createVertexBuffer(Devices& devices)
{
    vk::Device* logicalDevice = devices.getDevice();
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->createBuffer(devices, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = logicalDevice->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    logicalDevice->unmapMemory(stagingBufferMemory);

    this->createBuffer(devices, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

    this->copyBuffer(devices, stagingBuffer, vertexBuffer, bufferSize);

    logicalDevice->destroyBuffer(stagingBuffer);
    logicalDevice->freeMemory(stagingBufferMemory);
}

void CommandBuffers::createIndexBuffer(Devices& devices)
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->createBuffer(devices, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    vk::Device* logicalDevice = devices.getDevice();
    void* data = logicalDevice->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    logicalDevice->unmapMemory(stagingBufferMemory);

    this->createBuffer(devices, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    this->copyBuffer(devices, stagingBuffer, indexBuffer, bufferSize);

    logicalDevice->destroyBuffer(stagingBuffer);
    logicalDevice->freeMemory(stagingBufferMemory);
}

void CommandBuffers::createCommandBuffers(Devices& devices, int maxFramesInFlight)
{

    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(*commandPool.get())
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(static_cast<uint32_t>(maxFramesInFlight));

    commandBuffers.reserve(maxFramesInFlight);
    std::vector<vk::CommandBuffer> commandBufferValues = devices.getDevice()->allocateCommandBuffers(allocInfo);
    for (vk::CommandBuffer& item : commandBufferValues)
    {
        this->commandBuffers.push_back(std::make_shared<vk::CommandBuffer>(item));
    }

    if (commandBuffers.empty())
    {
        throw std::runtime_error("Couldn't allocate command buffer!");
    }
}

uint32_t CommandBuffers::getCurrentFrameIndex()
{
    return this->currentFrame;
}

const vk::CommandBuffer* CommandBuffers::getCurrentCommandBuffer()
{
    return this->commandBuffers[this->currentFrame].get();
}

void CommandBuffers::copyBuffer(Devices& devices, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size)
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(*this->commandPool.get())
        .setCommandBufferCount(1);

    vk::Device* logicalDevice = devices.getDevice();
    vk::CommandBuffer commandBuffer;
    if (logicalDevice->allocateCommandBuffers(&allocInfo, &commandBuffer) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate command buffer!");
    }

    vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    commandBuffer.begin(beginInfo);

    vk::BufferCopy copyRegion = vk::BufferCopy()
        .setSrcOffset(0).setDstOffset(0)
        .setSize(size);

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffer);

    const vk::Queue* graphicsQueue = devices.getGraphicsQueue();
    graphicsQueue->submit(submitInfo);
    graphicsQueue->waitIdle();

    logicalDevice->freeCommandBuffers(*this->commandPool.get(), commandBuffer);
}

void CommandBuffers::createBuffer(Devices& devices, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    vk::Device* logicalDevice = devices.getDevice();
    buffer = logicalDevice->createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = logicalDevice->getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(devices.findMemoryType(memRequirements.memoryTypeBits, properties));

    bufferMemory = logicalDevice->allocateMemory(allocInfo);
    logicalDevice->bindBufferMemory(buffer, bufferMemory, 0);
}

void CommandBuffers::recordCommandBuffer(const vk::Extent2D& swapchainExtent, const vk::RenderPassBeginInfo& renderPassInfo,
    const vk::Pipeline& graphicsPipeline, const vk::PipelineLayout& pipelineLayout, const vk::DescriptorSet* descriptorSets)
{
    vk::CommandBuffer* commandBuffer = this->commandBuffers[currentFrame].get();
    commandBuffer->reset();

    vk::CommandBufferBeginInfo beginInfo;

    commandBuffer->begin(beginInfo);

    commandBuffer->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f).setY(0.0f)
        .setWidth(static_cast<float>(swapchainExtent.width))
        .setHeight(static_cast<float>(swapchainExtent.height))
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    commandBuffer->setViewport(0, 1, &viewport);

    vk::Rect2D scissor{ {0, 0}, swapchainExtent };
    commandBuffer->setScissor(0, 1, &scissor);

    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer->bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer->bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, descriptorSets, 0, nullptr);
    commandBuffer->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    commandBuffer->endRenderPass();
    commandBuffer->end();
}

void CommandBuffers::updateUniformBuffer(const vk::Extent2D& swapchainExtent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), 0.25f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1.0f;

    memcpy(uniformBuffersMapped[this->currentFrame], &ubo, sizeof(ubo));
}

void CommandBuffers::increaseFrame(int maxFramesInFlight)
{
    this->currentFrame = (this->currentFrame + 1) % maxFramesInFlight;
}

vk::DescriptorBufferInfo CommandBuffers::createDescriptorBufferInfo(size_t index)
{
    vk::DescriptorBufferInfo result{ uniformBuffers[index], 0, sizeof(UniformBufferObject) };
    return result;
}

void CommandBuffers::releaseUniformBuffers(vk::Device* logicalDevice, size_t maxFramesInFlight)
{
    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
        logicalDevice->destroyBuffer(uniformBuffers[i]);
        logicalDevice->freeMemory(uniformBuffersMemory[i]);
    }
}

void CommandBuffers::release(vk::Device* logicalDevice)
{
    logicalDevice->destroyBuffer(indexBuffer);
    logicalDevice->freeMemory(indexBufferMemory);
    logicalDevice->destroyBuffer(vertexBuffer);
    logicalDevice->freeMemory(vertexBufferMemory);

    logicalDevice->destroyCommandPool(*this->commandPool.get());
}
