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
	std::unordered_map<uint32_t, uint32_t> materials;
};

// Parses glTF files — extracts materials, primitives, and mesh hierarchy into engine resources.
class GltfLoader
{
public:
	static int loadModelFromFile(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
	                             BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
	                             tinygltf::Model& model, TextureManager& tManager, ModelManager& mManager);
	static MaterialMaps materialsParser(tinygltf::Model& model, TextureManager& tManager,
	                                    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
	                                    BufferManager& bManager, const char* filePath);
	static std::vector<PrimitivesInfo> primitiveParser(tinygltf::Mesh& mesh, std::vector<Vertex>& outVertices,
	                                                   std::vector<uint32_t>& outIndices, tinygltf::Model& model,
	                                                   int32_t globalVertexOffset, const MaterialMaps& materialMaps);
	static std::vector<MeshInfo> modelParser(tinygltf::Model model);
	static std::shared_ptr<TextureData> createDefaultWhiteTexture();
};
