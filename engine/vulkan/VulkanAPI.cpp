#include "VulkanAPI.h"

#include "engine/Utils.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <optional>
#include <set>
#include <array>

#define GLM_FORMCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription()
            .setBinding(0)
            .setStride(sizeof(Vertex))
            .setInputRate(vk::VertexInputRate::eVertex);

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

        attributeDescriptions[0]
            .setBinding(0)
            .setLocation(0)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(offsetof(Vertex, pos));

        attributeDescriptions[1]
            .setBinding(0)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, color));

        return attributeDescriptions;
    }
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices =
{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


vk::SurfaceKHR surface = nullptr;
vk::Instance instance = nullptr;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::Queue graphicsQueue;
vk::Queue presentQueue;

vk::SwapchainKHR swapChain;
std::vector<vk::Image> swapChainImages;
vk::Format swapChainImageFormat;
vk::Extent2D swapChainExtent;
std::vector<vk::ImageView> swapChainImageViews;
vk::RenderPass renderPass;
vk::DescriptorSetLayout descriptorSetLayout;
vk::PipelineLayout pipelineLayout;
vk::Pipeline graphicsPipeline;
std::vector<vk::Framebuffer> swapChainFramebuffers;
vk::CommandPool commandPool;
std::vector<vk::CommandBuffer> commandBuffers;
std::vector<vk::Semaphore> imageAvailableSemaphores;
std::vector<vk::Semaphore> renderFinishedSemaphores;
std::vector<vk::Fence> inFlightFences;
const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;
bool framebufferResized = false;
vk::Buffer vertexBuffer;
vk::DeviceMemory vertexBufferMemory;
vk::Buffer indexBuffer;
vk::DeviceMemory indexBufferMemory;

std::vector<vk::Buffer> uniformBuffers;
std::vector<vk::DeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;

vk::DescriptorPool descriptorPool;
vk::DescriptorSet descriptorSet;

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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

void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    buffer = device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));

    bufferMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(buffer, bufferMemory, 0);
}

void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(commandPool)
        .setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer;
    if (device.allocateCommandBuffers(&allocInfo, &commandBuffer) != vk::Result::eSuccess)
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

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffer);
}

void VulkanAPI::init(SDLAPI& sdlApi)
{
    this->sdlApi = &sdlApi;

    getRequiredExtensions();
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanAPI::drawFrame()
{
    if (device.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't wait for fence!");
    }

    vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame]);

    if (imageIndex.result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    else if ((imageIndex.result != vk::Result::eSuccess) && (imageIndex.result != vk::Result::eSuboptimalKHR))
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    if (device.resetFences(1, &inFlightFences[currentFrame]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("drawFrame() - Couldn't reset fence!");
    }

    updateUniformBuffer(currentFrame);

    commandBuffers[currentFrame].reset();
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex.value);

    vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(waitSemaphores)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffers[currentFrame])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(signalSemaphores);

    if (graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    vk::SwapchainKHR swapChains[] = { swapChain };

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(signalSemaphores)
        .setSwapchainCount(1)
        .setPSwapchains(swapChains)
        .setPImageIndices(&imageIndex.value)
        .setPResults(nullptr);

    vk::Result result = presentQueue.presentKHR(&presentInfo);

    if ((result == vk::Result::eErrorOutOfDateKHR) || (result != vk::Result::eSuboptimalKHR) || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanAPI::createInstance()
{
#if defined(_DEBUG)
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    if (!checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, bot not available!");
    }
#endif

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
    vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
        .setFlags(vk::InstanceCreateFlags())
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data())
        .setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()))
        .setPpEnabledLayerNames(validationLayers.data());

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

