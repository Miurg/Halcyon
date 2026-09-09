#include "GraphicsCore/Resources/Factories/SceneInstanceFactory.hpp"
#include <ktx.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"
#include "GraphicsCore/Systems/RenderSystem.hpp"
#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Components/SceneTemplateManagerComponent.hpp"
#include "GraphicsCore/Resources/Components/RenderAssetComponent.hpp"
#include "GraphicsCore/Resources/Components/SceneInstanceComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include <string>
#include <unordered_map>
#include <utility>
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Systems/LightUpdateSystem.hpp"

#include "GraphicsCore/Resources/Factories/GltfLoader.hpp"

glm::mat4 convertGLTFMatrix(const std::vector<double>& matrix)
{
	float m[16];
	for (size_t i = 0; i < 16; ++i)
	{
		m[i] = static_cast<float>(matrix[i]);
	}

	return glm::make_mat4(m);
}

SceneTemplateNodeHandle parseSceneTemplateHierarchy(
    SceneTemplateNodeHandle parentNode, tinygltf::Model& model, SceneTemplateManager& sceneTemplateManager,
    SceneTemplate& sceneTemplate, const std::vector<MeshHandle>& meshSlots, int nodeIndex)
{
	tinygltf::Node& node = model.nodes[nodeIndex];

	glm::vec3 localPosition = {0.0f, 0.0f, 0.0f};
	glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 localScale = {1.0f, 1.0f, 1.0f};
	glm::vec3 skew;
	glm::vec4 perspective;

	// Decompose the node's transformation
	if (node.matrix.size() > 0)
	{
		glm::mat4 localMatrix = convertGLTFMatrix(node.matrix);
		glm::decompose(localMatrix, localScale, localRotation, localPosition, skew, perspective);
	}
	else // Use TRS if matrix is not provided
	{
		if (node.rotation.size() == 4)
		{
			localRotation = glm::quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
			                          static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
		}
		if (node.translation.size() == 3)
		{
			localPosition = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
			                          static_cast<float>(node.translation[2]));
		}
		if (node.scale.size() == 3)
		{
			localScale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
			                       static_cast<float>(node.scale[2]));
		}
	}

	SceneTemplateTransformHandle transformHandle =
	    sceneTemplateManager.addTransform(PRS{localPosition, localRotation, localScale});
	sceneTemplate.transforms.push_back(transformHandle);

	SceneTemplateNode sceneTemplateNode;
	sceneTemplateNode.name = node.name.empty() ? "Node " + std::to_string(nodeIndex) : node.name;
	sceneTemplateNode.transform = transformHandle;
	sceneTemplateNode.parent = parentNode;
	if (node.mesh != -1)
	{
		sceneTemplateNode.mesh = meshSlots[node.mesh];
	}

	auto nodeLightIt = node.extensions.find("KHR_lights_punctual");
	if (nodeLightIt != node.extensions.end())
	{
		auto extIt = model.extensions.find("KHR_lights_punctual");
		if (extIt != model.extensions.end())
		{
			int lightIndex = nodeLightIt->second.Get("light").GetNumberAsInt();
			auto& lightsArr = extIt->second.Get("lights");

			if (lightIndex >= 0 && lightIndex < (int)lightsArr.ArrayLen())
			{
				auto& lightDef = lightsArr.Get(lightIndex);
				PointLightComponent light{};

				light.intensity = (lightDef.Has("intensity") ? (float)lightDef.Get("intensity").GetNumberAsDouble()
				                                             : 1.0f); // cd (candela), as per KHR_lights_punctual spec
				light.radius = lightDef.Has("range") ? (float)lightDef.Get("range").GetNumberAsDouble() : 10.0f;
				light.innerConeAngle = glm::cos(glm::radians(15.0f));
				light.outerConeAngle = glm::cos(glm::radians(30.0f));

				if (lightDef.Has("color"))
				{
					auto& c = lightDef.Get("color");
					light.color = glm::vec3((float)c.Get(0).GetNumberAsDouble(), (float)c.Get(1).GetNumberAsDouble(),
					                        (float)c.Get(2).GetNumberAsDouble());
				}

				std::string typeStr = lightDef.Has("type") ? lightDef.Get("type").Get<std::string>() : "point";
				if (typeStr == "spot")
				{
					light.type = 1;
					if (lightDef.Has("spot"))
					{
						auto& spot = lightDef.Get("spot");
						if (spot.Has("innerConeAngle"))
							light.innerConeAngle = glm::cos((float)spot.Get("innerConeAngle").GetNumberAsDouble());
						if (spot.Has("outerConeAngle"))
							light.outerConeAngle = glm::cos((float)spot.Get("outerConeAngle").GetNumberAsDouble());
					}
				}
				else
					light.type = 0;

				if (node.rotation.size() == 4)
				{
					glm::quat rot((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1],
					              (float)node.rotation[2]);
					light.direction = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
				}

				sceneTemplateNode.light = sceneTemplateManager.addLight(light);
				sceneTemplate.lights.push_back(sceneTemplateNode.light);
			}
		}
	}

	SceneTemplateNodeHandle nodeHandle = sceneTemplateManager.addNode(std::move(sceneTemplateNode));
	sceneTemplate.nodes.push_back(nodeHandle);

	for (int childIndex : node.children)
	{
		parseSceneTemplateHierarchy(nodeHandle, model, sceneTemplateManager, sceneTemplate, meshSlots, childIndex);
	}
	return nodeHandle;
}

