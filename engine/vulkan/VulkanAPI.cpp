#include "VulkanAPI.h"

#include "engine/Utils.h"
#include "engine/mediaLoader/MediaLoader.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <array>
#include <chrono>
#include <optional>
#include <set>

vk::SurfaceKHR surface = nullptr;
vk::Instance instance = nullptr;

const int MAX_FRAMES_IN_FLIGHT = 2;

vk::PipelineLayout pipelineLayout;
vk::Pipeline graphicsPipeline;

// Devices
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

vk::PhysicalDevice physicalDevice;
vk::Device logicalDevice;
QueueFamilyIndices familyIndices;
vk::Queue graphicsQueue;
vk::Queue presentQueue;

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
//

// Swapchain
vk::SwapchainKHR swapchainKHR;

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

std::vector<vk::Image> swapchainImages;
vk::Format swapchainImageFormat;
vk::Extent2D swapchainExtent;
std::vector<vk::ImageView> swapchainImageViews;
std::vector<vk::Framebuffer> swapchainFramebuffers;
vk::ResultValue<uint32_t> currentImageIndex{ vk::Result::eSuccess, 0 };
//

// RenderPass
vk::RenderPass renderPassRef;
//

// DescriptorSets
std::vector<vk::DescriptorSet> vkDescriptorSets;
vk::DescriptorSetLayout descriptorSetLayout;
vk::DescriptorPool descriptorPool;
//

// CommandBuffers
struct ModelBuffers
{
    size_t indicesSize{ 0 };
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
};

vk::CommandPool commandPool;
std::vector<vk::CommandBuffer> commandBuffers;
uint32_t currentFrame = 0;

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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
//

// SyncObjects
std::vector<vk::Semaphore> imageAvailableSemaphores;
std::vector<vk::Semaphore> renderFinishedSemaphores;
std::vector<vk::Fence> inFlightFences;
//

void VulkanAPI::init(SDLAPI& sdlApi)
{
    this->sdlApi = &sdlApi;

    this->createInstance();
    this->debugMessenger.init(instance, nullptr);
    this->createSurface();
    this->Devices_init(instance, surface, this->validationLayers);
    this->Swapchain_init(surface, this->sdlApi->window);
    this->Swapchain_createImageViews();
    this->RenderPass_init();

    this->DescriptorSets_initLayout();
    this->createGraphicsPipeline();

    this->CommandBuffers_init(surface);
    this->Swapchain_createFramebuffers();

    this->DescriptorSets_initPool();
    this->DescriptorSets_initDescriptorSet();
    this->CommandBuffers_createCommandBuffers();
    this->SyncObjects_init();
}

void VulkanAPI::beginDraw()
{
    uint32_t currentFrame = this->CommandBuffers_getCurrentFrameIndex();
    this->SyncObjects_waitForFence(currentFrame);

    const vk::Semaphore& currentImageSemaphore = imageAvailableSemaphores[currentFrame];
    currentImageIndex = logicalDevice.acquireNextImageKHR(swapchainKHR, UINT64_MAX, currentImageSemaphore);

    if (currentImageIndex.result == vk::Result::eErrorOutOfDateKHR)
    {
        this->Swapchain_recreate(surface, this->sdlApi->window);
        this->CommandBuffers_recreateDepthResources();
        this->Swapchain_createFramebuffers();
        return;
    }
    else if ((currentImageIndex.result != vk::Result::eSuccess) && (currentImageIndex.result != vk::Result::eSuboptimalKHR))
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    this->SyncObjects_resetFence(currentFrame);



	// Record command buffer
    const vk::CommandBuffer& commandBuffer = commandBuffers[currentFrame];
    commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo;

    commandBuffer.begin(beginInfo);

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

    vk::RenderPassBeginInfo& renderPassInfo = this->RenderPass_createInfo(swapchainExtent, currentImageIndex.value);
    renderPassInfo.setClearValues(clearValues);

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f).setY(0.0f)
        .setWidth(static_cast<float>(swapchainExtent.width))
        .setHeight(static_cast<float>(swapchainExtent.height))
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{ {0, 0}, swapchainExtent };
    commandBuffer.setScissor(0, 1, &scissor);
}

