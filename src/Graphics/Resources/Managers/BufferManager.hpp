#pragma once

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"
#include <vk_mem_alloc.h>
#include "Texture.hpp"
#include "Buffer.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../VulkanDevice.hpp"
#include "PrimitivesInfo.hpp"
#include "MeshInfo.hpp"
#include "DescriptorManager.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"

class DescriptorManager;

class BufferManager
{
public:
	BufferManager(VulkanDevice& vulkanDevice);
	~BufferManager();

	int createMesh(const char path[MAX_PATH_LEN], BindlessTextureDSetComponent& dSetComponent,
	               DescriptorManager& dManager);
	bool isMeshLoaded(const char path[MAX_PATH_LEN]);

	int generateTextureData(const char texturePath[MAX_PATH_LEN], int texWidth, int texHeight,
	                        const unsigned char* pixels, BindlessTextureDSetComponent& dSetComponent,
	                        DescriptorManager& dManager);
	bool isTextureLoaded(const char texturePath[MAX_PATH_LEN]);
	int createBuffer(vk::MemoryPropertyFlags propertyBits, vk::DeviceSize sizeBuffer, uint_fast16_t numberBuffers,
	                 uint_fast16_t numberBinding, vk::DescriptorSetLayout layout);
	int createShadowMap(uint32_t shadowResolutionX, uint32_t shadowResolutionY);

	std::vector<Texture> textures;
	std::unordered_map<std::string, int> texturePaths;
	std::vector<VertexIndexBuffer> vertexIndexBuffers;
	std::unordered_map<std::string, int> meshPaths;
	std::vector<MeshInfo> meshes;
	std::vector<Buffer> buffers;

private:
	VulkanDevice& vulkanDevice;
	VmaAllocator allocator = {};

	void initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, vk::DeviceSize sizeBuffer,
	                      uint_fast16_t numberBuffers);
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
	int createMeshInternal(const char path[MAX_PATH_LEN], BindlessTextureDSetComponent& dSetComponent,
	                       DescriptorManager& dManager);
};

struct CameraStucture
{
	alignas(16) glm::mat4 cameraSpaceMatrix;
};

struct SunStructure
{
	alignas(16) glm::mat4 lightSpaceMatrix;
	glm::vec4 direction; // xyz: direction, w: padding
	glm::vec4 color;     // rgb: color, w: intensity of light
	glm::vec4 ambient;   // rgb: ambient color, w: intensity of ambient
};

struct PrimitiveSctructure // (16 + 4 + 4 + padding = 32 bytes)
{
	alignas(16) glm::vec4 baseColor; // 16 bytes
	uint32_t transformIndex; // 4 bytes
	uint32_t textureIndex;   // 4 bytes
	uint32_t _padding[2];
};

struct TransformStructure
{
	alignas(16) glm::mat4 model;
};