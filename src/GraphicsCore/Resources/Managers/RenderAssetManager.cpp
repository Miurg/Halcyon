#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"
#include <algorithm>
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

RenderAssetManager::RenderAssetManager(VulkanDevice& vulkanDevice, VmaAllocator allocator)
    : vulkanDevice(vulkanDevice), allocator(allocator)
{
	vertexIndexBuffers.push_back(VertexIndexBuffer());
	VertexIndexBuffer& buffer = vertexIndexBuffers.back();

	createDeviceLocalBuffer(allocator, VERTEX_BUFFER_BYTES,
	                        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst |
	                            vk::BufferUsageFlagBits::eTransferSrc,
	                        buffer.vertexBuffer, buffer.vertexBufferAllocation);
	createDeviceLocalBuffer(allocator, INDEX_BUFFER_BYTES,
	                        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst |
	                            vk::BufferUsageFlagBits::eTransferSrc,
	                        buffer.indexBuffer, buffer.indexBufferAllocation);

	buffer.vertexAllocator.reset(static_cast<uint32_t>(VERTEX_BUFFER_BYTES / sizeof(Vertex)));
	buffer.indexAllocator.reset(static_cast<uint32_t>(INDEX_BUFFER_BYTES / sizeof(uint32_t)));
}

RenderAssetManager::~RenderAssetManager()
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

bool RenderAssetManager::isRenderAssetLoaded(const char path[MAX_PATH_LEN]) const
{
	return renderAssetPaths.find(VulkanUtils::normalizePath(path)) != renderAssetPaths.end();
}

RenderAssetHandle RenderAssetManager::getRenderAssetHandle(const char path[MAX_PATH_LEN]) const
{
	auto it = renderAssetPaths.find(VulkanUtils::normalizePath(path));
	if (it == renderAssetPaths.end()) return RenderAssetHandle{};
	return it->second;
}

void RenderAssetManager::registerRenderAssetPath(const char path[MAX_PATH_LEN], RenderAssetHandle handle)
{
	renderAssetPaths[VulkanUtils::normalizePath(path)] = handle;
}

void RenderAssetManager::unregisterRenderAssetPath(RenderAssetHandle handle)
{
	for (auto it = renderAssetPaths.begin(); it != renderAssetPaths.end();)
	{
		if (it->second.id == handle.id)
			it = renderAssetPaths.erase(it);
		else
			++it;
	}
}

