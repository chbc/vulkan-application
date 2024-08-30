#include "SDLAPI.h"

#include <SDL2/SDL_syswm.h>

#include <iostream>

SDL_Window* SDLAPI::window = nullptr;

bool SDLAPI::init(SDL_WindowFlags windowFlags)
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);
    if (window == NULL) {
        std::cout << "Could not create SDL window." << std::endl;
        return false;
    }

    return true;
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
    SDL_DestroyWindow(window);
    SDL_Quit();
}
