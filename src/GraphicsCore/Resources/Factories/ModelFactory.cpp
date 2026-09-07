#include "GraphicsCore/Resources/Factories/ModelFactory.hpp"
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
#include "GraphicsCore/Components/SceneManagerComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelComponent.hpp"
#include "GraphicsCore/Resources/Components/SceneComponent.hpp"
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

SceneNodeHandle parseSceneHierarchy(SceneNodeHandle parentNode, tinygltf::Model& model, SceneManager& sceneManager,
                                    Scene& scene, const std::vector<MeshHandle>& meshSlots, int nodeIndex)
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

	SceneTransformHandle transformHandle = sceneManager.addTransform(PRS{localPosition, localRotation, localScale});
	scene.transforms.push_back(transformHandle);

	SceneNode sceneNode;
	sceneNode.name = node.name.empty() ? "Node " + std::to_string(nodeIndex) : node.name;
	sceneNode.transform = transformHandle;
	sceneNode.parent = parentNode;
	if (node.mesh != -1)
	{
		sceneNode.mesh = meshSlots[node.mesh];
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

				sceneNode.light = sceneManager.addLight(light);
				scene.lights.push_back(sceneNode.light);
			}
		}
	}

	SceneNodeHandle nodeHandle = sceneManager.addNode(std::move(sceneNode));
	scene.nodes.push_back(nodeHandle);

	for (int childIndex : node.children)
	{
		parseSceneHierarchy(nodeHandle, model, sceneManager, scene, meshSlots, childIndex);
	}
	return nodeHandle;
}

Orhescyon::Entity instantiateScene(const char path[MAX_PATH_LEN], SceneHandle sceneHandle, GeneralManager& gm,
                                   const SceneManager& sceneManager)
{
	const Scene& scene = sceneManager.getScene(sceneHandle);
	if (scene.models.empty()) throw std::runtime_error("Cannot instantiate a scene without a model");

	Orhescyon::Entity modelRootEntity = gm.createEntity();
	std::string pathString = path;
	size_t lastSlash = pathString.find_last_of("/\\");
	std::string filename = (lastSlash == std::string::npos) ? pathString : pathString.substr(lastSlash + 1);
	gm.addComponentImmediate<NameComponent>(modelRootEntity, filename);
	gm.addComponentImmediate<GlobalTransformComponent>(modelRootEntity);
	gm.addComponentImmediate<LocalTransformComponent>(modelRootEntity);
	gm.addComponentImmediate<RelationshipComponent>(modelRootEntity);
	gm.addComponentImmediate<ModelComponent>(modelRootEntity, scene.models.front());
	gm.addComponentImmediate<SceneComponent>(modelRootEntity, sceneHandle);
	gm.subscribeEntityImmediate<TransformSystem>(modelRootEntity);

	std::unordered_map<int, Orhescyon::Entity> nodeEntities;
	nodeEntities.reserve(scene.nodes.size());

	for (SceneNodeHandle nodeHandle : scene.nodes)
	{
		const SceneNode& sceneNode = sceneManager.getNode(nodeHandle);
		const PRS& transform = sceneManager.getTransform(sceneNode.transform);

		Orhescyon::Entity entity = gm.createEntity();
		gm.addComponentImmediate<NameComponent>(entity, sceneNode.name);
		gm.addComponentImmediate<GlobalTransformComponent>(entity);
		gm.addComponentImmediate<LocalTransformComponent>(entity, transform.position, transform.rotation,
		                                                  transform.scale);
		gm.addComponentImmediate<RelationshipComponent>(entity);

		if (sceneNode.mesh.id != -1)
		{
			gm.addComponentImmediate<MeshInfoComponent>(entity, sceneNode.mesh);
			gm.subscribeEntityImmediate<RenderSystem>(entity);
			gm.subscribeEntityImmediate<BufferUpdateSystem>(entity);
		}
		if (sceneNode.light.id != -1)
		{
			gm.addComponentImmediate<PointLightComponent>(entity, sceneManager.getLight(sceneNode.light));
			gm.subscribeEntityImmediate<LightUpdateSystem>(entity);
		}
		gm.subscribeEntityImmediate<TransformSystem>(entity);
		nodeEntities.emplace(nodeHandle.id, entity);
	}

	for (SceneNodeHandle nodeHandle : scene.nodes)
	{
		const SceneNode& sceneNode = sceneManager.getNode(nodeHandle);
		Orhescyon::Entity entity = nodeEntities.at(nodeHandle.id);
		Orhescyon::Entity parentEntity =
		    sceneNode.parent.id == -1 ? modelRootEntity : nodeEntities.at(sceneNode.parent.id);
		gm.getComponent<RelationshipComponent>(parentEntity)->addChild(parentEntity, entity, gm);
	}

	return modelRootEntity;
}

