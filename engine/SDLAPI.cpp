#include "SDLAPI.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <iostream>

void SDLAPI::init(int windowFlags)
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::exception("Could not initialize SDL.");
    }

    initialized = true;

    window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);
    if (window == NULL)
    {
        throw std::exception("Could not create SDL window.");
    }
}

void SDLAPI::processInput(bool& stillRunning)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

        switch (event.type) {

        case SDL_QUIT:
            stillRunning = false;
            break;

        default:
            // Do nothing.
            break;
        }
    }
}

void SDLAPI::processFrameEnd()
{
    SDL_Delay(17);
}

void SDLAPI::release()
{
    if (window != NULL)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    if (initialized)
    {
        SDL_Quit();
        initialized = false;
    }
}
