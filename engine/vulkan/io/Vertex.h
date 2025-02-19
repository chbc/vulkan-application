#pragma once

#include <array>

#define GLM_FORMCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace vk
{
    struct VertexInputBindingDescription;
    struct VertexInputAttributeDescription;
}

struct Vertex
{
public:
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

public:
    bool operator == (const Vertex& other) const;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

namespace std
{

template<> struct hash<Vertex>
{
    size_t operator()(Vertex const& vertex) const
    {
        return  ((hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
            (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};

}
