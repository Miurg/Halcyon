#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "GraphicsCore/Components/PRSStructure.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstddef>

struct HALCYON_API SceneTemplateNode
{
	std::string name;
	MeshHandle mesh;
	SceneTemplateTransformHandle transform;
	SceneTemplateNodeHandle parent;
	SceneTemplateLightHandle light;
};

struct HALCYON_API SceneTemplate
{
	RenderAssetHandle renderAsset;
	std::vector<SceneTemplateNodeHandle> nodes;
	std::vector<SceneTemplateTransformHandle> transforms;
	std::vector<SceneTemplateLightHandle> lights;
	int refCount = 0;
};

class HALCYON_API SceneTemplateManager
{
public:
	SceneTemplateTransformHandle addTransform(PRS transform);
	SceneTemplateLightHandle addLight(PointLightComponent light);
	SceneTemplateNodeHandle addNode(SceneTemplateNode node);
	SceneTemplateHandle addSceneTemplate(const char* path, int sceneIndex, SceneTemplate sceneTemplate);
	void setDefaultSceneTemplate(const char* path, SceneTemplateHandle handle);

	bool isSceneTemplateLoaded(const char* path, int sceneIndex = -1) const;
	SceneTemplateHandle getSceneTemplateHandle(const char* path, int sceneIndex = -1) const;
	void addSceneTemplateRef(SceneTemplateHandle handle);
	bool releaseSceneTemplateRef(SceneTemplateHandle handle);

	const PRS& getTransform(SceneTemplateTransformHandle handle) const;
	const PointLightComponent& getLight(SceneTemplateLightHandle handle) const;
	const SceneTemplateNode& getNode(SceneTemplateNodeHandle handle) const;
	const SceneTemplate& getSceneTemplate(SceneTemplateHandle handle) const;

	size_t transformCount() const;
	size_t lightCount() const;
	size_t nodeCount() const;
	size_t sceneTemplateCount() const;

private:
	std::vector<PRS> transforms;
	std::vector<PointLightComponent> lights;
	std::vector<SceneTemplateNode> nodes;
	std::vector<SceneTemplate> sceneTemplates;
	std::unordered_map<std::string, std::unordered_map<int, SceneTemplateHandle>> sceneTemplateCache;
	std::vector<int> _freeTransformSlots;
	std::vector<int> _freeLightSlots;
	std::vector<int> _freeNodeSlots;
	std::vector<int> _freeSceneTemplateSlots;
};
