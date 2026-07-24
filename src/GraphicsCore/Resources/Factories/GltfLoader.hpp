#pragma once
#include "GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"
#include "GraphicsCore/Resources/Managers/Vertex.hpp"
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include <memory>
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>
#include "GraphicsCore/Resources/Managers/MeshInfo.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"

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

struct MaterialMaps
{
	std::unordered_map<uint32_t, MaterialHandle> materials;
	std::vector<int> ownedTextures;
};

// Parses glTF files — extracts materials, primitives, and mesh hierarchy into engine resources.
class GltfLoader
{
public:
	static ModelHandle loadModelFromFile(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
	                             BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
	                             tinygltf::Model& model, TextureManager& textureManager, ModelManager& modelManager,
	                             MaterialManager& materialManager, VulkanDevice& vulkanDevice, VmaAllocator allocator);
	static MaterialMaps materialsParser(tinygltf::Model& model, TextureManager& textureManager,
	                                    MaterialManager& materialManager, BindlessTextureDSetComponent& dSetComponent,
	                                    DescriptorManager& descriptorManager, BufferManager& bufferManager,
	                                    const char* filePath, VulkanDevice& vulkanDevice, VmaAllocator allocator);
	static std::vector<PrimitivesInfo> primitiveParser(tinygltf::Mesh& mesh, std::vector<Vertex>& outVertices,
	                                                   std::vector<uint32_t>& outIndices, tinygltf::Model& model,
	                                                   int32_t globalVertexOffset, const MaterialMaps& materialMaps);
	static std::vector<MeshInfo> modelParser(tinygltf::Model model);
	static std::shared_ptr<TextureData> createDefaultWhiteTexture();

private:
	static int loadMaterialTexture(tinygltf::Model& model, const std::map<std::string, tinygltf::Parameter>& params,
	                               const char* paramName, bool isSrgb, int fallback, const char* filePath,
	                               std::vector<int>& ownedTextures, TextureManager& textureManager,
	                               BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
	                               VulkanDevice& vulkanDevice, VmaAllocator allocator);
};
