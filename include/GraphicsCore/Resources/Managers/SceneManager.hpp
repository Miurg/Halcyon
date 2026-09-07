#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/Components/PRSStructure.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstddef>

struct HALCYON_API SceneNode
{
	std::string name;
	MeshHandle mesh;
	SceneTransformHandle transform;
	SceneNodeHandle parent;
	SceneLightHandle light;
};

struct HALCYON_API Scene
{
	std::vector<ModelHandle> models;
	std::vector<SceneNodeHandle> nodes;
	std::vector<SceneTransformHandle> transforms;
	std::vector<SceneLightHandle> lights;
	int refCount = 0;
};

class HALCYON_API SceneManager
{
public:
	SceneTransformHandle addTransform(PRS transform);
	SceneLightHandle addLight(PointLightComponent light);
	SceneNodeHandle addNode(SceneNode node);
	SceneHandle addScene(const char* path, Scene scene);

	bool isSceneLoaded(const char* path) const;
	SceneHandle getSceneHandle(const char* path) const;
	void addSceneRef(SceneHandle handle);
	bool releaseSceneRef(SceneHandle handle);

	const PRS& getTransform(SceneTransformHandle handle) const;
	const PointLightComponent& getLight(SceneLightHandle handle) const;
	const SceneNode& getNode(SceneNodeHandle handle) const;
	const Scene& getScene(SceneHandle handle) const;

	size_t transformCount() const;
	size_t lightCount() const;
	size_t nodeCount() const;
	size_t sceneCount() const;

private:
	std::vector<PRS> transforms;
	std::vector<PointLightComponent> lights;
	std::vector<SceneNode> nodes;
	std::vector<Scene> scenes;
	std::unordered_map<std::string, SceneHandle> sceneCache;
	std::vector<int> _freeTransformSlots;
	std::vector<int> _freeLightSlots;
	std::vector<int> _freeNodeSlots;
	std::vector<int> _freeSceneSlots;
};
