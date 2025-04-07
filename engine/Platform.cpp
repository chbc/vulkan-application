#include "Platform.h"

#include "SDL2/SDL_video.h"

Platform::~Platform()
{
    vulkanApi.preRelease();
    sdlApi.release();
    vulkanApi.release();
}

void Platform::init()
{
    sdlApi.init(SDL_WINDOW_VULKAN);
    vulkanApi.init(sdlApi);
}

size_t Platform::loadModel(const char* filePath)
{
    return vulkanApi.loadModel(filePath);
}

void Platform::loadTexture(const char* filePath)
{
    vulkanApi.loadTexture(filePath);
}

void Platform::updateDescriptorSets(size_t modelId)
{
    vulkanApi.updateDescriptorSets(modelId);
}

void Platform::processInput(bool& stillRunning)
{
    sdlApi.processInput(stillRunning);
}

void Platform::beginDraw()
{
    vulkanApi.beginDraw();
}

void Platform::updateTransform(size_t modelId, const glm::mat4& transform)
{
	vulkanApi.updateTransform(modelId, transform);
}

void Platform::drawItem(size_t itemId)
{
    vulkanApi.drawItem(itemId);
}

void Platform::endDraw()
{
    vulkanApi.endDraw();
}

void Platform::processFrameEnd()
{
    sdlApi.processFrameEnd();
}
