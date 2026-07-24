#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/VulkanConst.hpp"

MaterialHandle MaterialManager::emplaceMaterial(BindlessTextureDSetComponent& dSetComponent, MaterialStructure material,
                                                BufferManager& bufferManager)
{
	auto cached = _materialCache.find(material);
	if (cached != _materialCache.end())
	{
		addMaterialRef(cached->second);
		return cached->second;
	}

	int slot;
	if (!_freeMaterialSlots.empty())
	{
		slot = _freeMaterialSlots.back();
		_freeMaterialSlots.pop_back();
		materials[slot] = material;
		_materialRefCounts[slot] = 1;
	}
	else
	{
		materials.push_back(material);
		_materialRefCounts.push_back(1);
		slot = static_cast<int>(materials.size() - 1);
	}
	_materialCache.emplace(material, MaterialHandle{slot});
	bufferManager.writeToBuffer(dSetComponent.materialBuffer, 0, slot, material);
	return MaterialHandle{slot};
}

void MaterialManager::addMaterialRef(MaterialHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(materials.size())) return;

	_materialRefCounts[handle.id]++;
}

bool MaterialManager::releaseMaterialRef(MaterialHandle handle)
{
	if (handle.id < 0 || handle.id >= static_cast<int>(materials.size())) return false;
	if (_materialRefCounts[handle.id] <= 0) return false;

	return --_materialRefCounts[handle.id] == 0;
}

void MaterialManager::freeMaterial(MaterialHandle handle, uint64_t frameNumber)
{
	if (!releaseMaterialRef(handle)) return;

	// Erased immediately so emplaceMaterial can't hand out a slot that is pending free.
	_materialCache.erase(materials[handle.id]);
	_pendingMaterialFrees.push_back({handle.id, frameNumber + MAX_FRAMES_IN_FLIGHT});
}

void MaterialManager::collectMaterialFrees(uint64_t frameNumber)
{
	for (auto it = _pendingMaterialFrees.begin(); it != _pendingMaterialFrees.end();)
	{
		if (it->retireFrame <= frameNumber)
		{
			_freeMaterialSlots.push_back(it->slot);
			it = _pendingMaterialFrees.erase(it);
		}
		else
			++it;
	}
}

const MaterialStructure& MaterialManager::getMaterial(MaterialHandle handle) const
{
	return materials[handle.id];
}

size_t MaterialManager::materialCount() const
{
	return materials.size();
}

size_t MaterialManager::freeMaterialSlotCount() const
{
	return _freeMaterialSlots.size();
}

size_t MaterialManager::pendingMaterialFreeCount() const
{
	return _pendingMaterialFrees.size();
}
