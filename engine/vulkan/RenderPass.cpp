#include "RenderPass.h"
#include "Swapchain.h"
#include "Devices.h"

#include <vulkan/vulkan.hpp>

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

    this->renderPassRef = std::make_shared<vk::RenderPass>();

    if (devices.getDevice()->createRenderPass(&renderPassInfo, nullptr, renderPassRef.get()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

vk::RenderPassBeginInfo RenderPass::createInfo(Swapchain& swapchain, vk::Extent2D& extent, uint32_t imageIndex)
{
    vk::Framebuffer framebuffer;
    swapchain.getFramebuffer(imageIndex, framebuffer);

    vk::ClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };
    vk::RenderPassBeginInfo result = vk::RenderPassBeginInfo()
        .setRenderPass(*this->renderPassRef)
        .setFramebuffer(framebuffer)
        .setRenderArea(vk::Rect2D{ {0, 0}, extent })
        .setClearValueCount(1)
        .setPClearValues(&clearColor);

    return result;
}

vk::RenderPass* RenderPass::getRenderPassRef()
{
    return renderPassRef.get();
}

void RenderPass::release(vk::Device* logicalDevice)
{
    logicalDevice->destroyRenderPass(*renderPassRef.get());
}
