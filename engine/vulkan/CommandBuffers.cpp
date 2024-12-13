#include "CommandBuffers.h"
#include "Devices.h"
#include "Swapchain.h"

#include <vulkan/vulkan.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "dependencies/stb_image.h"

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

vk::Image textureImage;
vk::DeviceMemory textureImageMemory;
vk::ImageView textureImageView;
vk::Sampler textureSampler;

vk::Image depthImage;
vk::DeviceMemory depthImageMemory;
vk::ImageView depthImageView;

void CommandBuffers::init(const vk::SurfaceKHR& surface, Devices& devices, Swapchain& swapchain, int maxFramesInFlight)
{
    QueueFamilyIndices queueFamilyIndices = devices.findQueueFamilies(surface);

    vk::Device* logicalDevice = devices.getDevice();
    this->createCommandPool(logicalDevice, queueFamilyIndices.graphicsFamily.value());
    this->createDepthResources(devices, swapchain);
    this->createTextureImage(devices);
    this->createTextureImageView(devices);
    this->createTextureSampler(devices);
    this->createVertexBuffer(devices);
    this->createIndexBuffer(devices);
    this->createUniformBuffers(devices, maxFramesInFlight);
}

void CommandBuffers::createCommandPool(vk::Device* logicalDevice, uint32_t queueFamilyIndex)
{
    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(queueFamilyIndex);
    vk::CommandPool commandPoolValue = logicalDevice->createCommandPool(poolInfo);
    this->commandPool = std::make_shared<vk::CommandPool>(commandPoolValue);
}

