#include "ValidationLayers.h"
#include <vulkan/vulkan.hpp>

ValidationLayers::ValidationLayers()
{
#if defined(_DEBUG)
    data.emplace_back("VK_LAYER_KHRONOS_validation");
#endif
}

bool ValidationLayers::checkSupport()
{
#if defined(_DEBUG)
    uint32_t layerCount;
    if (vk::enumerateInstanceLayerProperties(&layerCount, nullptr) != vk::Result::eSuccess)
    {
        return false;
    }

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    if (vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != vk::Result::eSuccess)
    {
        return false;
    }

    bool result = false;

    for (const char* layerName : this->data)
    {
        result = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                result = true;
                break;
            }
        }
    }

    return result;
#endif

    return true;
}

std::vector<const char*>& ValidationLayers::getData()
{
    return data;
}
