#include "VulkanAPI.h"

#include "engine/Utils.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <array>

vk::SurfaceKHR surface = nullptr;
vk::Instance instance = nullptr;

const int MAX_FRAMES_IN_FLIGHT = 2;

vk::PipelineLayout pipelineLayout;
vk::Pipeline graphicsPipeline;

void VulkanAPI::init(SDLAPI& sdlApi)
{
    this->sdlApi = &sdlApi;

    createInstance();
    this->debugMessenger.init(instance, nullptr);
    createSurface();
    this->devices.init(instance, surface, this->validationLayers);
    this->swapchain.init(surface, this->sdlApi->window, this->devices);
    this->swapchain.createImageViews(this->devices);
    this->renderPass.init(this->devices, this->swapchain);

    vk::Device* logicalDevice = this->devices.getDevice();

    this->descriptorSets.initLayout(logicalDevice);
    createGraphicsPipeline();
    this->swapchain.createFramebuffers(devices, this->renderPass.getRenderPassRef());
    this->commandBuffers.init(surface, this->devices, MAX_FRAMES_IN_FLIGHT);
    this->descriptorSets.initPool(logicalDevice);
    createDescriptorSets();
    this->commandBuffers.createCommandBuffers(this->devices, MAX_FRAMES_IN_FLIGHT);
    this->syncObjects.init(logicalDevice, MAX_FRAMES_IN_FLIGHT);
}

void VulkanAPI::drawFrame()
{
    uint32_t currentFrame = this->commandBuffers.getCurrentFrameIndex();
    vk::Device* logicalDevice = this->devices.getDevice();
    this->syncObjects.waitForFence(logicalDevice, currentFrame);

    vk::SwapchainKHR* swapchainKHR = swapchain.getSwapchainKHR();
    const vk::Semaphore& currentImageSemaphore = this->syncObjects.getImageSemaphore(currentFrame);
    const vk::Semaphore& currentRenderSemaphore = this->syncObjects.getRenderSemaphore(currentFrame);
    vk::ResultValue<uint32_t> imageIndex = logicalDevice->acquireNextImageKHR(*swapchainKHR, UINT64_MAX, currentImageSemaphore);

    if (imageIndex.result == vk::Result::eErrorOutOfDateKHR)
    {
        this->swapchain.recreate(surface, this->sdlApi->window, this->devices, this->renderPass.getRenderPassRef());
        return;
    }
    else if ((imageIndex.result != vk::Result::eSuccess) && (imageIndex.result != vk::Result::eSuboptimalKHR))
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    this->syncObjects.resetFence(logicalDevice, currentFrame);

    const vk::Extent2D& swapchainExtent = this->swapchain.getExtent();
    this->commandBuffers.updateUniformBuffer(swapchainExtent);

    const vk::RenderPassBeginInfo& renderPassInfo = this->renderPass.createInfo(this->swapchain, swapchainExtent, imageIndex.value);
    this->commandBuffers.recordCommandBuffer(swapchainExtent, renderPassInfo, graphicsPipeline,
        pipelineLayout, this->descriptorSets.getDescriptorSet());
    const vk::CommandBuffer* commandBuffer = this->commandBuffers.getCurrentCommandBuffer();

    vk::Semaphore waitSemaphores[] = { currentImageSemaphore };
    vk::Semaphore signalSemaphores[] = { currentRenderSemaphore };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(waitSemaphores)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(commandBuffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(signalSemaphores);

    const vk::Queue* graphicsQueue = this->devices.getGraphicsQueue();
    const vk::Queue* presentQueue = this->devices.getPresentQueue();

    const vk::Fence& inFlightFence = this->syncObjects.getInFlightFence(currentFrame);
    if (graphicsQueue->submit(1, &submitInfo, inFlightFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(signalSemaphores)
        .setSwapchainCount(1)
        .setPSwapchains(swapchainKHR)
        .setPImageIndices(&imageIndex.value)
        .setPResults(nullptr);

    vk::Result result = presentQueue->presentKHR(&presentInfo);

    if ((result == vk::Result::eErrorOutOfDateKHR) || (result != vk::Result::eSuboptimalKHR) || framebufferResized)
    {
        framebufferResized = false;
        this->swapchain.recreate(surface, this->sdlApi->window, this->devices, this->renderPass.getRenderPassRef());
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    this->commandBuffers.increaseFrame(MAX_FRAMES_IN_FLIGHT);
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = this->descriptorSets.createPipelineLayoutInfo();

    vk::Device* logicalDevice = this->devices.getDevice();
    if (logicalDevice->createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != vk::Result::eSuccess)
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
        .setRenderPass(*this->renderPass.getRenderPassRef())
        .setSubpass(0)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);

    vk::Result result;
    std::tie(result, graphicsPipeline) = logicalDevice->createGraphicsPipeline(nullptr, pipelineInfo);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    logicalDevice->destroyShaderModule(vertShaderModule);
    logicalDevice->destroyShaderModule(fragShaderModule);
}

void VulkanAPI::createDescriptorSets()
{
    vk::Device* logicalDevice = this->devices.getDevice();
    this->descriptorSets.initDescriptorSet(logicalDevice);

    vk::DescriptorSet* descriptorSet = this->descriptorSets.getDescriptorSet();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo descriptorBufferInfo = this->commandBuffers.createDescriptorBufferInfo(i);
        vk::WriteDescriptorSet writeDescriptorSet{ *descriptorSet, 0, 0, vk::DescriptorType::eUniformBuffer, {}, descriptorBufferInfo };
        logicalDevice->updateDescriptorSets(writeDescriptorSet, nullptr);
    }
}

vk::ShaderModule VulkanAPI::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    vk::ShaderModule shaderModule = this->devices.getDevice()->createShaderModule(createInfo);
    return shaderModule;
}

void VulkanAPI::preRelease()
{
    vk::Device* logicalDevice = this->devices.getDevice();

    if (instance != nullptr)
    {
        logicalDevice->waitIdle();
        this->swapchain.release(this->devices);

        this->commandBuffers.releaseUniformBuffers(logicalDevice, MAX_FRAMES_IN_FLIGHT);

        this->descriptorSets.release(logicalDevice);
        this->commandBuffers.release(logicalDevice);
        logicalDevice->destroyPipeline(graphicsPipeline);
        logicalDevice->destroyPipelineLayout(pipelineLayout);
        this->renderPass.release(logicalDevice);

        this->syncObjects.release(logicalDevice, MAX_FRAMES_IN_FLIGHT);

        logicalDevice->destroy();
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
