#include "ModelFactory.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include "../../Components/GlobalTransformComponent.hpp"
#include "../../Components/LocalTransformComponent.hpp"
#include "../../Components/RelationshipComponent.hpp"
#include "../../Systems/TransformSystem.hpp"
#include "../../Systems/RenderSystem.hpp"
#include "../../Components/NameComponent.hpp"
#include <string>
#include "../../Components/PointLightComponent.hpp"
#include "../../Systems/LightUpdateSystem.hpp"

glm::mat4 convertGLTFMatrix(const std::vector<double>& matrix)
{
	float m[16];
	for (size_t i = 0; i < 16; ++i)
	{
		m[i] = static_cast<float>(matrix[i]);
	}

	return glm::make_mat4(m);
}
Entity ModelFactory::createEntityHierarchy(int parentEntity, tinygltf::Model& model, GeneralManager& gm, int offset,
                                           BufferManager& bManager, int nodeIndex)
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

	Entity entity = gm.createEntity();
	std::string nodeName = node.name.empty() ? "Node " + std::to_string(nodeIndex) : node.name;
	gm.addComponent<NameComponent>(entity, nodeName);
	gm.addComponent<GlobalTransformComponent>(entity);
	gm.addComponent<LocalTransformComponent>(entity, localPosition, localRotation, localScale);
	gm.addComponent<RelationshipComponent>(entity);
	if (node.mesh != -1)
	{
		gm.addComponent<MeshInfoComponent>(entity, offset + node.mesh);
		gm.subscribeEntity<RenderSystem>(entity);
	}
	gm.subscribeEntity<TransformSystem>(entity);

	// Establish parent-child relationship
	if (parentEntity != -1)
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
				    (lightDef.Has("intensity") ? (float)lightDef.Get("intensity").GetNumberAsDouble() / 683.0f
				                               : 1.0f); // Convert from lumens to nits for a point light with 1m radius
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

				// Направление из ротации ноды
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
		createEntityHierarchy(entity, model, gm, offset, bManager, childIndex);
	}
	return entity;
}

Entity ModelFactory::loadModel(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
                               BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
                               GeneralManager& gm, TextureManager& tManager, ModelManager& mManager)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = false;

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

	int offset = GltfLoader::loadModelFromFile(path, vertexIndexBInt, bManager, dSetComponent, dManager, model, tManager,
	                                           mManager);

	mManager.createVertexBuffer(mManager.vertexIndexBuffers[vertexIndexBInt]);
	mManager.createIndexBuffer(mManager.vertexIndexBuffers[vertexIndexBInt]);

	// Create root entity for the model
	Entity modelRootEntity = gm.createEntity();
	std::string pathString = path;
	size_t lastSlash = pathString.find_last_of("/\\");
	std::string filename = (lastSlash == std::string::npos) ? pathString : pathString.substr(lastSlash + 1);
	gm.addComponent<NameComponent>(modelRootEntity, filename);
	gm.addComponent<GlobalTransformComponent>(modelRootEntity);
	gm.addComponent<LocalTransformComponent>(modelRootEntity);
	gm.addComponent<RelationshipComponent>(modelRootEntity);
	gm.subscribeEntity<TransformSystem>(modelRootEntity);

	const int sceneIndex = model.defaultScene > -1 ? model.defaultScene : 0;
	const tinygltf::Scene& scene = model.scenes[sceneIndex];

	for (int rootNodeIndex : scene.nodes)
	{
		createEntityHierarchy(modelRootEntity, model, gm, offset, bManager, rootNodeIndex);
	}
	std::cout << "Loaded model: " << path << " with offset: " << offset << std::endl;
	return modelRootEntity;
}
