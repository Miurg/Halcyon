#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include <stdexcept>
#include "GraphicsCore/VulkanUtils.hpp"

namespace
{
constexpr vk::DeviceSize VERTEX_BUFFER_BYTES = 512ull * 1024 * 1024;
constexpr vk::DeviceSize INDEX_BUFFER_BYTES = 512ull * 1024 * 1024;

void createDeviceLocalBuffer(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
                             vk::Buffer& outBuffer, VmaAllocation& outAllocation)
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkBuffer buffer;
	VmaAllocation allocation;
	VkBufferCreateInfo bufferInfoC = (VkBufferCreateInfo)bufferInfo;

	if (vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create geometry buffer!");
	}

	outBuffer = vk::Buffer(buffer);
	outAllocation = allocation;
}
} // namespace

ModelManager::ModelManager(VulkanDevice& vulkanDevice, VmaAllocator allocator)
    : vulkanDevice(vulkanDevice), allocator(allocator)
{
	vertexIndexBuffers.push_back(VertexIndexBuffer());
	VertexIndexBuffer& buffer = vertexIndexBuffers.back();

	createDeviceLocalBuffer(allocator, VERTEX_BUFFER_BYTES,
	                        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	                        buffer.vertexBuffer, buffer.vertexBufferAllocation);
	createDeviceLocalBuffer(allocator, INDEX_BUFFER_BYTES,
	                        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
	                        buffer.indexBuffer, buffer.indexBufferAllocation);

	buffer.vertexAllocator.reset(static_cast<uint32_t>(VERTEX_BUFFER_BYTES / sizeof(Vertex)));
	buffer.indexAllocator.reset(static_cast<uint32_t>(INDEX_BUFFER_BYTES / sizeof(uint32_t)));
}

ModelManager::~ModelManager()
{
	for (auto& meshBuffer : vertexIndexBuffers)
	{
		if (meshBuffer.vertexBuffer)
		{
			vmaDestroyBuffer(allocator, meshBuffer.vertexBuffer, meshBuffer.vertexBufferAllocation);
		}
		if (meshBuffer.indexBuffer)
		{
			vmaDestroyBuffer(allocator, meshBuffer.indexBuffer, meshBuffer.indexBufferAllocation);
		}
	}
}

bool ModelManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

std::optional<GeometryAllocation> ModelManager::allocateGeometry(int bufferIndex, uint32_t vertexCount,
                                                                 uint32_t indexCount)
{
	VertexIndexBuffer& buffer = vertexIndexBuffers[bufferIndex];

	auto vertexBase = buffer.vertexAllocator.allocate(vertexCount);
	if (!vertexBase) return std::nullopt;

	auto indexBase = buffer.indexAllocator.allocate(indexCount);
	if (!indexBase)
	{
		// Roll back the vertex range so a failed index allocation doesn't leak it.
		buffer.vertexAllocator.free(*vertexBase, vertexCount);
		return std::nullopt;
	}

	return GeometryAllocation{*vertexBase, vertexCount, *indexBase, indexCount, bufferIndex};
}

void ModelManager::uploadVertices(int bufferIndex, uint32_t vertexBase, const Vertex* data, uint32_t count)
{
	if (count == 0) return;
	VertexIndexBuffer& buffer = vertexIndexBuffers[bufferIndex];

	vk::DeviceSize byteSize = sizeof(Vertex) * count;
	auto staging = VulkanUtils::createStagingBuffer(data, byteSize, allocator);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	vk::BufferCopy copyRegion{0, sizeof(Vertex) * vertexBase, byteSize};
	cmd.copyBuffer(staging.buffer, buffer.vertexBuffer, copyRegion);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
}

void ModelManager::uploadIndices(int bufferIndex, uint32_t indexBase, const uint32_t* data, uint32_t count)
{
	if (count == 0) return;
	VertexIndexBuffer& buffer = vertexIndexBuffers[bufferIndex];

	vk::DeviceSize byteSize = sizeof(uint32_t) * count;
	auto staging = VulkanUtils::createStagingBuffer(data, byteSize, allocator);

	auto cmd = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
	vk::BufferCopy copyRegion{0, sizeof(uint32_t) * indexBase, byteSize};
	cmd.copyBuffer(staging.buffer, buffer.indexBuffer, copyRegion);
	VulkanUtils::endSingleTimeCommands(cmd, vulkanDevice);

	VulkanUtils::destroyStagingBuffer(staging, allocator);
}

void ModelManager::freeGeometry(const GeometryAllocation& allocation, uint64_t frameNumber)
{
	_pendingGeometryFrees.push_back({allocation, frameNumber + MAX_FRAMES_IN_FLIGHT});
}

void ModelManager::collectGeometryFrees(uint64_t frameNumber)
{
	for (auto it = _pendingGeometryFrees.begin(); it != _pendingGeometryFrees.end();)
	{
		if (it->retireFrame <= frameNumber)
		{
			VertexIndexBuffer& buffer = vertexIndexBuffers[it->allocation.bufferIndex];
			buffer.vertexAllocator.free(it->allocation.vertexBase, it->allocation.vertexCount);
			buffer.indexAllocator.free(it->allocation.indexBase, it->allocation.indexCount);
			it = _pendingGeometryFrees.erase(it);
		}
		else
			++it;
	}
}

int ModelManager::allocateMeshSlot()
{
	if (!_freeMeshSlots.empty())
	{
		int slot = _freeMeshSlots.back();
		_freeMeshSlots.pop_back();
		meshes[slot] = MeshInfo();
		return slot;
	}
	meshes.push_back(MeshInfo());
	return static_cast<int>(meshes.size() - 1);
}

int ModelManager::allocateModelSlot()
{
	if (!_freeModelSlots.empty())
	{
		int slot = _freeModelSlots.back();
		_freeModelSlots.pop_back();
		models[slot] = Model();
		return slot;
	}
	models.push_back(Model());
	return static_cast<int>(models.size() - 1);
}