void CommandBuffers::createDepthResources(Devices& devices, Swapchain& swapchain)
{
    const vk::Extent2D& swapchainExtent = swapchain.getExtent();
    
    vk::Format depthFormat = devices.findDepthFormat();
    this->createImage(devices, swapchainExtent.width, swapchainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);

    depthImageView = devices.createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void CommandBuffers::recreateDepthResources(Devices& devices, Swapchain& swapchain)
{
    this->releaseDepthImages(devices.getDevice());
    this->createDepthResources(devices, swapchain);
}

void CommandBuffers::createTextureImage(Devices& devices)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../../textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->createBuffer(devices, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    vk::Device* logicalDevice = devices.getDevice();
    void* data = logicalDevice->mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    logicalDevice->unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(devices, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal,
        textureImage, textureImageMemory);

    this->transitionImageLayout(devices, textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    this->copyBufferToImage(devices, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    this->transitionImageLayout(devices, textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    logicalDevice->destroyBuffer(stagingBuffer);
    logicalDevice->freeMemory(stagingBufferMemory);
}

void CommandBuffers::createImage(Devices& devices, uint32_t widith, uint32_t height, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo = vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setExtent(vk::Extent3D{ static_cast<uint32_t>(widith), static_cast<uint32_t>(height), 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setFormat(format)
        .setTiling(tiling)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setUsage(usage)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive);

    vk::Device* logicalDevice = devices.getDevice();

    if (logicalDevice->createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create image!");
    }

    vk::MemoryRequirements memRequirements = logicalDevice->getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(devices.findMemoryType(memRequirements.memoryTypeBits, properties));

    imageMemory = logicalDevice->allocateMemory(allocInfo, nullptr);
    logicalDevice->bindImageMemory(image, imageMemory, 0);
}

void CommandBuffers::transitionImageLayout(Devices& devices, vk::Image& image, vk::Format format,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = this->beginSingleTimeCommands(devices.getDevice());

    vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setImage(image)
        .setSubresourceRange(vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if ((oldLayout == vk::ImageLayout::eUndefined) && (newLayout == vk::ImageLayout::eTransferDstOptimal))
    {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if ((oldLayout == vk::ImageLayout::eTransferDstOptimal) && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal))
    {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier
    (
        sourceStage, destinationStage,
        vk::DependencyFlagBits::eByRegion, 0, nullptr,
        0, nullptr, 1, &barrier
    );

    this->endSingleTimeCommands(devices, commandBuffer);
}

void CommandBuffers::copyBufferToImage(Devices& devices, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer commandBuffer = this->beginSingleTimeCommands(devices.getDevice());

    vk::BufferImageCopy region = vk::BufferImageCopy()
        .setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 })
        .setImageOffset({ 0, 0, 0 })
        .setImageExtent({ width, height, 1 });

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    this->endSingleTimeCommands(devices, commandBuffer);
}

void CommandBuffers::createTextureImageView(Devices& devices)
{
    textureImageView = devices.createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void CommandBuffers::createTextureSampler(Devices& devices)
{
    vk::PhysicalDeviceProperties properties = devices.getPhysicalDevice()->getProperties();

    vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eMirroredRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eMirroredRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eMirroredRepeat)
        .setAnisotropyEnable(vk::True)
        .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(vk::False)
        .setCompareEnable(vk::False)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0f)
        .setMinLod(0.0f)
        .setMaxLod(0.0f);

    textureSampler = devices.getDevice()->createSampler(samplerInfo);
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

void CommandBuffers::createCommandBuffers(vk::Device* logicalDevice, int maxFramesInFlight)
{

    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(*commandPool.get())
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(static_cast<uint32_t>(maxFramesInFlight));

    commandBuffers.reserve(maxFramesInFlight);
    std::vector<vk::CommandBuffer> commandBufferValues = logicalDevice->allocateCommandBuffers(allocInfo);
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

vk::ImageView& CommandBuffers::getDepthImageView()
{
    return depthImageView;
}

const vk::CommandBuffer* CommandBuffers::getCurrentCommandBuffer()
{
    return this->commandBuffers[this->currentFrame].get();
}

void CommandBuffers::copyBuffer(Devices& devices, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size)
{
    vk::CommandBuffer commandBuffer = this->beginSingleTimeCommands(devices.getDevice());

    vk::BufferCopy copyRegion = vk::BufferCopy()
        .setSrcOffset(0).setDstOffset(0)
        .setSize(size);

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    this->endSingleTimeCommands(devices, commandBuffer);
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

void CommandBuffers::recordCommandBuffer(const vk::Extent2D& swapchainExtent, vk::RenderPassBeginInfo& renderPassInfo,
    const vk::Pipeline& graphicsPipeline, const vk::PipelineLayout& pipelineLayout, const vk::DescriptorSet* descriptorSets)
{
    vk::CommandBuffer* commandBuffer = this->commandBuffers[currentFrame].get();
    commandBuffer->reset();

    vk::CommandBufferBeginInfo beginInfo;

    commandBuffer->begin(beginInfo);

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

    renderPassInfo.setClearValues(clearValues);

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

vk::CommandBuffer CommandBuffers::beginSingleTimeCommands(vk::Device* logicalDevice)
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(*this->commandPool)
        .setCommandBufferCount(1);

    std::vector<vk::CommandBuffer> commandBufferValues = logicalDevice->allocateCommandBuffers(allocInfo);
    vk::CommandBuffer result = commandBufferValues.front();

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    result.begin(beginInfo);

    return result;
}

void CommandBuffers::endSingleTimeCommands(Devices& devices, vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setCommandBuffers(commandBuffer);
    std::vector<vk::SubmitInfo> submitInfos = { submitInfo };

    const vk::Queue* graphicsQueue = devices.getGraphicsQueue();
    graphicsQueue->submit(submitInfos);
    graphicsQueue->waitIdle();

    devices.getDevice()->freeCommandBuffers(*this->commandPool, 1, &commandBuffer);
}

void CommandBuffers::increaseFrame(int maxFramesInFlight)
{
    this->currentFrame = (this->currentFrame + 1) % maxFramesInFlight;
}

void CommandBuffers::createDescriptorsBufferInfo(size_t index, vk::DescriptorBufferInfo& bufferInfo, vk::DescriptorImageInfo& imageInfo)
{
    bufferInfo = vk::DescriptorBufferInfo(uniformBuffers[index], 0, sizeof(UniformBufferObject));
    imageInfo = vk::DescriptorImageInfo(textureSampler, textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void CommandBuffers::releaseUniformBuffers(vk::Device* logicalDevice, size_t maxFramesInFlight)
{
    for (size_t i = 0; i < maxFramesInFlight; i++)
    {
        if (i < uniformBuffers.size())
        {
            logicalDevice->destroyBuffer(uniformBuffers[i]);
        }

        if (i < uniformBuffersMemory.size())
        {
            logicalDevice->freeMemory(uniformBuffersMemory[i]);
        }
    }
}

void CommandBuffers::release(vk::Device* logicalDevice)
{
    logicalDevice->destroySampler(textureSampler);
    logicalDevice->destroyImageView(textureImageView);
    logicalDevice->destroyImage(textureImage);
    logicalDevice->freeMemory(textureImageMemory);

    logicalDevice->destroyBuffer(indexBuffer);
    logicalDevice->freeMemory(indexBufferMemory);
    logicalDevice->destroyBuffer(vertexBuffer);
    logicalDevice->freeMemory(vertexBufferMemory);

    this->releaseDepthImages(logicalDevice);

    logicalDevice->destroyCommandPool(*this->commandPool.get());

}

void CommandBuffers::releaseDepthImages(vk::Device* logicalDevice)
{
    logicalDevice->destroyImageView(depthImageView);
    logicalDevice->destroyImage(depthImage);
    logicalDevice->freeMemory(depthImageMemory);
}
