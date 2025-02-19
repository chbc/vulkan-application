#pragma once

#include <vector>
#include <memory>

class Entity;
class Platform;

class Scene
{
private:
	std::vector<std::unique_ptr<Entity>> entities;

public:
	Scene();
	void init();
	void draw(const Platform& platform);
};
