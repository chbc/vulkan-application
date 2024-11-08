#pragma once

#include <vector>

class ValidationLayers
{
private:
	std::vector<const char*> data;

private:
	ValidationLayers();
	bool checkSupport();
	const std::vector<const char*>& getData() const;

friend class VulkanAPI;
friend class Devices;
};
