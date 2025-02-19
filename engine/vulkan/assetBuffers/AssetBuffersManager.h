#pragma once

#include "../Devices.h"
#include "MeshBufferData.h"
#include "TextureBufferData.h"

#include <unordered_map>

class AssetBuffersManager
{
private:
	Devices devices;
	std::unordered_map<uint16_t, MeshBufferData> MeshesMap;
	std::unordered_map<uint16_t, TextureBufferData> TexturesMap;

public:
	void init(const Devices& arg_devices);
	uint16_t loadMesh();
	uint16_t loadTexture();
};