void VulkanAPI::drawItem(size_t itemId)
{
    this->CommandBuffers_updateUniformBuffer(swapchainExtent);

    std::shared_ptr<ModelBuffers>& item = this->modelBuffersMap[itemId];
    vk::DeviceSize offsets[] = { 0 };
    vk::Buffer vertexBuffers[] = { item->vertexBuffer };

    const vk::CommandBuffer& commandBuffer = commandBuffers[currentFrame];
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(item->indexBuffer, 0, vk::IndexType::eUint32);

    const vk::DescriptorSet* descriptorSets = &vkDescriptorSets[currentFrame];
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, descriptorSets, 0, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(item->indicesSize), 1, 0, 0, 0);
}

void VulkanAPI::endDraw()
{
    const vk::CommandBuffer& commandBuffer = commandBuffers[currentFrame];
    commandBuffer.endRenderPass();
    commandBuffer.end();

    const vk::Semaphore& currentImageSemaphore = imageAvailableSemaphores[currentFrame];
    const vk::Semaphore& currentRenderSemaphore = renderFinishedSemaphores[currentFrame];
    vk::Semaphore waitSemaphores[] = { currentImageSemaphore };
    vk::Semaphore signalSemaphores[] = { currentRenderSemaphore };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(waitSemaphores)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(signalSemaphores);

    const vk::Fence& inFlightFence = inFlightFences[currentFrame];
    if (graphicsQueue.submit(1, &submitInfo, inFlightFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(signalSemaphores)
        .setSwapchainCount(1)
        .setPSwapchains(&swapchainKHR)
        .setPImageIndices(&currentImageIndex.value)
        .setPResults(nullptr);

    vk::Result result = presentQueue.presentKHR(&presentInfo);

    if ((result == vk::Result::eErrorOutOfDateKHR) || (result != vk::Result::eSuboptimalKHR) || framebufferResized)
    {
        framebufferResized = false;
        this->Swapchain_recreate(surface, this->sdlApi->window);
        this->CommandBuffers_recreateDepthResources();
        this->Swapchain_createFramebuffers();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    this->CommandBuffers_increaseFrame(MAX_FRAMES_IN_FLIGHT);
}

std::vector<const char*> VulkanAPI::getRequiredExtensions()
{
    std::vector<const char*> result;

    unsigned extension_count;
    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi->window, &extension_count, NULL))
    {
        throw std::exception("Could not get the number of required instance extensions from SDL.");
    }

    result.resize(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi->window, &extension_count, result.data()))
    {
        throw std::exception("Could not get the names of required instance extensions from SDL.");
    }

#if defined(_DEBUG)
    result.push_back("VK_EXT_debug_utils");
#endif

    return result;
}

void VulkanAPI::createInstance()
{
    if (!validationLayers.checkSupport())
    {
        throw std::runtime_error("Validation layers requested, bot not available!");
    }

    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
        .setPApplicationName("Vulkan C++ Windowed Program Template")
        .setApplicationVersion(1)
        .setPEngineName("LunarG SDK")
        .setEngineVersion(1)
        .setApiVersion(VK_API_VERSION_1_0);

    // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    std::vector<const char*> extensions = getRequiredExtensions();
    vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
        .setFlags(vk::InstanceCreateFlags())
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(extensions)
        .setPEnabledLayerNames(validationLayers.getData());

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    debugMessenger.populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.setPNext((VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo);

    // Create the Vulkan instance.
    try
    {
        instance = vk::createInstance(createInfo);
    }
    catch (const std::exception& e)
    {
        char message[200];
        sprintf_s(message, "Could not create a Vulkan instance: %s", e.what());
        throw std::exception(message);
        
    }
}

void VulkanAPI::createSurface()
{
    // Create a Vulkan surface for rendering
    VkSurfaceKHR c_surface;
    if (!SDL_Vulkan_CreateSurface(sdlApi->window, static_cast<VkInstance>(instance), &c_surface))
    {
        throw std::exception("Could not create a Vulkan surface.");
    }

    surface = c_surface;
}

void VulkanAPI::createGraphicsPipeline()
{
    std::vector<char> vertShaderCode = Utils::readFile("../../shaders/vert.spv");
    std::vector<char> fragShaderCode = Utils::readFile("../../shaders/frag.spv");

    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()))
        .setPVertexBindingDescriptions(&bindingDescription)
        .setPVertexAttributeDescriptions(attributeDescriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(vk::False);

    vk::PipelineViewportStateCreateInfo viewportState = vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1)
        .setScissorCount(1);

    vk::PipelineRasterizationStateCreateInfo rasterizer = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(vk::False)
        .setRasterizerDiscardEnable(vk::False)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setDepthBiasEnable(vk::False)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(0.0f)
        .setDepthBiasSlopeFactor(0.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling = vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(vk::False)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(vk::False)
        .setAlphaToOneEnable(vk::False);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(vk::False)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo colorBlending = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(vk::False)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachmentCount(1)
        .setPAttachments(&colorBlendAttachment)
        .setBlendConstants(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f });

    std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicState = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
        .setPDynamicStates(dynamicStates.data());

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = this->DescriptorSets_createPipelineLayoutInfo();

    if (logicalDevice.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil = vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(vk::True)
        .setDepthWriteEnable(vk::True)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(vk::False)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f)
        .setStencilTestEnable(vk::False);

    vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shaderStages)
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(pipelineLayout)
        .setRenderPass(renderPassRef)
        .setSubpass(0)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);

    vk::Result result;
    std::tie(result, graphicsPipeline) = logicalDevice.createGraphicsPipeline(nullptr, pipelineInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    logicalDevice.destroyShaderModule(vertShaderModule);
    logicalDevice.destroyShaderModule(fragShaderModule);
}