Orhescyon::Entity ModelFactory::loadModel(const char path[MAX_PATH_LEN], int vertexIndexBInt,
                                          BufferManager& bufferManager, BindlessTextureDSetComponent& dSetComponent,
                                          DescriptorManager& descriptorManager, GeneralManager& gm,
                                          TextureManager& textureManager, ModelManager& modelManager,
                                          MaterialManager& materialManager, VulkanDevice& vulkanDevice,
                                          VmaAllocator allocator)
{
	SceneManager& sceneManager = *gm.getContextComponent<SceneManagerContext, SceneManagerComponent>()->sceneManager;
	SceneHandle sceneHandle = sceneManager.getSceneHandle(path);
	if (sceneHandle.id != -1)
	{
		const Scene& scene = sceneManager.getScene(sceneHandle);
		if (scene.models.empty()) throw std::runtime_error("Cannot instantiate a scene without a model");
		sceneManager.addSceneRef(sceneHandle);
		modelManager.addModelRef(scene.models.front());

		Orhescyon::Entity modelRootEntity = instantiateScene(path, sceneHandle, gm, sceneManager);
		std::cout << "Loaded model: " << path << std::endl;
		return modelRootEntity;
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
		throw std::runtime_error("Failed to load glTF model");
	}

	ModelHandle modelHandle = modelManager.getModelHandle(path);
	if (modelHandle.id != -1)
	{
		modelManager.addModelRef(modelHandle);
	}
	else
	{
		modelHandle =
		    GltfLoader::loadModelFromFile(path, vertexIndexBInt, bufferManager, dSetComponent, descriptorManager, model,
		                                  textureManager, modelManager, materialManager, vulkanDevice, allocator);
	}

	Scene scene;
	scene.models.push_back(modelHandle);

	const int sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	const tinygltf::Scene& gltfScene = model.scenes[sceneIndex];
	for (int rootNodeIndex : gltfScene.nodes)
	{
		parseSceneHierarchy(SceneNodeHandle{}, model, sceneManager, scene, modelManager.getModel(modelHandle).meshes,
		                    rootNodeIndex);
	}
	sceneHandle = sceneManager.addScene(path, std::move(scene));

	Orhescyon::Entity modelRootEntity = instantiateScene(path, sceneHandle, gm, sceneManager);
	std::cout << "Loaded model: " << path << std::endl;
	return modelRootEntity;
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

bool ModelFactory::unloadModel(Orhescyon::Entity modelRootEntity, GeneralManager& gm, ModelManager& modelManager,
                               TextureManager& textureManager, MaterialManager& materialManager)
{
	if (!gm.hasComponent<ModelComponent>(modelRootEntity) || !gm.hasComponent<SceneComponent>(modelRootEntity))
		return false;
	ModelHandle modelHandle = gm.getComponent<ModelComponent>(modelRootEntity)->modelIndex;
	SceneHandle sceneHandle = gm.getComponent<SceneComponent>(modelRootEntity)->scene;
	SceneManager& sceneManager = *gm.getContextComponent<SceneManagerContext, SceneManagerComponent>()->sceneManager;

	// Entity destruction does not repair neighbours — unlink the root from its parent's child list first.
	RelationshipComponent* rootRel = gm.getComponent<RelationshipComponent>(modelRootEntity);
	if (rootRel->parent != NULL_ENTITY)
	{
		RelationshipComponent* parentRel = gm.getComponent<RelationshipComponent>(rootRel->parent);
		if (parentRel->firstChild == modelRootEntity) parentRel->firstChild = rootRel->nextSibling;
		if (rootRel->prevSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(rootRel->prevSibling)->nextSibling = rootRel->nextSibling;
		if (rootRel->nextSibling != NULL_ENTITY)
			gm.getComponent<RelationshipComponent>(rootRel->nextSibling)->prevSibling = rootRel->prevSibling;
	}

	// Snapshot the subtree before destroying — entity destruction erases RelationshipComponent.
	std::vector<Orhescyon::Entity> toDestroy;
	collectSubtree(modelRootEntity, gm, toDestroy);
	for (Orhescyon::Entity entity : toDestroy) gm.destroyEntityImmediate(entity);

	sceneManager.releaseSceneRef(sceneHandle);
	if (!modelManager.releaseModelRef(modelHandle)) return true;
	Model& model = modelManager.getModel(modelHandle);

	uint32_t frameNumber = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;
	modelManager.freeGeometry(model.allocation, frameNumber);
	for (MeshHandle slot : model.meshes) modelManager.freeMeshSlot(slot);
	for (TextureHandle textureId : model.textures) textureManager.freeTexture(textureId, frameNumber);
	for (MaterialHandle materialSlot : model.materials) materialManager.freeMaterial(materialSlot, frameNumber);
	modelManager.unregisterModelPath(modelHandle);
	modelManager.freeModelSlot(modelHandle);

	return true;
}
