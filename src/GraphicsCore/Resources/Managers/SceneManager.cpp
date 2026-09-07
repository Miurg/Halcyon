#include "GraphicsCore/Resources/Managers/SceneManager.hpp"

#include "GraphicsCore/VulkanUtils.hpp"
#include <utility>

SceneTransformHandle SceneManager::addTransform(PRS transform)
{
	if (!_freeTransformSlots.empty())
	{
		int slot = _freeTransformSlots.back();
		_freeTransformSlots.pop_back();
		transforms[slot] = std::move(transform);
		return SceneTransformHandle{slot};
	}

	transforms.push_back(std::move(transform));
	return SceneTransformHandle{static_cast<int>(transforms.size() - 1)};
}

SceneLightHandle SceneManager::addLight(PointLightComponent light)
{
	if (!_freeLightSlots.empty())
	{
		int slot = _freeLightSlots.back();
		_freeLightSlots.pop_back();
		lights[slot] = std::move(light);
		return SceneLightHandle{slot};
	}

	lights.push_back(std::move(light));
	return SceneLightHandle{static_cast<int>(lights.size() - 1)};
}

SceneNodeHandle SceneManager::addNode(SceneNode node)
{
	if (!_freeNodeSlots.empty())
	{
		int slot = _freeNodeSlots.back();
		_freeNodeSlots.pop_back();
		nodes[slot] = std::move(node);
		return SceneNodeHandle{slot};
	}

	nodes.push_back(std::move(node));
	return SceneNodeHandle{static_cast<int>(nodes.size() - 1)};
}

SceneHandle SceneManager::addScene(const char* path, Scene scene)
{
	std::string normalizedPath = path == nullptr ? std::string{} : VulkanUtils::normalizePath(path);
	auto cached = sceneCache.find(normalizedPath);
	if (cached != sceneCache.end()) return cached->second;

	scene.refCount = 1;
	SceneHandle handle;
	if (!_freeSceneSlots.empty())
	{
		handle.id = _freeSceneSlots.back();
		_freeSceneSlots.pop_back();
		scenes[handle.id] = std::move(scene);
	}
	else
	{
		scenes.push_back(std::move(scene));
		handle.id = static_cast<int>(scenes.size() - 1);
	}
	sceneCache.emplace(std::move(normalizedPath), handle);
	return handle;
}

bool SceneManager::isSceneLoaded(const char* path) const
{
	return getSceneHandle(path).id != -1;
}

SceneHandle SceneManager::getSceneHandle(const char* path) const
{
	const std::string normalizedPath = path == nullptr ? std::string{} : VulkanUtils::normalizePath(path);
	auto it = sceneCache.find(normalizedPath);
	if (it == sceneCache.end()) return SceneHandle{};
	return it->second;
}

void SceneManager::addSceneRef(SceneHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(scenes.size())) return;
	if (scenes[handle.id].refCount <= 0) return;

	scenes[handle.id].refCount++;
}

bool SceneManager::releaseSceneRef(SceneHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(scenes.size())) return false;

	Scene& scene = scenes[handle.id];
	if (scene.refCount <= 0) return false;
	if (--scene.refCount != 0) return false;

	for (SceneNodeHandle node : scene.nodes)
	{
		nodes[node.id] = SceneNode{};
		_freeNodeSlots.push_back(node.id);
	}
	for (SceneTransformHandle transform : scene.transforms)
	{
		transforms[transform.id] = PRS{};
		_freeTransformSlots.push_back(transform.id);
	}
	for (SceneLightHandle light : scene.lights)
	{
		lights[light.id] = PointLightComponent{};
		_freeLightSlots.push_back(light.id);
	}

	for (auto it = sceneCache.begin(); it != sceneCache.end();)
	{
		if (it->second.id == handle.id)
			it = sceneCache.erase(it);
		else
			++it;
	}

	scene = Scene{};
	_freeSceneSlots.push_back(handle.id);
	return true;
}

const PRS& SceneManager::getTransform(SceneTransformHandle handle) const
{
	return transforms[handle.id];
}

const PointLightComponent& SceneManager::getLight(SceneLightHandle handle) const
{
	return lights[handle.id];
}

const SceneNode& SceneManager::getNode(SceneNodeHandle handle) const
{
	return nodes[handle.id];
}

const Scene& SceneManager::getScene(SceneHandle handle) const
{
	return scenes[handle.id];
}

size_t SceneManager::transformCount() const
{
	return transforms.size();
}

size_t SceneManager::lightCount() const
{
	return lights.size();
}

size_t SceneManager::nodeCount() const
{
	return nodes.size();
}

size_t SceneManager::sceneCount() const
{
	return scenes.size();
}
