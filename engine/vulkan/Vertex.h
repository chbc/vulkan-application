#pragma once

#include <array>

#define GLM_FORMCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vk
{
    struct VertexInputBindingDescription;
    struct VertexInputAttributeDescription;
}

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};
