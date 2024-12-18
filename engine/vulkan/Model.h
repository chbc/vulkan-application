#pragma once

#include "Vertex.h"
#include <vector>

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};