vk::ShaderModule VulkanAPI::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    vk::ShaderModule shaderModule = logicalDevice.createShaderModule(createInfo);
    return shaderModule;
}

// Devices
void VulkanAPI::Devices_init(const vk::Instance& instance, const vk::SurfaceKHR& surface, const ValidationLayers& validationLayers)
{
    this->Devices_pickPhysicalDevice(instance, surface);
    this->Devices_createLogicalDevice(validationLayers);
}

void VulkanAPI::Devices_pickPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface)
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const vk::PhysicalDevice& device : devices)
    {
        if (Devices_isDeviceSuitable(surface, device))
        {
            physicalDevice = device;
            break;
        }
    }

    if (!physicalDevice)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanAPI::Devices_createLogicalDevice(const ValidationLayers& validationLayers)
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queueFamily)
            .setQueueCount(1)
            .setQueuePriorities(queuePriority);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures()
        .setSamplerAnisotropy(vk::True);

    vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
        .setPQueueCreateInfos(queueCreateInfos.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledLayerNames(validationLayers.getData());

    logicalDevice = physicalDevice.createDevice(createInfo);

    if (!logicalDevice)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    graphicsQueue = logicalDevice.getQueue(familyIndices.graphicsFamily.value(), 0);
    presentQueue = logicalDevice.getQueue(familyIndices.presentFamily.value(), 0);
}

uint32_t VulkanAPI::Devices_findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

