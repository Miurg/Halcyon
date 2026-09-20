#include "SmithCore/Renderables.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"
#include "GraphicsCore/Systems/RenderSystem.hpp"
#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "GraphicsCore/Systems/LightUpdateSystem.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/RenderAssetManagerComponent.hpp"
#include "GraphicsCore/Components/SceneTemplateManagerComponent.hpp"
#include "GraphicsCore/Components/MaterialManagerComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/RenderAssetComponent.hpp"
#include "GraphicsCore/Resources/Components/SceneInstanceComponent.hpp"
#include "GraphicsCore/Resources/Factories/SceneInstanceFactory.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
constexpr int DEFAULT_VERTEX_INDEX_BUFFER = 0;

void collectSubtree(Orhescyon::Entity entity, Orhescyon::GeneralManager& gm,
                    std::vector<Orhescyon::Entity>& entities)
{
	entities.push_back(entity);
	RelationshipComponent* relationship = gm.getComponent<RelationshipComponent>(entity);
	if (!relationship) return;

	for (Orhescyon::Entity child = relationship->firstChild; child != NULL_ENTITY;)
	{
		Orhescyon::Entity next = gm.getComponent<RelationshipComponent>(child)->nextSibling;
		collectSubtree(child, gm, entities);
		child = next;
	}
}
} // namespace

void Smith::Renderables::forgeTransform(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, glm::vec3 pos,
                                        glm::quat rot)
{
	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponentImmediate<LocalTransformComponent>(e);
	}

	if (!gm.hasComponent<GlobalTransformComponent>(e))
	{
		gm.addComponentImmediate<GlobalTransformComponent>(e, pos, rot);
	}

	if (!gm.hasComponent<RelationshipComponent>(e))
	{
		gm.addComponentImmediate<RelationshipComponent>(e);
	}

	if (!gm.isSubscribedTo<TransformSystem>(e))
	{
		gm.subscribeEntityImmediate<TransformSystem>(e);
	}

	// Raise the dirty flag so TransformSystem applies pos/rot; construction alone does not.
	GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(e);
	global->setGlobalPosition(pos);
	global->setGlobalRotation(rot);
}

Orhescyon::Entity Smith::Renderables::forgeSceneInstance(Orhescyon::GeneralManager& gm, const char* path,
                                                         int sceneIndex)
{
	BufferManager& bufferManager =
	    *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	BindlessTextureDSetComponent& dSetComponent =
	    *gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManager& descriptorManager =
	    *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	RenderAssetManager& renderAssetManager =
	    *gm.getContextComponent<RenderAssetManagerContext, RenderAssetManagerComponent>()->renderAssetManager;
	SceneTemplateManager& sceneTemplateManager =
	    *gm.getContextComponent<SceneTemplateManagerContext, SceneTemplateManagerComponent>()->sceneTemplateManager;
	MaterialManager& materialManager =
	    *gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;

	SceneTemplateHandle sceneTemplateHandle = SceneInstanceFactory::loadSceneInstance(
	    path, DEFAULT_VERTEX_INDEX_BUFFER, bufferManager, dSetComponent, descriptorManager, textureManager,
	    renderAssetManager, sceneTemplateManager, materialManager, vulkanDevice, allocator, sceneIndex);

	const SceneTemplate& sceneTemplate = sceneTemplateManager.getSceneTemplate(sceneTemplateHandle);
	if (sceneTemplate.renderAsset.id == -1)
		throw std::runtime_error("Cannot instantiate a scene template without a render asset");

	Orhescyon::Entity sceneInstance = gm.createEntity();
	std::string filename = std::filesystem::path(path).filename().string();
	gm.addComponentImmediate<NameComponent>(sceneInstance, filename);
	gm.addComponentImmediate<GlobalTransformComponent>(sceneInstance);
	gm.addComponentImmediate<LocalTransformComponent>(sceneInstance);
	gm.addComponentImmediate<RelationshipComponent>(sceneInstance);
	gm.addComponentImmediate<RenderAssetComponent>(sceneInstance, sceneTemplate.renderAsset);
	gm.addComponentImmediate<SceneInstanceComponent>(sceneInstance, sceneTemplateHandle);
	gm.subscribeEntityDeferred<TransformSystem>(sceneInstance);

	std::unordered_map<int, Orhescyon::Entity> nodeEntities;
	nodeEntities.reserve(sceneTemplate.nodes.size());

	for (SceneTemplateNodeHandle nodeHandle : sceneTemplate.nodes)
	{
		const SceneTemplateNode& sceneTemplateNode = sceneTemplateManager.getNode(nodeHandle);
		const PRS& transform = sceneTemplateManager.getTransform(sceneTemplateNode.transform);

		Orhescyon::Entity entity = gm.createEntity();
		gm.addComponentImmediate<NameComponent>(entity, sceneTemplateNode.name);
		gm.addComponentImmediate<GlobalTransformComponent>(entity);
		gm.addComponentImmediate<LocalTransformComponent>(entity, transform.position, transform.rotation,
		                                                  transform.scale);
		gm.addComponentImmediate<RelationshipComponent>(entity);

		if (sceneTemplateNode.mesh.id != -1)
		{
			gm.addComponentImmediate<MeshInfoComponent>(entity, sceneTemplateNode.mesh);
			gm.subscribeEntityDeferred<RenderSystem>(entity);
			gm.subscribeEntityDeferred<BufferUpdateSystem>(entity);
		}
		if (sceneTemplateNode.light.id != -1)
		{
			gm.addComponentImmediate<PointLightComponent>(entity, sceneTemplateManager.getLight(sceneTemplateNode.light));
			gm.subscribeEntityDeferred<LightUpdateSystem>(entity);
		}
		gm.subscribeEntityDeferred<TransformSystem>(entity);
		nodeEntities.emplace(nodeHandle.id, entity);
	}

	for (SceneTemplateNodeHandle nodeHandle : sceneTemplate.nodes)
	{
		const SceneTemplateNode& sceneTemplateNode = sceneTemplateManager.getNode(nodeHandle);
		Orhescyon::Entity entity = nodeEntities.at(nodeHandle.id);
		Orhescyon::Entity parentEntity =
		    sceneTemplateNode.parent.id == -1 ? sceneInstance : nodeEntities.at(sceneTemplateNode.parent.id);
		gm.getComponent<RelationshipComponent>(parentEntity)->addChild(parentEntity, entity, gm);
	}

	std::cout << "Loaded scene instance: " << path << std::endl;
	return sceneInstance;
}

