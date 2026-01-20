#include "LoadFileFactory.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "../../VulkanUtils.hpp"

MeshInfo LoadFileFactory::addMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh)
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

	MeshInfo info;
	info.indexCount = indexCount;
	info.indexOffset = firstIndex;
	info.vertexOffset = vertexOffset;
	memcpy(info.path, path, MAX_PATH_LEN);
	return info;
}

std::tuple<int, int> LoadFileFactory::getSizesFromFileTexture(const char texturePath[MAX_PATH_LEN])
{
	int texWidth, texHeight, texChannels;
	stbi_info(texturePath, &texWidth, &texHeight, &texChannels);
	return {texWidth, texHeight};
}

void LoadFileFactory::uploadTextureFromFile(const char* texturePath, Texture& texture, VmaAllocator& allocator,
                                            VulkanDevice& vulkanDevice)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;

	vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	stbi_image_free(pixels);

	transitionImageLayout(texture.textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
	                      vulkanDevice);

	VulkanUtils::copyBufferToImage(stagingBuffer, texture.textureImage, static_cast<uint32_t>(texWidth),
	                               static_cast<uint32_t>(texHeight), vulkanDevice);

	transitionImageLayout(texture.textureImage, vk::ImageLayout::eTransferDstOptimal,
	                      vk::ImageLayout::eShaderReadOnlyOptimal, vulkanDevice);

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void LoadFileFactory::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                            VulkanDevice& vulkanDevice)
{
	auto commandBuffer = VulkanUtils::beginSingleTimeCommands(vulkanDevice);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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