vk::Format VulkanAPI::Devices_findDepthFormat()
{
    return this->Devices_findSupportedFormat
    (
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::Format VulkanAPI::Devices_findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    vk::Format result = vk::Format::eUndefined;

    for (vk::Format format : candidates)
    {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if
        (
            ((tiling == vk::ImageTiling::eLinear) && (props.linearTilingFeatures & features) == features) ||
            ((tiling == vk::ImageTiling::eOptimal) && (props.optimalTilingFeatures & features) == features)
        )
        {
            result = format;
            break;
        }
    }

    if (result == vk::Format::eUndefined)
    {
        throw std::runtime_error("Failed to find supported format!");
    }

    return result;
}

bool VulkanAPI::Devices_hasStencilComponent(vk::Format format)
{
    return ((format == vk::Format::eD32SfloatS8Uint) || (format == vk::Format::eD24UnormS8Uint));
}

bool VulkanAPI::Devices_isDeviceSuitable(const vk::SurfaceKHR& surface, const vk::PhysicalDevice& device)
{
    QueueFamilyIndices indices = Devices_findQueueFamilies(surface, &device);
    bool extensionsSupported = Devices_checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    if (extensionsSupported)
    {
        swapChainAdequate = Swapchain_isSwapChainAdequate(surface, &device);
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    bool result = indices.isComplete() && extensionsSupported &&
        swapChainAdequate && supportedFeatures.samplerAnisotropy;
    if (result)
    {
        familyIndices = indices;
    }

    return result;
}

QueueFamilyIndices VulkanAPI::Devices_findQueueFamilies(const vk::SurfaceKHR& surface)
{
    return Devices_findQueueFamilies(surface, &physicalDevice);
}

QueueFamilyIndices VulkanAPI::Devices_findQueueFamilies(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device->getQueueFamilyProperties();

    for (int i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        if (device->getSurfaceSupportKHR(i, surface) > 0)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
    }

    return indices;
}

bool VulkanAPI::Devices_checkDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const vk::ExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

vk::ImageView VulkanAPI::Devices_createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
        .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(vk::ImageSubresourceRange{ aspectFlags, 0, 1, 0, 1 });

    vk::ImageView result = logicalDevice.createImageView(viewInfo);

    if (result == nullptr)
    {
        throw std::runtime_error("Failed to create image views!");
    }

    return result;
}
//

// Swapchain
void VulkanAPI::Swapchain_init(const vk::SurfaceKHR& surface, SDL_Window* window)
{
    SwapChainSupportDetails swapchainSupport = this->Swapchain_querySwapChainSupport(surface, &physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = Swapchain_chooseSwapSurfaceFormat(swapchainSupport.formats);
    vk::PresentModeKHR presentMode = Swapchain_chooseSwapPresentMode(swapchainSupport.presentModes);
    vk::Extent2D extent = Swapchain_chooseSwapExtent(swapchainSupport.capabilities, window);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if ((swapchainSupport.capabilities.maxImageCount > 0) && (imageCount > swapchainSupport.capabilities.maxImageCount))
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(imageCount)
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(swapchainSupport.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setClipped(vk::True)
        .setOldSwapchain(nullptr);

    QueueFamilyIndices indices = Devices_findQueueFamilies(surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        createInfo.setQueueFamilyIndexCount(2);
        createInfo.setPQueueFamilyIndices(queueFamilyIndices);
    }
    else
    {
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        createInfo.setQueueFamilyIndexCount(0);
        createInfo.setPQueueFamilyIndices(nullptr);
    }

    swapchainKHR = logicalDevice.createSwapchainKHR(createInfo);
    swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchainKHR);
    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}

void VulkanAPI::Swapchain_createImageViews()
{
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        swapchainImageViews[i] = Devices_createImageView(swapchainImages[i], swapchainImageFormat, vk::ImageAspectFlagBits::eColor);
        if (swapchainImageViews[i] == nullptr)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void VulkanAPI::Swapchain_createFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImageViews.size());
    for (size_t i = 0; i < swapchainImageViews.size(); i++)
    {
        std::array<vk::ImageView, 2> attachments = { swapchainImageViews[i], depthImageView };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
            .setRenderPass(renderPassRef)
            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
            .setAttachments(attachments)
            .setWidth(swapchainExtent.width)
            .setHeight(swapchainExtent.height)
            .setLayers(1);

        swapchainFramebuffers[i] = logicalDevice.createFramebuffer(framebufferInfo);
    }
}

void VulkanAPI::Swapchain_getFramebuffer(uint32_t index, vk::Framebuffer& result)
{
    result = swapchainFramebuffers[index];
}

SwapChainSupportDetails VulkanAPI::Swapchain_querySwapChainSupport(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    SwapChainSupportDetails details;
    details.capabilities = device->getSurfaceCapabilitiesKHR(surface);
    details.formats = device->getSurfaceFormatsKHR(surface);
    details.presentModes = device->getSurfacePresentModesKHR(surface);

    return details;
}

bool VulkanAPI::Swapchain_isSwapChainAdequate(const vk::SurfaceKHR& surface, const vk::PhysicalDevice* device)
{
    SwapChainSupportDetails swapChainSupport = Swapchain_querySwapChainSupport(surface, device);
    bool result = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    return result;
}

vk::SurfaceFormatKHR VulkanAPI::Swapchain_chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if ((availableFormat.format == vk::Format::eB8G8R8A8Srgb) &&
            (availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            )
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR VulkanAPI::Swapchain_chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanAPI::Swapchain_chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width = std::clamp
    (
        actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
    );

    actualExtent.height = std::clamp
    (
        actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
    );

    return actualExtent;
}

void VulkanAPI::Swapchain_recreate(const vk::SurfaceKHR& surface, SDL_Window* window)
{
    logicalDevice.waitIdle();

    this->Swapchain_release();

    this->Swapchain_init(surface, window);
    this->Swapchain_createImageViews();
}

void VulkanAPI::Swapchain_release()
{
    for (vk::Framebuffer& framebuffer : swapchainFramebuffers)
    {
        logicalDevice.destroyFramebuffer(framebuffer);
    }

    for (const vk::ImageView& imageView : swapchainImageViews)
    {
        logicalDevice.destroyImageView(imageView);
    }

    logicalDevice.destroySwapchainKHR(swapchainKHR);
}
//

// RenderPass
void VulkanAPI::RenderPass_init()
{
    vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
        .setFormat(swapchainImageFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
        .setFormat(Devices_findDepthFormat())
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference()
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef);

    vk::SubpassDependency dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
        .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
        .setAttachments(attachments)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency);

    if (logicalDevice.createRenderPass(&renderPassInfo, nullptr, &renderPassRef) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

vk::RenderPassBeginInfo VulkanAPI::RenderPass_createInfo(const vk::Extent2D& extent, uint32_t imageIndex)
{
    vk::Framebuffer framebuffer;
    Swapchain_getFramebuffer(imageIndex, framebuffer);

    vk::ClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };
    vk::RenderPassBeginInfo result = vk::RenderPassBeginInfo()
        .setRenderPass(renderPassRef)
        .setFramebuffer(framebuffer)
        .setRenderArea(vk::Rect2D{ {0, 0}, extent })
        .setClearValueCount(1)
        .setPClearValues(&clearColor);

    return result;
}
//

// DescriptorSets
void VulkanAPI::DescriptorSets_initLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
    vk::DescriptorSetLayoutBinding samplerLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo LayoutInfo{ {}, bindings };

    descriptorSetLayout = logicalDevice.createDescriptorSetLayout(LayoutInfo);
}