bool Smith::Renderables::destroySceneInstance(Orhescyon::GeneralManager& gm,
                                              Orhescyon::Entity sceneInstance)
{
	if (!gm.hasComponent<RenderAssetComponent>(sceneInstance) ||
	    !gm.hasComponent<SceneInstanceComponent>(sceneInstance))
		return false;

	RenderAssetHandle renderAssetHandle = gm.getComponent<RenderAssetComponent>(sceneInstance)->renderAsset;
	SceneTemplateHandle sceneTemplateHandle =
	    gm.getComponent<SceneInstanceComponent>(sceneInstance)->sceneTemplate;
	SceneTemplateManager& sceneTemplateManager =
	    *gm.getContextComponent<SceneTemplateManagerContext, SceneTemplateManagerComponent>()->sceneTemplateManager;
	RenderAssetManager& renderAssetManager =
	    *gm.getContextComponent<RenderAssetManagerContext, RenderAssetManagerComponent>()->renderAssetManager;
	TextureManager& textureManager =
	    *gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	MaterialManager& materialManager =
	    *gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;
	uint32_t frameNumber = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;

	RelationshipComponent* sceneInstanceRelationship = gm.getComponent<RelationshipComponent>(sceneInstance);
	if (sceneInstanceRelationship->parent != NULL_ENTITY)
	{
		RelationshipComponent* parentRelationship =
		    gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->parent);
		if (parentRelationship->firstChild == sceneInstance)
			parentRelationship->firstChild = sceneInstanceRelationship->nextSibling;
		if (sceneInstanceRelationship->prevSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->prevSibling)->nextSibling =
			    sceneInstanceRelationship->nextSibling;
		if (sceneInstanceRelationship->nextSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->nextSibling)->prevSibling =
			    sceneInstanceRelationship->prevSibling;
	}

	std::vector<Orhescyon::Entity> entities;
	collectSubtree(sceneInstance, gm, entities);
	for (Orhescyon::Entity entity : entities) gm.destroyEntityImmediate(entity);

	return SceneInstanceFactory::unloadSceneInstance(
	    sceneTemplateHandle, renderAssetHandle, sceneTemplateManager, renderAssetManager, textureManager,
	    materialManager, frameNumber);
}
