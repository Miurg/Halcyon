#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../Components/TextureInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include "../../VulkanUtils.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../../Components/CameraComponent.hpp"
#include "../Components/ModelsBuffersComponent.hpp"

class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice);
	~BufferManager();

	MeshInfoComponent createMesh(const char path[MAX_PATH_LEN]);
	bool isMeshLoaded(const char path[MAX_PATH_LEN]);
	std::vector<VertexIndexBuffer> meshes;
	std::unordered_map<std::string, MeshInfoComponent> meshPaths;

	int generateTextureData(const char texturePath[MAX_PATH_LEN], vk::Format format, vk::ImageAspectFlags aspectFlags);
	int createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	std::vector<Texture> textures;
	std::unordered_map<std::string, int> texturePaths;

	int createBuffer(vk::MemoryPropertyFlags propertyBits, uint_fast16_t numberObjects, size_t sizeBuffer,
	                 uint_fast16_t numberBuffers, uint_fast16_t numberBinding, vk::DescriptorSetLayout layout);
	void bindShadowMap(int bufferIndex, vk::ImageView imageView, vk::Sampler sampler);

	vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
	vk::raii::DescriptorSetLayout textureSetLayout = nullptr;
	vk::raii::DescriptorSetLayout modelSetLayout = nullptr;
	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator;

	vk::raii::DescriptorPool descriptorPool = nullptr;

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, uint_fast16_t numberObjects,
	                      size_t sizeBuffer, uint_fast16_t numberBuffers);
	void allocateGlobalDescriptorSets(Buffer& bufferIn, size_t sizeBuffer, uint_fast16_t numberBuffers,
	                                  uint_fast16_t numberBinding, vk::DescriptorSetLayout layout);

	void createImageView(Texture& texture, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void createTextureSampler(Texture& texture);
	void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void allocateTextureDescriptorSets(int textureNumber);
	void createShadowSampler(Texture& texture);
	vk::Format findBestFormat();
	vk::Format findBestSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
	                                   vk::FormatFeatureFlags features);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
	                 vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, Texture& texture);
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