void VulkanAPI::DescriptorSets_initPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes =
    {
        vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT },
        vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT}
    };

    vk::DescriptorPoolCreateInfo poolInfo{ {} , MAX_FRAMES_IN_FLIGHT, poolSizes };
    descriptorPool = logicalDevice.createDescriptorPool(poolInfo);
}

void VulkanAPI::DescriptorSets_initDescriptorSet()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(descriptorPool)
        .setDescriptorSetCount(MAX_FRAMES_IN_FLIGHT)
        .setSetLayouts(layouts);

    vkDescriptorSets = logicalDevice.allocateDescriptorSets(allocInfo);
}

vk::PipelineLayoutCreateInfo VulkanAPI::DescriptorSets_createPipelineLayoutInfo()
{
    vk::PipelineLayoutCreateInfo result = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptorSetLayout)
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges(nullptr);

    return result;
}

void VulkanAPI::DescriptorSets_release()
{
    logicalDevice.destroyDescriptorPool(descriptorPool);
    logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);
}
//

// CommandBuffers
size_t VulkanAPI::loadModel(const char* filePath)
{
    Model model;
    MediaLoader::loadModel(filePath, model);
    std::shared_ptr<ModelBuffers> modelBuffers{ new ModelBuffers };
    modelBuffers->indicesSize = model.indices.size();

    this->CommandBuffers_createVertexBuffer(model, modelBuffers.get());
    this->CommandBuffers_createIndexBuffer(model, modelBuffers.get());
    this->CommandBuffers_createUniformBuffers();

    size_t result = this->modelBuffersMap.size();
    this->modelBuffersMap.emplace_back(modelBuffers);

    return result;
}

void VulkanAPI::loadTexture(const char* filePath)
{
    int texWidth, texHeight;
    unsigned char* pixels = MediaLoader::loadTexture(filePath, texWidth, texHeight);

    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->CommandBuffers_createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = logicalDevice.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    logicalDevice.unmapMemory(stagingBufferMemory);

    MediaLoader::freePixels(pixels);

    this->CommandBuffers_createImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal,
        textureImage, textureImageMemory);

    textureImageView = Devices_createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    this->CommandBuffers_transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    this->CommandBuffers_copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    this->CommandBuffers_transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

void VulkanAPI::updateDescriptorSets()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorSet& descriptorSet = vkDescriptorSets[i];
        vk::DescriptorBufferInfo bufferInfo;
        vk::DescriptorImageInfo imageInfo;

        bufferInfo = vk::DescriptorBufferInfo(uniformBuffers[i], 0, sizeof(UniformBufferObject));
        imageInfo = vk::DescriptorImageInfo(textureSampler, textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal);

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
        descriptorWrites[0].setDstSet(descriptorSet)
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setBufferInfo(bufferInfo);

        descriptorWrites[1].setDstSet(descriptorSet)
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setImageInfo(imageInfo);

        logicalDevice.updateDescriptorSets(descriptorWrites, nullptr);
    }
}

