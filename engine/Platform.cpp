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

void Platform::processInput(bool& stillRunning)
{
    sdlApi.processInput(stillRunning);
}

void Platform::processFrameEnd()
{
    sdlApi.processFrameEnd();
}
