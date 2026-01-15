#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../Components/MaterialDSetComponent.hpp"

class DescriptorManager;

class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice);
	~BufferManager();

	MeshInfoComponent createMesh(const char path[MAX_PATH_LEN]);
	bool isMeshLoaded(const char path[MAX_PATH_LEN]);

	int generateTextureData(const char texturePath[MAX_PATH_LEN], vk::Format format, vk::ImageAspectFlags aspectFlags,
	                        MaterialDSetComponent& dSetComponent, DescriptorManager& dManager);
	int createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	int createBuffer(vk::MemoryPropertyFlags propertyBits, uint_fast16_t numberObjects, size_t sizeBuffer,
	                 uint_fast16_t numberBuffers, uint_fast16_t numberBinding, vk::DescriptorSetLayout layout,
	                 DescriptorManager& dManager);

	std::vector<Texture> textures;
	std::unordered_map<std::string, int> texturePaths;
	std::vector<VertexIndexBuffer> meshes;
	std::unordered_map<std::string, MeshInfoComponent> meshPaths;
	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};


	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, uint_fast16_t numberObjects,
	                      size_t sizeBuffer, uint_fast16_t numberBuffers);

	void createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void createTextureSampler(Texture& texture);
	void createShadowSampler(Texture& texture);
	vk::Format findBestFormat();
	vk::Format findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
	                                   vk::FormatFeatureFlags features);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture);
	void createVertexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer);
	void createIndexBuffer(VulkanDevice& vulkanDevice, VertexIndexBuffer& vertexIndexBuffer);

};

struct CameraStucture
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 lightSpaceMatrix;
};

struct ModelSctructure
{
	alignas(16) glm::mat4 model;
};