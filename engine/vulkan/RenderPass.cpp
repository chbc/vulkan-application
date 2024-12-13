#include "RenderPass.h"
#include "Swapchain.h"
#include "Devices.h"

#include <vulkan/vulkan.hpp>

vk::RenderPass renderPassRef;

void RenderPass::init(Devices& devices, Swapchain& swapchain)
{
    vk::Format swapchainImageFormat = swapchain.getImageFormat();

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
        .setFormat(devices.findDepthFormat())
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

    if (devices.getDevice()->createRenderPass(&renderPassInfo, nullptr, &renderPassRef) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

vk::RenderPassBeginInfo RenderPass::createInfo(Swapchain& swapchain, const vk::Extent2D& extent, uint32_t imageIndex)
{
    vk::Framebuffer framebuffer;
    swapchain.getFramebuffer(imageIndex, framebuffer);

    vk::ClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };
    vk::RenderPassBeginInfo result = vk::RenderPassBeginInfo()
        .setRenderPass(renderPassRef)
        .setFramebuffer(framebuffer)
        .setRenderArea(vk::Rect2D{ {0, 0}, extent })
        .setClearValueCount(1)
        .setPClearValues(&clearColor);

    return result;
}

vk::RenderPass& RenderPass::getRenderPassRef()
{
    return renderPassRef;
}

void RenderPass::release(vk::Device* logicalDevice)
{
    logicalDevice->destroyRenderPass(renderPassRef);
}
