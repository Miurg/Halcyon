#include "GraphicsCore/Resources/Managers/SceneTemplateManager.hpp"

#include "GraphicsCore/VulkanUtils.hpp"
#include <utility>

SceneTemplateTransformHandle SceneTemplateManager::addTransform(PRS transform)
{
	if (!_freeTransformSlots.empty())
	{
		int slot = _freeTransformSlots.back();
		_freeTransformSlots.pop_back();
		transforms[slot] = std::move(transform);
		return SceneTemplateTransformHandle{slot};
	}

	transforms.push_back(std::move(transform));
	return SceneTemplateTransformHandle{static_cast<int>(transforms.size() - 1)};
}

SceneTemplateLightHandle SceneTemplateManager::addLight(PointLightComponent light)
{
	if (!_freeLightSlots.empty())
	{
		int slot = _freeLightSlots.back();
		_freeLightSlots.pop_back();
		lights[slot] = std::move(light);
		return SceneTemplateLightHandle{slot};
	}

	lights.push_back(std::move(light));
	return SceneTemplateLightHandle{static_cast<int>(lights.size() - 1)};
}

SceneTemplateNodeHandle SceneTemplateManager::addNode(SceneTemplateNode node)
{
	if (!_freeNodeSlots.empty())
	{
		int slot = _freeNodeSlots.back();
		_freeNodeSlots.pop_back();
		nodes[slot] = std::move(node);
		return SceneTemplateNodeHandle{slot};
	}

	nodes.push_back(std::move(node));
	return SceneTemplateNodeHandle{static_cast<int>(nodes.size() - 1)};
}

SceneTemplateHandle SceneTemplateManager::addSceneTemplate(const char* path, int sceneIndex,
                                                            SceneTemplate sceneTemplate)
{
	std::string normalizedPath = path == nullptr ? std::string{} : VulkanUtils::normalizePath(path);
	auto& cachedScenes = sceneTemplateCache[normalizedPath];
	auto cached = cachedScenes.find(sceneIndex);
	if (cached != cachedScenes.end()) return cached->second;

	sceneTemplate.refCount = 1;
	SceneTemplateHandle handle;
	if (!_freeSceneTemplateSlots.empty())
	{
		handle.id = _freeSceneTemplateSlots.back();
		_freeSceneTemplateSlots.pop_back();
		sceneTemplates[handle.id] = std::move(sceneTemplate);
	}
	else
	{
		sceneTemplates.push_back(std::move(sceneTemplate));
		handle.id = static_cast<int>(sceneTemplates.size() - 1);
	}
	cachedScenes.emplace(sceneIndex, handle);
	return handle;
}

void SceneTemplateManager::setDefaultSceneTemplate(const char* path, SceneTemplateHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(sceneTemplates.size())) return;
	if (sceneTemplates[handle.id].refCount <= 0) return;

	std::string normalizedPath = path == nullptr ? std::string{} : VulkanUtils::normalizePath(path);
	sceneTemplateCache[normalizedPath][-1] = handle;
}

bool SceneTemplateManager::isSceneTemplateLoaded(const char* path, int sceneIndex) const
{
	return getSceneTemplateHandle(path, sceneIndex).id != -1;
}

SceneTemplateHandle SceneTemplateManager::getSceneTemplateHandle(const char* path, int sceneIndex) const
{
	const std::string normalizedPath = path == nullptr ? std::string{} : VulkanUtils::normalizePath(path);
	auto pathIt = sceneTemplateCache.find(normalizedPath);
	if (pathIt == sceneTemplateCache.end()) return SceneTemplateHandle{};
	auto sceneIt = pathIt->second.find(sceneIndex);
	if (sceneIt == pathIt->second.end()) return SceneTemplateHandle{};
	return sceneIt->second;
}

void SceneTemplateManager::addSceneTemplateRef(SceneTemplateHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(sceneTemplates.size())) return;
	if (sceneTemplates[handle.id].refCount <= 0) return;

	sceneTemplates[handle.id].refCount++;
}

bool SceneTemplateManager::releaseSceneTemplateRef(SceneTemplateHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(sceneTemplates.size())) return false;

	SceneTemplate& sceneTemplate = sceneTemplates[handle.id];
	if (sceneTemplate.refCount <= 0) return false;
	if (--sceneTemplate.refCount != 0) return false;

	for (SceneTemplateNodeHandle node : sceneTemplate.nodes)
	{
		nodes[node.id] = SceneTemplateNode{};
		_freeNodeSlots.push_back(node.id);
	}
	for (SceneTemplateTransformHandle transform : sceneTemplate.transforms)
	{
		transforms[transform.id] = PRS{};
		_freeTransformSlots.push_back(transform.id);
	}
	for (SceneTemplateLightHandle light : sceneTemplate.lights)
	{
		lights[light.id] = PointLightComponent{};
		_freeLightSlots.push_back(light.id);
	}

	for (auto pathIt = sceneTemplateCache.begin(); pathIt != sceneTemplateCache.end();)
	{
		auto& cachedScenes = pathIt->second;
		for (auto sceneIt = cachedScenes.begin(); sceneIt != cachedScenes.end();)
		{
			if (sceneIt->second.id == handle.id)
				sceneIt = cachedScenes.erase(sceneIt);
			else
				++sceneIt;
		}

		if (cachedScenes.empty())
			pathIt = sceneTemplateCache.erase(pathIt);
		else
			++pathIt;
	}

	sceneTemplate = SceneTemplate{};
	_freeSceneTemplateSlots.push_back(handle.id);
	return true;
}

const PRS& SceneTemplateManager::getTransform(SceneTemplateTransformHandle handle) const
{
	return transforms[handle.id];
}

const PointLightComponent& SceneTemplateManager::getLight(SceneTemplateLightHandle handle) const
{
	return lights[handle.id];
}

const SceneTemplateNode& SceneTemplateManager::getNode(SceneTemplateNodeHandle handle) const
{
	return nodes[handle.id];
}

const SceneTemplate& SceneTemplateManager::getSceneTemplate(SceneTemplateHandle handle) const
{
	return sceneTemplates[handle.id];
}

size_t SceneTemplateManager::transformCount() const
{
	return transforms.size();
}

size_t SceneTemplateManager::lightCount() const
{
	return lights.size();
}

size_t SceneTemplateManager::nodeCount() const
{
	return nodes.size();
}

size_t SceneTemplateManager::sceneTemplateCount() const
{
	return sceneTemplates.size();
}
