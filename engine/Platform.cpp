#include "Platform.h"
#include "VulkanAPI.h"
#include "SDLAPI.h"

bool Platform::init()
{
    return VulkanAPI::init();
}

void Platform::processInput(bool& stillRunning)
{
    SDLAPI::processInput(stillRunning);
}

void Platform::processFrameEnd()
{
    SDLAPI::processFrameEnd();
}

void Platform::release()
{
    VulkanAPI::release();
}
