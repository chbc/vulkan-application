/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
#include "Devices.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "DescriptorSets.h"
#include "CommandBuffers.h"

#include <vector>

struct SwapChainSupportDetails;

class VulkanAPI
{
private:
	SDLAPI *sdlApi;
	DebugMessenger debugMessenger;
	ValidationLayers validationLayers;
	Devices devices;
	Swapchain swapchain;
	RenderPass renderPass;
	DescriptorSets descriptorSets;
	CommandBuffers commandBuffers;

public:
	void init(SDLAPI& sdlApi);
	void drawFrame();

private:
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void createGraphicsPipeline();
	
	void createDescriptorSets();
	void createSyncObjects();
	vk::ShaderModule createShaderModule(const std::vector<char>& code);
	void preRelease();
	void release();

friend class Platform;
};
