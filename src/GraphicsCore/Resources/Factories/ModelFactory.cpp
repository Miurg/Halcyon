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
#include "GraphicsCore/Resources/Components/ModelComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"
#include <string>
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
Orhescyon::Entity createEntityHierarchy(Orhescyon::Entity parentEntity, tinygltf::Model& model, GeneralManager& gm,
                                           const std::vector<int>& meshSlots, BufferManager& bManager, int nodeIndex)
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

	Orhescyon::Entity entity = gm.createEntity();
	std::string nodeName = node.name.empty() ? "Node " + std::to_string(nodeIndex) : node.name;
	gm.addComponent<NameComponent>(entity, nodeName);
	gm.addComponent<GlobalTransformComponent>(entity);
	gm.addComponent<LocalTransformComponent>(entity, localPosition, localRotation, localScale);
	gm.addComponent<RelationshipComponent>(entity);
	if (node.mesh != -1)
	{
		gm.addComponent<MeshInfoComponent>(entity, meshSlots[node.mesh]);
		gm.subscribeEntity<RenderSystem>(entity);
		gm.subscribeEntity<BufferUpdateSystem>(entity);
	}
	gm.subscribeEntity<TransformSystem>(entity);

	// Establish parent-child relationship
	if (parentEntity != Orhescyon::Entity::invalid())
	{
		RelationshipComponent& relationship = *gm.getComponent<RelationshipComponent>(parentEntity);
		relationship.addChild(parentEntity, entity, gm);
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
				auto* comp = gm.addComponent<PointLightComponent>(entity);
				gm.subscribeEntity<LightUpdateSystem>(entity);

				comp->intensity =
				    (lightDef.Has("intensity") ? (float)lightDef.Get("intensity").GetNumberAsDouble()
				                               : 1.0f); // cd (candela), as per KHR_lights_punctual spec
				comp->radius = lightDef.Has("range") ? (float)lightDef.Get("range").GetNumberAsDouble() : 10.0f;
				comp->innerConeAngle = glm::cos(glm::radians(15.0f));
				comp->outerConeAngle = glm::cos(glm::radians(30.0f));

				if (lightDef.Has("color"))
				{
					auto& c = lightDef.Get("color");
					comp->color = glm::vec3((float)c.Get(0).GetNumberAsDouble(), (float)c.Get(1).GetNumberAsDouble(),
					                        (float)c.Get(2).GetNumberAsDouble());
				}

				std::string typeStr = lightDef.Has("type") ? lightDef.Get("type").Get<std::string>() : "point";
				if (typeStr == "spot")
				{
					comp->type = 1;
					if (lightDef.Has("spot"))
					{
						auto& spot = lightDef.Get("spot");
						if (spot.Has("innerConeAngle"))
							comp->innerConeAngle = glm::cos((float)spot.Get("innerConeAngle").GetNumberAsDouble());
						if (spot.Has("outerConeAngle"))
							comp->outerConeAngle = glm::cos((float)spot.Get("outerConeAngle").GetNumberAsDouble());
					}
				}
				else
					comp->type = 0;

				if (node.rotation.size() == 4)
				{
					glm::quat rot((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1],
					              (float)node.rotation[2]);
					comp->direction = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
				}
			}
		}
	}

	for (int childIndex : node.children)
	{
		createEntityHierarchy(entity, model, gm, meshSlots, bManager, childIndex);
	}
	return entity;
}

Orhescyon::Entity ModelFactory::loadModel(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
                               BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
                               GeneralManager& gm, TextureManager& tManager, ModelManager& mManager,
                               VulkanDevice& vulkanDevice, VmaAllocator allocator)
{
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
		    if (!data)
			    return false;
		    image->width      = w;
		    image->height     = h;
		    image->component  = 4;
		    image->bits       = 8;
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

	int modelIndex;
	if (mManager.isModelLoaded(path))
	{
		modelIndex = mManager.modelPaths[path];
		mManager.getModel(modelIndex).refCount++;
	}
	else
	{
		modelIndex = GltfLoader::loadModelFromFile(path, vertexIndexBInt, bManager, dSetComponent, dManager, model,
		                                           tManager, mManager, vulkanDevice, allocator);
	}

	// Create root entity for the model
	Orhescyon::Entity modelRootEntity = gm.createEntity();
	std::string pathString = path;
	size_t lastSlash = pathString.find_last_of("/\\");
	std::string filename = (lastSlash == std::string::npos) ? pathString : pathString.substr(lastSlash + 1);
	gm.addComponent<NameComponent>(modelRootEntity, filename);
	gm.addComponent<GlobalTransformComponent>(modelRootEntity);
	gm.addComponent<LocalTransformComponent>(modelRootEntity);
	gm.addComponent<RelationshipComponent>(modelRootEntity);
	gm.addComponent<ModelComponent>(modelRootEntity, modelIndex);
	gm.subscribeEntity<TransformSystem>(modelRootEntity);

	const int sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	const tinygltf::Scene& scene = model.scenes[sceneIndex];

	for (int rootNodeIndex : scene.nodes)
	{
		createEntityHierarchy(modelRootEntity, model, gm, mManager.getModel(modelIndex).meshes, bManager, rootNodeIndex);
	}
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

bool ModelFactory::unloadModel(Orhescyon::Entity modelRootEntity, GeneralManager& gm, ModelManager& mManager,
                               TextureManager& tManager)
{
	if (!gm.hasComponent<ModelComponent>(modelRootEntity)) return false;
	int modelIndex = gm.getComponent<ModelComponent>(modelRootEntity)->modelIndex;

	// destroyEntity does not repair neighbours — unlink the root from its parent's child list first.
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

	// Snapshot the subtree before destroying — destroyEntity erases RelationshipComponent.
	std::vector<Orhescyon::Entity> toDestroy;
	collectSubtree(modelRootEntity, gm, toDestroy);
	for (Orhescyon::Entity entity : toDestroy) gm.destroyEntity(entity);

	Model& model = mManager.getModel(modelIndex);
	model.refCount--;
	if (model.refCount > 0) return true;

	uint32_t frameNumber = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>()->frameNumber;
	mManager.freeGeometry(model.allocation, frameNumber);
	for (int slot : model.meshes) mManager.freeMeshSlot(slot);
	for (int textureId : model.textures) tManager.freeTexture(TextureHandle{textureId}, frameNumber);
	for (int materialSlot : model.materials) tManager.freeMaterial(materialSlot, frameNumber);
	for (auto it = mManager.modelPaths.begin(); it != mManager.modelPaths.end();)
	{
		if (it->second == modelIndex)
			it = mManager.modelPaths.erase(it);
		else
			++it;
	}
	mManager.freeModelSlot(modelIndex);

	return true;
}