Orhescyon::Entity instantiateSceneTemplate(const char path[MAX_PATH_LEN], SceneTemplateHandle sceneTemplateHandle,
                                           GeneralManager& gm,
                                           const SceneTemplateManager& sceneTemplateManager)
{
	const SceneTemplate& sceneTemplate = sceneTemplateManager.getSceneTemplate(sceneTemplateHandle);
	if (sceneTemplate.renderAssets.empty())
		throw std::runtime_error("Cannot instantiate a scene template without a render asset");

	Orhescyon::Entity sceneInstance = gm.createEntity();
	std::string pathString = path;
	size_t lastSlash = pathString.find_last_of("/\\");
	std::string filename = (lastSlash == std::string::npos) ? pathString : pathString.substr(lastSlash + 1);
	gm.addComponentImmediate<NameComponent>(sceneInstance, filename);
	gm.addComponentImmediate<GlobalTransformComponent>(sceneInstance);
	gm.addComponentImmediate<LocalTransformComponent>(sceneInstance);
	gm.addComponentImmediate<RelationshipComponent>(sceneInstance);
	gm.addComponentImmediate<RenderAssetComponent>(sceneInstance, sceneTemplate.renderAssets.front());
	gm.addComponentImmediate<SceneInstanceComponent>(sceneInstance, sceneTemplateHandle);
	gm.subscribeEntityImmediate<TransformSystem>(sceneInstance);

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
			gm.subscribeEntityImmediate<RenderSystem>(entity);
			gm.subscribeEntityImmediate<BufferUpdateSystem>(entity);
		}
		if (sceneTemplateNode.light.id != -1)
		{
			gm.addComponentImmediate<PointLightComponent>(entity, sceneTemplateManager.getLight(sceneTemplateNode.light));
			gm.subscribeEntityImmediate<LightUpdateSystem>(entity);
		}
		gm.subscribeEntityImmediate<TransformSystem>(entity);
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

	return sceneInstance;
}

Orhescyon::Entity SceneInstanceFactory::loadSceneInstance(
    const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager, GeneralManager& gm,
    TextureManager& textureManager, RenderAssetManager& renderAssetManager, MaterialManager& materialManager,
    VulkanDevice& vulkanDevice, VmaAllocator allocator)
{
	SceneTemplateManager& sceneTemplateManager =
	    *gm.getContextComponent<SceneTemplateManagerContext, SceneTemplateManagerComponent>()->sceneTemplateManager;
	SceneTemplateHandle sceneTemplateHandle = sceneTemplateManager.getSceneTemplateHandle(path);
	if (sceneTemplateHandle.id != -1)
	{
		const SceneTemplate& sceneTemplate = sceneTemplateManager.getSceneTemplate(sceneTemplateHandle);
		if (sceneTemplate.renderAssets.empty())
			throw std::runtime_error("Cannot instantiate a scene template without a render asset");
		sceneTemplateManager.addSceneTemplateRef(sceneTemplateHandle);
		renderAssetManager.addRenderAssetRef(sceneTemplate.renderAssets.front());

		Orhescyon::Entity sceneInstance =
		    instantiateSceneTemplate(path, sceneTemplateHandle, gm, sceneTemplateManager);
		std::cout << "Loaded scene instance: " << path << std::endl;
		return sceneInstance;
	}

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = false;

	// Preserve KTX2 raw bytes; decode PNG/JPEG normally via stb_image
	loader.SetImageLoader(
	    [](tinygltf::Image* image, const int, std::string*, std::string*, int, int, const unsigned char* bytes, int size,
	       void*) -> bool
	    {
		    if (image->mimeType == "image/ktx2")
		    {
			    image->image.assign(bytes, bytes + size);
			    image->as_is = true;
			    return true;
		    }
		    int w, h, comp;
		    unsigned char* data = stbi_load_from_memory(bytes, size, &w, &h, &comp, STBI_rgb_alpha);
		    if (!data) return false;
		    image->width = w;
		    image->height = h;
		    image->component = 4;
		    image->bits = 8;
		    image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		    image->image.assign(data, data + static_cast<size_t>(w * h * 4));
		    stbi_image_free(data);
		    return true;
	    },
	    nullptr);

	std::string pathStr = path;
	if (pathStr.size() >= 4 && pathStr.substr(pathStr.size() - 4) == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}

	if (!err.empty())
	{
		throw std::runtime_error("glTF error: " + err);
	}
	if (!ret)
	{
		throw std::runtime_error("Failed to load glTF asset");
	}

	RenderAssetHandle renderAssetHandle = renderAssetManager.getRenderAssetHandle(path);
	if (renderAssetHandle.id != -1)
	{
		renderAssetManager.addRenderAssetRef(renderAssetHandle);
	}
	else
	{
		renderAssetHandle = GltfLoader::loadRenderAssetFromFile(
		    path, vertexIndexBInt, bufferManager, dSetComponent, descriptorManager, model, textureManager,
		    renderAssetManager, materialManager, vulkanDevice, allocator);
	}

	SceneTemplate sceneTemplate;
	sceneTemplate.renderAssets.push_back(renderAssetHandle);

	const int sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	const tinygltf::Scene& gltfScene = model.scenes[sceneIndex];
	for (int rootNodeIndex : gltfScene.nodes)
	{
		parseSceneTemplateHierarchy(SceneTemplateNodeHandle{}, model, sceneTemplateManager, sceneTemplate,
		                            renderAssetManager.getRenderAsset(renderAssetHandle).meshes, rootNodeIndex);
	}
	sceneTemplateHandle = sceneTemplateManager.addSceneTemplate(path, std::move(sceneTemplate));

	Orhescyon::Entity sceneInstance =
	    instantiateSceneTemplate(path, sceneTemplateHandle, gm, sceneTemplateManager);
	std::cout << "Loaded scene instance: " << path << std::endl;
	return sceneInstance;
}

