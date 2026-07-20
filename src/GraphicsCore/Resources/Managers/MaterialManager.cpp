#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/VulkanConst.hpp"

int MaterialManager::emplaceMaterial(BindlessTextureDSetComponent& dSetComponent, MaterialStructure material,
                                     BufferManager& bufferManager)
{
	int slot;
	if (!_freeMaterialSlots.empty())
	{
		slot = _freeMaterialSlots.back();
		_freeMaterialSlots.pop_back();
		materials[slot] = material;
	}
	else
	{
		materials.push_back(material);
		slot = static_cast<int>(materials.size() - 1);
	}
	bufferManager.writeToBuffer(dSetComponent.materialBuffer, 0, slot, material);
	return slot;
}

void MaterialManager::freeMaterial(int slot, uint64_t frameNumber)
{
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

MaterialStructure& MaterialManager::getMaterial(int slot)
{
	return materials[slot];
}

size_t MaterialManager::freeMaterialSlotCount() const
{
	return _freeMaterialSlots.size();
}

size_t MaterialManager::pendingMaterialFreeCount() const
{
	return _pendingMaterialFrees.size();
}
