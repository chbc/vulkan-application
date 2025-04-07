/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
Vulkan C++ Windowed Project Template
Create and destroy a Vulkan surface on an SDL window.
*/

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

        modelId = platform.loadModel("../../media/viking_room.obj");
        platform.updateDescriptorSets(modelId);
        modelIds.emplace_back(modelId);

        modelTransforms.emplace_back(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ -0.75f, 0.0f, 0.0f }));
        modelTransforms.emplace_back(glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.75f, 0.0f, 0.0f }));

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