void VulkanAPI::setupDebugMessenger()
{
    if (this->debugMessenger.init(instance, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
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

void VulkanAPI::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const vk::PhysicalDevice& device : devices)
    {
        if (isDeviceSuitable(device))
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

void VulkanAPI::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queueFamily)
            .setQueueCount(1)
            .setQueuePriorities(queuePriority);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
        .setPQueueCreateInfos(queueCreateInfos.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
        .setPpEnabledExtensionNames(deviceExtensions.data());

#if defined(_DEBUG)
    createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()))
        .setPpEnabledLayerNames(validationLayers.data());
#else
    createInfo.setEnabledLayerCount(0);
#endif

    device = physicalDevice.createDevice(createInfo);

    if (!device)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}

void VulkanAPI::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ((swapChainSupport.capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.capabilities.maxImageCount))
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(imageCount)
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(swapChainSupport.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setClipped(vk::True)
        .setOldSwapchain(nullptr);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

    swapChain = device.createSwapchainKHR(createInfo);
    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanAPI::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        vk::ComponentMapping components
        {
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
        };

        vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
            .setImage(swapChainImages[i])
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(swapChainImageFormat)
            .setComponents(components)
            .setSubresourceRange(subresourceRange);

        swapChainImageViews[i] = device.createImageView(createInfo);
        if (swapChainImageViews[i] == nullptr)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void VulkanAPI::createRenderPass()
{
    vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
        .setFormat(swapChainImageFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef);

    vk::SubpassDependency dependency = vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
        .setAttachmentCount(1)
        .setPAttachments(&colorAttachment)
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency);

    if (device.createRenderPass(&renderPassInfo, nullptr, &renderPass) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptorSetLayout)
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges(nullptr);

    if (device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shaderStages)
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(nullptr)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(pipelineLayout)
        .setRenderPass(renderPass)
        .setSubpass(0)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);

    vk::Result result;
    std::tie(result, graphicsPipeline) = device.createGraphicsPipeline(nullptr, pipelineInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    device.destroyShaderModule(vertShaderModule);
    device.destroyShaderModule(fragShaderModule);
}

void VulkanAPI::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vk::ImageView attachments[] = { swapChainImageViews[i] };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
            .setRenderPass(renderPass)
            .setAttachmentCount(1)
            .setPAttachments(attachments)
            .setWidth(swapChainExtent.width)
            .setHeight(swapChainExtent.height)
            .setLayers(1);

        swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
    }
}

void VulkanAPI::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());

    commandPool = device.createCommandPool(poolInfo);
}

void VulkanAPI::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    device.unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}

void VulkanAPI::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    device.unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}

void VulkanAPI::createUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);

        uniformBuffersMapped[i] = device.mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

void VulkanAPI::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex };
    vk::DescriptorSetLayoutCreateInfo createInfo{ {}, uboLayoutBinding };
    descriptorSetLayout = device.createDescriptorSetLayout(createInfo);
}

void VulkanAPI::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eUniformBuffer, 1 };
    vk::DescriptorPoolCreateInfo createInfo{ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSize };
    descriptorPool = device.createDescriptorPool(createInfo);
}

void VulkanAPI::createDescriptorSets()
{
    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{ descriptorPool, descriptorSetLayout };
    descriptorSet = device.allocateDescriptorSets(descriptorSetAllocateInfo).front();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo descriptorBufferInfo{ uniformBuffers[i], 0, sizeof(UniformBufferObject) };
        vk::WriteDescriptorSet writeDescriptorSet{ descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, descriptorBufferInfo };
        device.updateDescriptorSets(writeDescriptorSet, nullptr);
    }
}

void VulkanAPI::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(static_cast<uint32_t>(commandBuffers.size()));

    commandBuffers = device.allocateCommandBuffers(allocInfo);
    
    if (commandBuffers.empty())
    {
        throw std::runtime_error("Couldn't allocate command buffer!");
    }
}

