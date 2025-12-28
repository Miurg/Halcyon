#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include "../../VulkanUtils.hpp"
#include "../../GameObject.hpp"

class AssetManager
{
public:
	AssetManager(VulkanDevice& vulkanDevice);
	~AssetManager();

	MeshInfoComponent createMesh(const char path[MAX_PATH_LEN]);
	bool isMeshLoaded(const char path[MAX_PATH_LEN]);
	MeshInfoComponent addMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh);
	std::vector<VertexIndexBuffer> meshes;
	std::unordered_map<std::string, MeshInfoComponent> meshPaths;

	size_t generateTextureData(const char texturePath[MAX_PATH_LEN]);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	std::vector<Texture> textures;
	std::unordered_map<std::string, size_t> texturePaths;

private:
	VulkanDevice& vulkanDevice;
	void createTextureImage(const char texturePath[MAX_PATH_LEN], Texture& texture);
	void createTextureImageView(Texture& texture);
	void createTextureSampler(Texture& texture);
	void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width,
	                       uint32_t height);
};