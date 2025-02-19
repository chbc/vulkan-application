#pragma once

#include <memory>

namespace vk
{
	class Buffer;
	class DeviceMemory;
	class Buffer;
	class DeviceMemory;
}

struct MeshBufferData
{
	std::shared_ptr<vk::Buffer> vertexBuffer;
	std::shared_ptr<vk::DeviceMemory> vertexBufferMemory;
	std::shared_ptr<vk::Buffer> indexBuffer;
	std::shared_ptr<vk::DeviceMemory> indexBufferMemory;
};
