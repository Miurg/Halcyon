#include "AssetManager.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


AssetManager::AssetManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

AssetManager::~AssetManager() {}

MeshInfoComponent AssetManager::createMesh(const char path[MAX_PATH_LEN])
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < meshes.size(); i++)
	{
		if (sizeof(meshes[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			MeshInfoComponent info = addMeshFromFile(path, meshes[i]);
			info.bufferIndex = static_cast<uint32_t>(i);
			meshes[i].createVertexBuffer(vulkanDevice);
			meshes[i].createIndexBuffer(vulkanDevice);
			meshPaths[std::string(path)] = info;
			return info;
		}
	}

	meshes.push_back(VertexIndexBuffer());
	MeshInfoComponent info = addMeshFromFile(path, meshes.back());
	info.bufferIndex = static_cast<uint32_t>(meshes.size() - 1);
	meshes.back().createVertexBuffer(vulkanDevice);
	meshes.back().createIndexBuffer(vulkanDevice);
	meshPaths[std::string(path)] = info;
	return info;
}

bool AssetManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

MeshInfoComponent AssetManager::addMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh)
{
	uint32_t firstIndex = static_cast<uint32_t>(mesh.indices.size());
	int32_t vertexOffset = static_cast<int32_t>(mesh.vertices.size());
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
	{
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
			              attrib.vertices[3 * index.vertex_index + 2]};

			vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
			                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size()) - vertexOffset;
				mesh.vertices.push_back(vertex);
			}

			mesh.indices.push_back(uniqueVertices[vertex]);
		}
	}

	uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size()) - firstIndex;

	MeshInfoComponent info;
	info.indexCount = indexCount;
	info.indexOffset = firstIndex;
	info.vertexOffset = vertexOffset;
	memcpy(info.path, path, MAX_PATH_LEN);
	return info;
}

TextureInfoComponent AssetManager::generateTextureData(const char texturePath[MAX_PATH_LEN])
{
	if (isTextureLoaded(texturePath))
	{
		return texturePaths[std::string(texturePath)];
	}
	textures.push_back(Texture());
	Texture& texture = textures.back();
	AssetManager::createTextureImage(texturePath, texture);
	AssetManager::createTextureImageView(texture);
	AssetManager::createTextureSampler(texture);

	TextureInfoComponent info;
	info.textureIndex = textures.size() - 1;
	texturePaths[std::string(texturePath)] = info;
	return info;
}

bool AssetManager::isTextureLoaded(const char texturePath[MAX_PATH_LEN])
{
	return texturePaths.find(texturePath) != texturePaths.end();
}

void AssetManager::createTextureImage(const char texturePath[MAX_PATH_LEN], Texture& texture)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}
	auto result = VulkanUtils::createBuffer(
	    imageSize, vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vulkanDevice);
	vk::raii::Buffer stagingTextureBuffer = std::move(result.first);
	vk::raii::DeviceMemory stagingTextureBufferMemory = std::move(result.second);

	void* data = stagingTextureBufferMemory.mapMemory(0, imageSize);
	memcpy(data, pixels, imageSize);
	stagingTextureBufferMemory.unmapMemory();

	stbi_image_free(pixels);

	VulkanUtils::createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
	                         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
	                         vk::MemoryPropertyFlagBits::eDeviceLocal, texture.textureImage,
	                         texture.textureImageMemory, vulkanDevice);

	transitionImageLayout(texture.textureImage, vk::ImageLayout::eUndefined,
	                      vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(stagingTextureBuffer, texture.textureImage, static_cast<uint32_t>(texWidth),
	                  static_cast<uint32_t>(texHeight));
	transitionImageLayout(texture.textureImage, vk::ImageLayout::eTransferDstOptimal,
	                      vk::ImageLayout::eShaderReadOnlyOptimal);
}

void AssetManager::createTextureImageView(Texture& texture)
{
	texture.textureImageView = VulkanUtils::createImageView(
	    texture.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, vulkanDevice);
}

void AssetManager::createTextureSampler(Texture& texture)
{
	vk::PhysicalDeviceProperties properties = vulkanDevice.physicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = vk::True;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = vk::False;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	texture.textureSampler = vk::raii::Sampler(vulkanDevice.device, samplerInfo);
}

void AssetManager::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout,
                                         vk::ImageLayout newLayout)
{
	auto commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = image;
	barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}
	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}

void AssetManager::copyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width,
                                     uint32_t height)
{
	vk::raii::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
	region.imageOffset = vk::Offset3D{0, 0, 0};
	region.imageExtent = vk::Extent3D{width, height, 1};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

	VulkanUtils::endSingleTimeCommands(commandBuffer, vulkanDevice);
}