std::optional<GeometryAllocation> RenderAssetManager::allocateGeometry(int bufferIndex, uint32_t vertexCount,
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

void RenderAssetManager::uploadVertices(int bufferIndex, uint32_t vertexBase, const Vertex* data, uint32_t count)
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

void RenderAssetManager::uploadIndices(int bufferIndex, uint32_t indexBase, const uint32_t* data, uint32_t count)
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

void RenderAssetManager::freeGeometry(const GeometryAllocation& allocation, uint64_t frameNumber)
{
	_pendingGeometryFrees.push_back({allocation, frameNumber + MAX_FRAMES_IN_FLIGHT});
}

void RenderAssetManager::collectGeometryFrees(uint64_t frameNumber)
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

void RenderAssetManager::defragment(VertexIndexBuffer& buffer)
{
	int bufferIndex = -1;
	for (size_t i = 0; i < vertexIndexBuffers.size(); ++i)
	{
		if (&vertexIndexBuffers[i] == &buffer)
		{
			bufferIndex = static_cast<int>(i);
			break;
		}
	}
	if (bufferIndex < 0) return;

	vulkanDevice.device.waitIdle();

	// Superseded by the arena rebuild below; kept entries would later free ranges
	// that by then belong to live render assets.
	for (auto it = _pendingGeometryFrees.begin(); it != _pendingGeometryFrees.end();)
	{
		if (it->allocation.bufferIndex == bufferIndex)
			it = _pendingGeometryFrees.erase(it);
		else
			++it;
	}

	std::vector<int> liveRenderAssets;
	for (size_t i = 0; i < renderAssets.size(); ++i)
	{
		if (renderAssets[i].refCount > 0 && renderAssets[i].allocation.bufferIndex == bufferIndex)
			liveRenderAssets.push_back(static_cast<int>(i));
	}

	auto compact = [&](RangeAllocator& arena, vk::Buffer gpuBuffer, vk::DeviceSize elementSize,
	                   uint32_t GeometryAllocation::* base, uint32_t GeometryAllocation::* count,
	                   uint32_t PrimitivesInfo::* offset)
	{
		std::sort(liveRenderAssets.begin(), liveRenderAssets.end(),
		          [&](int a, int b) { return renderAssets[a].allocation.*base < renderAssets[b].allocation.*base; });

		struct Relocation
		{
			int renderAsset;
			uint32_t oldBase;
			uint32_t newBase;
			uint32_t count;
		};
		std::vector<Relocation> relocations;

		arena.reset(arena.capacity());
		for (int renderAssetIndex : liveRenderAssets)
		{
			uint32_t elementCount = renderAssets[renderAssetIndex].allocation.*count;
			if (elementCount == 0) continue;
			uint32_t oldBase = renderAssets[renderAssetIndex].allocation.*base;
			uint32_t newBase = *arena.allocate(elementCount);
			relocations.push_back({renderAssetIndex, oldBase, newBase, elementCount});
		}

		uint32_t usedElements = arena.capacity() - arena.totalFree();
		bool anyMoved = false;
		for (const Relocation& relocation : relocations)
		{
			if (relocation.oldBase != relocation.newBase)
			{
				anyMoved = true;
				break;
			}
		}

		if (anyMoved && usedElements > 0)
		{
			// Same-buffer copies with overlapping regions are UB in Vulkan, and left-packing
			// overlaps routinely — round-trip through a scratch buffer instead.
			vk::Buffer scratch;
			VmaAllocation scratchAllocation = nullptr;
			createDeviceLocalBuffer(allocator, usedElements * elementSize,
			                        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, scratch,
			                        scratchAllocation);

			auto gather = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
			for (const Relocation& relocation : relocations)
			{
				vk::BufferCopy copyRegion{relocation.oldBase * elementSize, relocation.newBase * elementSize,
				                          relocation.count * elementSize};
				gather.copyBuffer(gpuBuffer, scratch, copyRegion);
			}
			VulkanUtils::endSingleTimeCommands(gather, vulkanDevice);

			auto scatter = VulkanUtils::beginSingleTimeCommands(vulkanDevice);
			vk::BufferCopy backRegion{0, 0, usedElements * elementSize};
			scatter.copyBuffer(scratch, gpuBuffer, backRegion);
			VulkanUtils::endSingleTimeCommands(scatter, vulkanDevice);

			vmaDestroyBuffer(allocator, scratch, scratchAllocation);
		}

		for (const Relocation& relocation : relocations)
		{
			renderAssets[relocation.renderAsset].allocation.*base = relocation.newBase;
			if (relocation.newBase == relocation.oldBase) continue;
			for (MeshHandle meshSlot : renderAssets[relocation.renderAsset].meshes)
			{
				for (PrimitivesInfo& primitive : meshes[meshSlot.id].primitives)
				{
					primitive.*offset = primitive.*offset - relocation.oldBase + relocation.newBase;
				}
			}
		}
	};

	compact(buffer.vertexAllocator, buffer.vertexBuffer, sizeof(Vertex), &GeometryAllocation::vertexBase,
	        &GeometryAllocation::vertexCount, &PrimitivesInfo::vertexOffset);
	compact(buffer.indexAllocator, buffer.indexBuffer, sizeof(uint32_t), &GeometryAllocation::indexBase,
	        &GeometryAllocation::indexCount, &PrimitivesInfo::indexOffset);
}

MeshHandle RenderAssetManager::allocateMeshSlot()
{
	if (!_freeMeshSlots.empty())
	{
		int slot = _freeMeshSlots.back();
		_freeMeshSlots.pop_back();
		meshes[slot] = MeshInfo();
		return MeshHandle{slot};
	}
	meshes.push_back(MeshInfo());
	return MeshHandle{static_cast<int>(meshes.size() - 1)};
}

RenderAssetHandle RenderAssetManager::allocateRenderAssetSlot()
{
	if (!_freeRenderAssetSlots.empty())
	{
		int slot = _freeRenderAssetSlots.back();
		_freeRenderAssetSlots.pop_back();
		renderAssets[slot] = RenderAsset();
		renderAssets[slot].refCount = 1;
		return RenderAssetHandle{slot};
	}
	renderAssets.push_back(RenderAsset());
	renderAssets.back().refCount = 1;
	return RenderAssetHandle{static_cast<int>(renderAssets.size() - 1)};
}

void RenderAssetManager::addRenderAssetRef(RenderAssetHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(renderAssets.size())) return;

	renderAssets[handle.id].refCount++;
}

bool RenderAssetManager::releaseRenderAssetRef(RenderAssetHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(renderAssets.size())) return false;
	if (renderAssets[handle.id].refCount <= 0) return false;

	return --renderAssets[handle.id].refCount == 0;
}

void RenderAssetManager::freeMeshSlot(MeshHandle handle)
{
	_freeMeshSlots.push_back(handle.id);
}

void RenderAssetManager::freeRenderAssetSlot(RenderAssetHandle handle)
{
	_freeRenderAssetSlots.push_back(handle.id);
}

size_t RenderAssetManager::meshCount() const
{
	return meshes.size();
}

size_t RenderAssetManager::renderAssetCount() const
{
	return renderAssets.size();
}

size_t RenderAssetManager::freeMeshSlotCount() const
{
	return _freeMeshSlots.size();
}

size_t RenderAssetManager::freeRenderAssetSlotCount() const
{
	return _freeRenderAssetSlots.size();
}

size_t RenderAssetManager::pendingGeometryFreeCount() const
{
	return _pendingGeometryFrees.size();
}

VertexIndexBuffer& RenderAssetManager::getVertexIndexBuffer(int index)
{
	return vertexIndexBuffers[index];
}

MeshInfo& RenderAssetManager::getMesh(MeshHandle handle)
{
	return meshes[handle.id];
}

RenderAsset& RenderAssetManager::getRenderAsset(RenderAssetHandle handle)
{
	return renderAssets[handle.id];
}