void VulkanAPI::CommandBuffers_init(const vk::SurfaceKHR& surface)
{
    QueueFamilyIndices queueFamilyIndices = Devices_findQueueFamilies(surface);

    this->CommandBuffers_createCommandPool(queueFamilyIndices.graphicsFamily.value());
    this->CommandBuffers_createDepthResources();

    this->CommandBuffers_createTextureSampler();
}

void VulkanAPI::CommandBuffers_createCommandPool(uint32_t queueFamilyIndex)
{
    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(queueFamilyIndex);
    commandPool = logicalDevice.createCommandPool(poolInfo);
}

void VulkanAPI::CommandBuffers_createDepthResources()
{
    vk::Format depthFormat = Devices_findDepthFormat();
    this->CommandBuffers_createImage(swapchainExtent.width, swapchainExtent.height, depthFormat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);

    depthImageView = Devices_createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void VulkanAPI::CommandBuffers_recreateDepthResources()
{
    this->CommandBuffers_releaseDepthImages();
    this->CommandBuffers_createDepthResources();
}

void VulkanAPI::CommandBuffers_createImage(uint32_t widith, uint32_t height, vk::Format format, vk::ImageTiling tiling,
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

    if (logicalDevice.createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create image!");
    }

    vk::MemoryRequirements memRequirements = logicalDevice.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(Devices_findMemoryType(memRequirements.memoryTypeBits, properties));

    imageMemory = logicalDevice.allocateMemory(allocInfo, nullptr);
    logicalDevice.bindImageMemory(image, imageMemory, 0);
}

void VulkanAPI::CommandBuffers_transitionImageLayout(vk::Image& image, vk::Format format,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = this->CommandBuffers_beginSingleTimeCommands();

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

    this->CommandBuffers_endSingleTimeCommands(commandBuffer);
}

void VulkanAPI::CommandBuffers_copyBufferToImage(vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height)
{
    vk::CommandBuffer commandBuffer = this->CommandBuffers_beginSingleTimeCommands();

    vk::BufferImageCopy region = vk::BufferImageCopy()
        .setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 })
        .setImageOffset({ 0, 0, 0 })
        .setImageExtent({ width, height, 1 });

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    this->CommandBuffers_endSingleTimeCommands(commandBuffer);
}

void VulkanAPI::CommandBuffers_createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

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

    textureSampler = logicalDevice.createSampler(samplerInfo);
}

void VulkanAPI::CommandBuffers_createVertexBuffer(const Model& model, ModelBuffers* modelBuffers)
{
    vk::DeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->CommandBuffers_createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = logicalDevice.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, model.vertices.data(), (size_t)bufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    this->CommandBuffers_createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, modelBuffers->vertexBuffer, modelBuffers->vertexBufferMemory);

    this->CommandBuffers_copyBuffer(stagingBuffer, modelBuffers->vertexBuffer, bufferSize);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

void VulkanAPI::CommandBuffers_createIndexBuffer(const Model& model, ModelBuffers* modelBuffers)
{
    vk::DeviceSize bufferSize = sizeof(model.indices[0]) * model.indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    this->CommandBuffers_createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = logicalDevice.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, model.indices.data(), (size_t)bufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    this->CommandBuffers_createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, modelBuffers->indexBuffer, modelBuffers->indexBufferMemory);

    this->CommandBuffers_copyBuffer(stagingBuffer, modelBuffers->indexBuffer, bufferSize);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

void VulkanAPI::CommandBuffers_createUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        this->CommandBuffers_createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);

        uniformBuffersMapped[i] = logicalDevice.mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

void VulkanAPI::CommandBuffers_createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

    commandBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::CommandBuffer> commandBufferValues = logicalDevice.allocateCommandBuffers(allocInfo);
    for (vk::CommandBuffer& item : commandBufferValues)
    {
        commandBuffers.push_back(item);
    }

    if (commandBuffers.empty())
    {
        throw std::runtime_error("Couldn't allocate command buffer!");
    }
}

uint32_t VulkanAPI::CommandBuffers_getCurrentFrameIndex()
{
    return currentFrame;
}

void VulkanAPI::CommandBuffers_copyBuffer(vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize& size)
{
    vk::CommandBuffer commandBuffer = this->CommandBuffers_beginSingleTimeCommands();

    vk::BufferCopy copyRegion = vk::BufferCopy()
        .setSrcOffset(0).setDstOffset(0)
        .setSize(size);

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    this->CommandBuffers_endSingleTimeCommands(commandBuffer);
}

