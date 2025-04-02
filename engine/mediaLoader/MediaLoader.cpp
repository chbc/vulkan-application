#include "MediaLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "dependencies/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "dependencies/stb_image.h"

namespace MediaLoader
{

void loadModel(const char* filePath, Model& result)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &error, filePath))
    {
        throw std::runtime_error(error);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex;
            vertex.pos =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord =
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            result.vertices.push_back(vertex);

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(result.vertices.size());
                result.vertices.push_back(vertex);
            }

            result.indices.push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
        }
    }
}

unsigned char* loadTexture(const char* filePath, int& texWidth, int& texHeight)
{
    int texChannels;
    stbi_uc* result = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!result)
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    return result;
}

void freePixels(void* pixels)
{
    stbi_image_free(pixels);
}

}
