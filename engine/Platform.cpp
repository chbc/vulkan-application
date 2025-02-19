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

uint16_t Platform::loadMesh()
{
    return vulkanApi.loadMesh();
}

uint16_t Platform::loadTexture()
{
    return vulkanApi.loadTexture();
}

void Platform::processInput(bool& stillRunning)
{
    sdlApi.processInput(stillRunning);
}

void Platform::drawFrame()
{
    vulkanApi.drawFrame();
}

void Platform::processFrameEnd()
{
    sdlApi.processFrameEnd();
}
