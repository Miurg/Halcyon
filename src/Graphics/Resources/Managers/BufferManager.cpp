#include "BufferManager.hpp"
#include <stdexcept>
#include "../Factories/LoadFileFactory.hpp"


BufferManager::BufferManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) 
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = *vulkanDevice.physicalDevice;
	allocatorInfo.device = *vulkanDevice.device;
	allocatorInfo.instance = *vulkanDevice.instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;

	VkResult result = vmaCreateAllocator(&allocatorInfo, &this->allocator);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create VMA allocator!");
	}

	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT)};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.maxSets = 1000 * 3 * MAX_FRAMES_IN_FLIGHT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);

	//===Global===
	vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                             vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding shadowBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
	                                             vk::ShaderStageFlagBits::eFragment, nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 2> globalBindings = {globalBinding, shadowBinding};

	vk::DescriptorSetLayoutCreateInfo globalInfo({}, static_cast<uint32_t>(globalBindings.size()),
	                                             globalBindings.data());
	globalSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, globalInfo);

	//===Textures===
	vk::DescriptorSetLayoutBinding textureBinding(0, vk::DescriptorType::eCombinedImageSampler, 1,
	                                              vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutCreateInfo textureInfo({}, 1, &textureBinding);
	textureSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, textureInfo);

	//===Model===
	vk::DescriptorSetLayoutBinding modelBinding(0, vk::DescriptorType::eStorageBuffer, 1,
	                                            vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutCreateInfo modelInfo({}, 1, &modelBinding);
	modelSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, modelInfo);
}

BufferManager::~BufferManager() 
{
	for (auto& texture : textures)
	{
		if (texture.textureSampler)
		{
			(*vulkanDevice.device).destroySampler(texture.textureSampler);
		}

		if (texture.textureImageView)
		{
			(*vulkanDevice.device).destroyImageView(texture.textureImageView);
		}

		if (texture.textureImage)
		{
			vmaDestroyImage(allocator, texture.textureImage, texture.textureImageAllocation);
		}
	}
	for (auto& buffer : buffers)
	{
		for (size_t i = 0; i < buffer.buffer.size(); ++i)
		{
			if (buffer.buffer[i])
			{
				vmaDestroyBuffer(allocator, buffer.buffer[i], buffer.bufferAllocation[i]);
			}
		}
	}

}

MeshInfoComponent BufferManager::createMesh(const char path[MAX_PATH_LEN])
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < meshes.size(); i++)
	{
		if (sizeof(meshes[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			MeshInfoComponent info = LoadFileFactory::addMeshFromFile(path, meshes[i]);
			info.bufferIndex = static_cast<uint32_t>(i);
			meshes[i].createVertexBuffer(vulkanDevice);
			meshes[i].createIndexBuffer(vulkanDevice);
			meshPaths[std::string(path)] = info;
			return info;
		}
	}

	meshes.push_back(VertexIndexBuffer());
	MeshInfoComponent info = LoadFileFactory::addMeshFromFile(path, meshes.back());
	info.bufferIndex = static_cast<uint32_t>(meshes.size() - 1);
	meshes.back().createVertexBuffer(vulkanDevice);
	meshes.back().createIndexBuffer(vulkanDevice);
	meshPaths[std::string(path)] = info;
	return info;
}

bool BufferManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

int BufferManager::createBuffer(vk::MemoryPropertyFlags propertyBits, uint_fast16_t numberObjects, size_t sizeBuffer,
                                uint_fast16_t numberBuffers, uint_fast16_t numberBinding,
                                vk::DescriptorSetLayout layout)
{
	buffers.push_back(Buffer());
	BufferManager::initGlobalBuffer(propertyBits, buffers.back(), numberObjects, sizeBuffer, numberBuffers);
	BufferManager::allocateGlobalDescriptorSets(buffers.back(), sizeBuffer, numberBuffers, numberBinding, layout);
	return buffers.size() - 1;
}

void BufferManager::initGlobalBuffer(vk::MemoryPropertyFlags propertyBits, Buffer& bufferIn, uint_fast16_t numberObjects,
                                  size_t sizeBuffer, uint_fast16_t numberBuffers)
{
	vk::DeviceSize bufferSize = sizeBuffer * numberObjects;

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	if (propertyBits & vk::MemoryPropertyFlagBits::eHostVisible)
	{
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	if (propertyBits & vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}

	for (size_t i = 0; i < numberBuffers; i++)
	{
		VkBuffer bufferC;
		VmaAllocation allocation;
		VmaAllocationInfo resultInfo;

		VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;

		vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &bufferC, &allocation, &resultInfo);

		bufferIn.buffer.push_back(vk::Buffer(bufferC));
		bufferIn.bufferAllocation.push_back(allocation);

		if (propertyBits & vk::MemoryPropertyFlagBits::eHostVisible)
		{
			bufferIn.bufferMapped.push_back(resultInfo.pMappedData);
		}
	}
}

void BufferManager::allocateGlobalDescriptorSets(Buffer& bufferIn, size_t sizeBuffer, uint_fast16_t numberBuffers,
                                                 uint_fast16_t numberBinding, vk::DescriptorSetLayout layout)
{
	std::vector<vk::DescriptorSetLayout> globalLayouts(numberBuffers, layout);

	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(globalLayouts.size());
	allocInfo.pSetLayouts = globalLayouts.data();

	bufferIn.descriptorSet = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
	for (size_t i = 0; i < numberBuffers; i++)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = bufferIn.buffer[i];
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = bufferIn.descriptorSet[i];
		descriptorWrite.dstBinding = numberBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}