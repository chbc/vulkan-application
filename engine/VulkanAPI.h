#pragma once

class VulkanAPI
{
private:
	static bool init();
	static bool createInstance();
	static void release();

friend class Platform;
};
