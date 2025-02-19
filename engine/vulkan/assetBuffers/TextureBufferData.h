#pragma once

#include <memory>

namespace vk
{
	class Image;
	class DeviceMemory;
	class ImageView;
	class Sampler;
}

struct TextureBufferData
{
	std::shared_ptr<vk::Image> textureImage;
	std::shared_ptr<vk::DeviceMemory> textureImageMemory;
	std::shared_ptr<vk::ImageView> textureImageView;
	std::shared_ptr<vk::Sampler> textureSampler;
};
