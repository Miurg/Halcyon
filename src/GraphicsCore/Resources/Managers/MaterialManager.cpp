#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/VulkanConst.hpp"

int MaterialManager::emplaceMaterial(BindlessTextureDSetComponent& dSetComponent, MaterialStructure material,
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
	_materialCache.emplace(material, slot);
	bufferManager.writeToBuffer(dSetComponent.materialBuffer, 0, slot, material);
	return slot;
}

void MaterialManager::addMaterialRef(int slot)
{
	if (slot < 0 || slot >= static_cast<int>(materials.size())) return;

	_materialRefCounts[slot]++;
}

bool MaterialManager::releaseMaterialRef(int slot)
{
	if (slot < 0 || slot >= static_cast<int>(materials.size())) return false;
	if (_materialRefCounts[slot] <= 0) return false;

	return --_materialRefCounts[slot] == 0;
}

void MaterialManager::freeMaterial(int slot, uint64_t frameNumber)
{
	if (!releaseMaterialRef(slot)) return;

	// Erased immediately so emplaceMaterial can't hand out a slot that is pending free.
	_materialCache.erase(materials[slot]);
	_pendingMaterialFrees.push_back({slot, frameNumber + MAX_FRAMES_IN_FLIGHT});
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

const MaterialStructure& MaterialManager::getMaterial(int slot) const
{
	return materials[slot];
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
