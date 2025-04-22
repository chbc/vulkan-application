
// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include "engine/Platform.h"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>

int main()
{
    Platform platform;
	std::vector<size_t> modelIds;
    std::vector<glm::mat4> modelTransforms;

    try
    {
        platform.init();

        size_t modelId = platform.loadModel("../../media/viking_room.obj");
        platform.loadTexture("../../media/viking_room.png");
        platform.updateDescriptorSets(modelId);
        modelIds.emplace_back(modelId);

        modelTransforms.emplace_back(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }));
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

    bool stillRunning = true;
    while(stillRunning) 
    {
        platform.processInput(stillRunning);

        platform.beginDraw();
        for (int i = 0; i < modelIds.size(); ++i)
        {
			size_t id = modelIds[i];

            glm::mat4& transform = modelTransforms[i];
			platform.updateTransform(id, transform);
		    platform.drawItem(id);
        }
		platform.endDraw();

        platform.processFrameEnd();
    }

    return 0;
}

// ubo.model = glm::rotate(glm::mat4(1.0f), 0.25f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
