#pragma once

#include <vector>

class ValidationLayers
{
private:
	std::vector<const char*> data;

private:
	ValidationLayers();
	bool checkSupport();
	std::vector<const char*>& getData();

friend class VulkanAPI;
};