void VulkanAPI::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    vk::CommandBufferBeginInfo beginInfo;

    commandBuffer.begin(beginInfo);

    vk::ClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };
    vk::RenderPassBeginInfo renderPassInfo = vk::RenderPassBeginInfo()
        .setRenderPass(renderPass)
        .setFramebuffer(swapChainFramebuffers[imageIndex])
        .setRenderArea(vk::Rect2D{ {0, 0}, swapChainExtent })
        .setClearValueCount(1)
        .setPClearValues(&clearColor);

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f).setY(0.0f)
        .setWidth(static_cast<float>(swapChainExtent.width))
        .setHeight(static_cast<float>(swapChainExtent.height))
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{ {0, 0}, swapChainExtent };
    commandBuffer.setScissor(0, 1, &scissor);

    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void VulkanAPI::createSyncObjects()
{
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
        .setFlags(vk::FenceCreateFlagBits::eSignaled);

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
        inFlightFences[i] = device.createFence(fenceInfo);
    }
}

void VulkanAPI::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1.0f;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

vk::ShaderModule VulkanAPI::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    vk::ShaderModule shaderModule = device.createShaderModule(createInfo);
    return shaderModule;
}

vk::SurfaceFormatKHR VulkanAPI::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

vk::PresentModeKHR VulkanAPI::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

vk::Extent2D VulkanAPI::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(sdlApi->window, &width, &height);

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

SwapChainSupportDetails VulkanAPI::querySwapChainSupport(const vk::PhysicalDevice& device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

bool VulkanAPI::isDeviceSuitable(const vk::PhysicalDevice& device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanAPI::checkDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const vk::ExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanAPI::findQueueFamilies(const vk::PhysicalDevice& device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    for (int i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        if (device.getSurfaceSupportKHR(i, surface) > 0)
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

void VulkanAPI::getRequiredExtensions()
{
    unsigned extension_count;
    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi->window, &extension_count, NULL))
    {
        throw std::exception("Could not get the number of required instance extensions from SDL.");
    }

    extensions.resize(extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(sdlApi->window, &extension_count, extensions.data()))
    {
        throw std::exception("Could not get the names of required instance extensions from SDL.");
    }

#if defined(_DEBUG)
    extensions.push_back("VK_EXT_debug_utils");
#endif
}

bool VulkanAPI::checkValidationLayerSupport()
{
    uint32_t layerCount;
    if (vk::enumerateInstanceLayerProperties(&layerCount, nullptr) != vk::Result::eSuccess)
    {
        return false;
    }

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    if (vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != vk::Result::eSuccess)
    {
        return false;
    }

    bool result = false;

    for (const char* layerName : validationLayers)
    {
        result = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

void VulkanAPI::cleanupSwapChain()
{
    for (vk::Framebuffer& framebuffer : swapChainFramebuffers)
    {
        device.destroyFramebuffer(framebuffer);
    }

    for (const vk::ImageView& imageView : swapChainImageViews)
    {
        device.destroyImageView(imageView);
    }

    device.destroySwapchainKHR(swapChain);
}

void VulkanAPI::recreateSwapChain()
{
    device.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}

void VulkanAPI::preRelease()
{
    if (instance != nullptr)
    {
        device.waitIdle();
        cleanupSwapChain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            device.destroyBuffer(uniformBuffers[i]);
            device.freeMemory(uniformBuffersMemory[i]);
        }

        device.destroyDescriptorPool(descriptorPool);
        device.destroyDescriptorSetLayout(descriptorSetLayout);
        device.destroyBuffer(indexBuffer);
        device.freeMemory(indexBufferMemory);
        device.destroyBuffer(vertexBuffer);
        device.freeMemory(vertexBufferMemory);
        device.destroyPipeline(graphicsPipeline);
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyRenderPass(renderPass);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            device.destroySemaphore(renderFinishedSemaphores[i]);
            device.destroySemaphore(imageAvailableSemaphores[i]);
            device.destroyFence(inFlightFences[i]);
        }

        device.destroyCommandPool(commandPool);
        device.destroy();
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
