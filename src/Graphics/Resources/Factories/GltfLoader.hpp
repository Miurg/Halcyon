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

class GltfLoader
{
public:
	static std::vector<PrimitivesInfo> loadModelFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh,
	                                                      BufferManager& bManager,
	                                                      BindlessTextureDSetComponent& dSetComponent,
	                                                      DescriptorManager& dManager);
	static std::unordered_map<uint32_t, uint32_t> materialsParser(tinygltf::Model& model, BufferManager& bManager,
	                                                       BindlessTextureDSetComponent& dSetComponent,
	                                                       DescriptorManager& dManager);
	static std::vector<PrimitivesInfo> primitiveParser(tinygltf::Mesh& mesh, VertexIndexBuffer& vertexIndexB,
	                                            tinygltf::Model& model, int32_t globalVertexOffset);
	static std::vector<MeshInfo> modelParser(tinygltf::Model model);
	static std::shared_ptr<TextureData> createDefaultWhiteTexture();
};
