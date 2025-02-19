#pragma once

#include <stdint.h>

class Entity
{
private:
	uint16_t meshHandle;
	uint16_t textureHandle;

public:
	Entity(uint16_t arg_meshHandle, uint16_t arg_textureHandle);

};
