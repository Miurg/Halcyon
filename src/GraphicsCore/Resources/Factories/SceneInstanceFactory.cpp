#include "GraphicsCore/Resources/Factories/SceneInstanceFactory.hpp"
#include <ktx.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include "GraphicsCore/Components/PointLightComponent.hpp"

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

SceneTemplateHandle SceneInstanceFactory::loadSceneInstance(
    const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager, TextureManager& textureManager,
    RenderAssetManager& renderAssetManager, SceneTemplateManager& sceneTemplateManager,
    MaterialManager& materialManager, VulkanDevice& vulkanDevice, VmaAllocator allocator, int sceneIndex)
{
	if (sceneIndex < -1) throw std::runtime_error("glTF scene index cannot be less than -1");

	auto retainCachedTemplate = [&](SceneTemplateHandle cachedHandle)
	{
		const SceneTemplate& sceneTemplate = sceneTemplateManager.getSceneTemplate(cachedHandle);
		if (sceneTemplate.renderAsset.id == -1)
			throw std::runtime_error("Cannot load a scene template without a render asset");
		sceneTemplateManager.addSceneTemplateRef(cachedHandle);
		renderAssetManager.addRenderAssetRef(sceneTemplate.renderAsset);
		return cachedHandle;
	};

	SceneTemplateHandle sceneTemplateHandle = sceneTemplateManager.getSceneTemplateHandle(path, sceneIndex);
	if (sceneTemplateHandle.id != -1) return retainCachedTemplate(sceneTemplateHandle);

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

	int selectedSceneIndex = -1;
	std::vector<int> rootNodeIndices;
	if (model.scenes.empty())
	{
		if (sceneIndex != -1)
			throw std::runtime_error("Cannot select a scene from a glTF asset without scenes");

		std::vector<bool> childNodes(model.nodes.size(), false);
		for (const tinygltf::Node& node : model.nodes)
		{
			for (int childIndex : node.children)
			{
				if (childIndex < 0 || childIndex >= static_cast<int>(model.nodes.size()))
					throw std::runtime_error("glTF node contains an invalid child index");
				childNodes[childIndex] = true;
			}
		}
		for (int nodeIndex = 0; nodeIndex < static_cast<int>(model.nodes.size()); ++nodeIndex)
		{
			if (!childNodes[nodeIndex]) rootNodeIndices.push_back(nodeIndex);
		}
	}
	else
	{
		selectedSceneIndex = sceneIndex;
		if (selectedSceneIndex == -1)
			selectedSceneIndex = model.defaultScene == -1 ? 0 : model.defaultScene;
		if (selectedSceneIndex < 0 || selectedSceneIndex >= static_cast<int>(model.scenes.size()))
			throw std::runtime_error("glTF scene index is out of range");
		rootNodeIndices = model.scenes[selectedSceneIndex].nodes;
	}

	sceneTemplateHandle = sceneTemplateManager.getSceneTemplateHandle(path, selectedSceneIndex);
	if (sceneTemplateHandle.id != -1)
	{
		if (sceneIndex == -1) sceneTemplateManager.setDefaultSceneTemplate(path, sceneTemplateHandle);
		return retainCachedTemplate(sceneTemplateHandle);
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
	sceneTemplate.renderAsset = renderAssetHandle;

	for (int rootNodeIndex : rootNodeIndices)
	{
		parseSceneTemplateHierarchy(SceneTemplateNodeHandle{}, model, sceneTemplateManager, sceneTemplate,
		                            renderAssetManager.getRenderAsset(renderAssetHandle).meshes, rootNodeIndex);
	}
	sceneTemplateHandle =
	    sceneTemplateManager.addSceneTemplate(path, selectedSceneIndex, std::move(sceneTemplate));
	if (sceneIndex == -1) sceneTemplateManager.setDefaultSceneTemplate(path, sceneTemplateHandle);
	return sceneTemplateHandle;
}

bool SceneInstanceFactory::unloadSceneInstance(SceneTemplateHandle sceneTemplateHandle,
                                               RenderAssetHandle renderAssetHandle,
                                               SceneTemplateManager& sceneTemplateManager,
                                               RenderAssetManager& renderAssetManager,
                                               TextureManager& textureManager, MaterialManager& materialManager,
                                               uint32_t frameNumber)
{
	sceneTemplateManager.releaseSceneTemplateRef(sceneTemplateHandle);
	if (!renderAssetManager.releaseRenderAssetRef(renderAssetHandle)) return true;
	RenderAsset& renderAsset = renderAssetManager.getRenderAsset(renderAssetHandle);

	renderAssetManager.freeGeometry(renderAsset.allocation, frameNumber);
	for (MeshHandle slot : renderAsset.meshes) renderAssetManager.freeMeshSlot(slot);
	for (TextureHandle textureId : renderAsset.textures) textureManager.freeTexture(textureId, frameNumber);
	for (MaterialHandle materialSlot : renderAsset.materials)
		materialManager.freeMaterial(materialSlot, frameNumber);
	renderAssetManager.unregisterRenderAssetPath(renderAssetHandle);
	renderAssetManager.freeRenderAssetSlot(renderAssetHandle);

	return true;
}