void collectSubtree(Orhescyon::Entity entity, GeneralManager& gm, std::vector<Orhescyon::Entity>& out)
{
	out.push_back(entity);
	RelationshipComponent* rel = gm.getComponent<RelationshipComponent>(entity);
	for (Orhescyon::Entity child = rel->firstChild; child != NULL_ENTITY;)
	{
		Orhescyon::Entity next = gm.getComponent<RelationshipComponent>(child)->nextSibling;
		collectSubtree(child, gm, out);
		child = next;
	}
}

bool SceneInstanceFactory::unloadSceneInstance(Orhescyon::Entity sceneInstance, GeneralManager& gm,
                                               RenderAssetManager& renderAssetManager,
                                               TextureManager& textureManager, MaterialManager& materialManager)
{
	if (!gm.hasComponent<RenderAssetComponent>(sceneInstance) ||
	    !gm.hasComponent<SceneInstanceComponent>(sceneInstance))
		return false;
	RenderAssetHandle renderAssetHandle = gm.getComponent<RenderAssetComponent>(sceneInstance)->renderAsset;
	SceneTemplateHandle sceneTemplateHandle =
	    gm.getComponent<SceneInstanceComponent>(sceneInstance)->sceneTemplate;
	SceneTemplateManager& sceneTemplateManager =
	    *gm.getContextComponent<SceneTemplateManagerContext, SceneTemplateManagerComponent>()->sceneTemplateManager;

	// Entity destruction does not repair neighbours — unlink the instance from its parent's child list first.
	RelationshipComponent* sceneInstanceRelationship = gm.getComponent<RelationshipComponent>(sceneInstance);
	if (sceneInstanceRelationship->parent != NULL_ENTITY)
	{
		RelationshipComponent* parentRel =
		    gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->parent);
		if (parentRel->firstChild == sceneInstance)
			parentRel->firstChild = sceneInstanceRelationship->nextSibling;
		if (sceneInstanceRelationship->prevSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->prevSibling)->nextSibling =
			    sceneInstanceRelationship->nextSibling;
		if (sceneInstanceRelationship->nextSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(sceneInstanceRelationship->nextSibling)->prevSibling =
			    sceneInstanceRelationship->prevSibling;
	}

	// Snapshot the subtree before destroying — entity destruction erases RelationshipComponent.
	std::vector<Orhescyon::Entity> toDestroy;
	collectSubtree(sceneInstance, gm, toDestroy);
	for (Orhescyon::Entity entity : toDestroy) gm.destroyEntityImmediate(entity);

	sceneTemplateManager.releaseSceneTemplateRef(sceneTemplateHandle);
	if (!renderAssetManager.releaseRenderAssetRef(renderAssetHandle)) return true;
	RenderAsset& renderAsset = renderAssetManager.getRenderAsset(renderAssetHandle);

	uint32_t frameNumber = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;
	renderAssetManager.freeGeometry(renderAsset.allocation, frameNumber);
	for (MeshHandle slot : renderAsset.meshes) renderAssetManager.freeMeshSlot(slot);
	for (TextureHandle textureId : renderAsset.textures) textureManager.freeTexture(textureId, frameNumber);
	for (MaterialHandle materialSlot : renderAsset.materials)
		materialManager.freeMaterial(materialSlot, frameNumber);
	renderAssetManager.unregisterRenderAssetPath(renderAssetHandle);
	renderAssetManager.freeRenderAssetSlot(renderAssetHandle);

	return true;
}
