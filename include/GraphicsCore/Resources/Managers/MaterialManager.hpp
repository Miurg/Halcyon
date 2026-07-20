#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include <vector>
#include <cstdint>

class BufferManager;

// Owns material slots and mirrors them into the bindless material buffer.
class HALCYON_API MaterialManager
{
public:
	int emplaceMaterial(BindlessTextureDSetComponent& dSetComponent, MaterialStructure material,
	                    BufferManager& bufferManager);
	void freeMaterial(int slot, uint64_t frameNumber);
	void collectMaterialFrees(uint64_t frameNumber);
	MaterialStructure& getMaterial(int slot);
	size_t freeMaterialSlotCount() const;
	size_t pendingMaterialFreeCount() const;

	std::vector<MaterialStructure> materials;

private:
	struct PendingMaterialFree
	{
		int slot;
		uint64_t retireFrame;
	};
	std::vector<PendingMaterialFree> _pendingMaterialFrees;
	std::vector<int> _freeMaterialSlots;
};
