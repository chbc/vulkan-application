#include "Scene.h"
#include "engine/graphics/Entity.h"
#include "engine/vulkan/AssetBuffers/AssetBuffersManager.h"
#include "engine/Platform.h"

Scene::Scene()
{
}

void Scene::init()
{
	AssetBuffersManager::loadMesh()
	Entity* entity = new Entity{ mesh, new Texture };
	this->entities.emplace_back(entity);
}

void Scene::draw(const Platform& platform)
{

}
