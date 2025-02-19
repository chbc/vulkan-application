#include "MeshLoader.h"
#include "engine/graphics/Mesh.h"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "dependencies/tiny_obj_loader.h"

const char* MODEL_PATH = "../../media/viking_room.obj";

Mesh* MeshLoader::load()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH))
    {
        throw std::runtime_error(warn + err);
    }

    Mesh* result = new Mesh;
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

            result->vertices.push_back(vertex);

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(result->vertices.size());
                result->vertices.push_back(vertex);
            }

            result->indices.push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
        }
    }

    return result;
}
