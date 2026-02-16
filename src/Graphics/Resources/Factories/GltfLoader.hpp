#pragma once
#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>
#include "../Managers/MeshInfo.hpp"
#include "../Managers/BufferManager.hpp"
#include "../Managers/TextureManager.hpp"
#include "../Managers/ModelManager.hpp"

struct TextureData
{
	std::string name;
	std::vector<unsigned char> pixels;
	int width;
	int height;
};

struct LoadedPrimitive
{
	PrimitivesInfo info;
	std::shared_ptr<TextureData> texture;
};

// Parsed material texture maps: base color and normal map indices per glTF material.
struct MaterialMaps
{
	std::unordered_map<uint32_t, uint32_t> baseColor;
	std::unordered_map<uint32_t, uint32_t> normalMap;
};

// Parses glTF files — extracts materials, primitives, and mesh hierarchy into engine resources.
class GltfLoader
{
public:
	static int loadModelFromFile(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
	                             BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
	                             tinygltf::Model model, TextureManager& tManager, ModelManager& mManager);
	static MaterialMaps materialsParser(tinygltf::Model& model, TextureManager& tManager,
	                                    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager);
	static std::vector<PrimitivesInfo> primitiveParser(tinygltf::Mesh& mesh, VertexIndexBuffer& vertexIndexB,
	                                                   tinygltf::Model& model, int32_t globalVertexOffset,
	                                                   int whiteTexture, int defaultNormalTexture,
	                                                   const MaterialMaps& materialMaps);
	static std::vector<MeshInfo> modelParser(tinygltf::Model model);
	static std::shared_ptr<TextureData> createDefaultWhiteTexture();
};
