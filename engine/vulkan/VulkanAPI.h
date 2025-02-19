/*
* Declaração adiantada de uma estrutura 'VulkanMembers'.
* Definição desta estrutura com todas as variáveis vulkan no cpp.
*/

#pragma once

#include "engine/sdl/SDLAPI.h"
#include "DebugMessenger.h"
#include "ValidationLayers.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "DescriptorSets.h"
#include "CommandBuffers.h"
#include "SyncObjects.h"
#include "assetBuffers/AssetBuffersManager.h"

#include <vector>

struct SwapChainSupportDetails;
struct Mesh;

class VulkanAPI
{
private:
	SDLAPI *sdlApi;
	DebugMessenger debugMessenger;
	ValidationLayers validationLayers;
	Swapchain swapchain;
	RenderPass renderPass;
	DescriptorSets descriptorSets;
	CommandBuffers commandBuffers;
	SyncObjects syncObjects;
	AssetBuffersManager assetsBufferManager;

	bool framebufferResized = false;

public:
	void init(SDLAPI& sdlApi);
	uint16_t loadMesh();
	uint16_t loadTexture();
	void drawFrame();

private:
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	void createSurface();
	void createGraphicsPipeline();
	
	void createDescriptorSets();
	vk::ShaderModule createShaderModule(const std::vector<char>& code);
	void preRelease();
	void release();

friend class Platform;
};