void VulkanAPI::CommandBuffers_createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    buffer = logicalDevice.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = logicalDevice.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(Devices_findMemoryType(memRequirements.memoryTypeBits, properties));

    bufferMemory = logicalDevice.allocateMemory(allocInfo);
    logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanAPI::CommandBuffers_updateUniformBuffer(const vk::Extent2D& swapchainExtent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), 0.25f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1.0f;

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

vk::CommandBuffer VulkanAPI::CommandBuffers_beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(commandPool)
        .setCommandBufferCount(1);

    std::vector<vk::CommandBuffer> commandBufferValues = logicalDevice.allocateCommandBuffers(allocInfo);
    vk::CommandBuffer result = commandBufferValues.front();

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    result.begin(beginInfo);

    return result;
}

void VulkanAPI::CommandBuffers_endSingleTimeCommands(vk::CommandBuffer& commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setCommandBuffers(commandBuffer);
    std::vector<vk::SubmitInfo> submitInfos = { submitInfo };

    graphicsQueue.submit(submitInfos);
    graphicsQueue.waitIdle();

    logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void VulkanAPI::CommandBuffers_increaseFrame(int maxFramesInFlight)
{
    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void VulkanAPI::CommandBuffers_releaseUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (i < uniformBuffers.size())
        {
            logicalDevice.destroyBuffer(uniformBuffers[i]);
        }

        if (i < uniformBuffersMemory.size())
        {
            logicalDevice.freeMemory(uniformBuffersMemory[i]);
        }
    }
}

void VulkanAPI::CommandBuffers_release()
{
    logicalDevice.destroySampler(textureSampler);
    logicalDevice.destroyImageView(textureImageView);
    logicalDevice.destroyImage(textureImage);
    logicalDevice.freeMemory(textureImageMemory);

    for (std::shared_ptr<ModelBuffers>& item : this->modelBuffersMap)
    {
        logicalDevice.destroyBuffer(item->indexBuffer);
        logicalDevice.freeMemory(item->indexBufferMemory);
        logicalDevice.destroyBuffer(item->vertexBuffer);
        logicalDevice.freeMemory(item->vertexBufferMemory);
    }

    this->CommandBuffers_releaseDepthImages();

    logicalDevice.destroyCommandPool(commandPool);
}

void VulkanAPI::CommandBuffers_releaseDepthImages()
{
    logicalDevice.destroyImageView(depthImageView);
    logicalDevice.destroyImage(depthImage);
    logicalDevice.freeMemory(depthImageMemory);
}
//

// SyncObjects
void VulkanAPI::SyncObjects_init()
{
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled);

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphores[i] = logicalDevice.createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = logicalDevice.createSemaphore(semaphoreInfo);
        inFlightFences[i] = logicalDevice.createFence(fenceInfo);
    }
}

void VulkanAPI::SyncObjects_waitForFence(int currentFrame)
{
    if (logicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't wait for fence!");
    }
}

void VulkanAPI::SyncObjects_resetFence(int currentFrame)
{
    if (logicalDevice.resetFences(1, &inFlightFences[currentFrame]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't reset fence!");
    }
}

void VulkanAPI::SyncObjects_release()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        logicalDevice.destroySemaphore(renderFinishedSemaphores[i]);
        logicalDevice.destroySemaphore(imageAvailableSemaphores[i]);
        logicalDevice.destroyFence(inFlightFences[i]);
    }
}
//

void VulkanAPI::preRelease()
{
    if (instance != nullptr)
    {
        logicalDevice.waitIdle();
        this->Swapchain_release();

        logicalDevice.destroyPipeline(graphicsPipeline);
        logicalDevice.destroyPipelineLayout(pipelineLayout);
        logicalDevice.destroyRenderPass(renderPassRef);

        this->CommandBuffers_releaseUniformBuffers();
        this->DescriptorSets_release();
        this->CommandBuffers_release();

        this->SyncObjects_release();

        logicalDevice.destroy();
        debugMessenger.release(instance, nullptr);
        if (surface != nullptr)
        {
            instance.destroySurfaceKHR(surface);
        }
    }
}

void VulkanAPI::release()
{
    if (instance != nullptr)
    {
        instance.destroy();
    }
}
