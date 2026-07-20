#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include <functional>

class BufferManager;

// Owns material slots and mirrors them into the bindless material buffer.
class HALCYON_API MaterialManager
{
public:
	int emplaceMaterial(BindlessTextureDSetComponent& dSetComponent, MaterialStructure material,
	                    BufferManager& bufferManager);
	void freeMaterial(int slot, uint64_t frameNumber);
	void collectMaterialFrees(uint64_t frameNumber);
	const MaterialStructure& getMaterial(int slot) const;
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
	std::vector<int> _materialRefCounts;

	// Content key for deduplication; padding fields are constant and skipped.
	struct MaterialKeyHash
	{
		size_t operator()(const MaterialStructure& material) const noexcept
		{
			size_t seed = 0;
			auto combine = [&seed](size_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); };
			combine(material.textureIndex);
			combine(material.normalMapIndex);
			combine(material.metallicRoughnessIndex);
			combine(material.emissiveIndex);
			combine(std::hash<float>{}(material.alphaCutoff));
			combine(material.alphaMode);
			combine(std::hash<float>{}(material.emissiveStrength));
			combine(material.doubleSided);
			for (int i = 0; i < 4; ++i) combine(std::hash<float>{}(material.baseColorFactor[i]));
			for (int i = 0; i < 3; ++i) combine(std::hash<float>{}(material.emissiveFactor[i]));
			combine(std::hash<float>{}(material.roughnessFactor));
			combine(std::hash<float>{}(material.metallicFactor));
			return seed;
		}
	};
	struct MaterialKeyEqual
	{
		bool operator()(const MaterialStructure& a, const MaterialStructure& b) const noexcept
		{
			return a.textureIndex == b.textureIndex && a.normalMapIndex == b.normalMapIndex &&
			       a.metallicRoughnessIndex == b.metallicRoughnessIndex && a.emissiveIndex == b.emissiveIndex &&
			       a.alphaCutoff == b.alphaCutoff && a.alphaMode == b.alphaMode &&
			       a.emissiveStrength == b.emissiveStrength && a.doubleSided == b.doubleSided &&
			       a.baseColorFactor == b.baseColorFactor && a.emissiveFactor == b.emissiveFactor &&
			       a.roughnessFactor == b.roughnessFactor && a.metallicFactor == b.metallicFactor;
		}
	};
	std::unordered_map<MaterialStructure, int, MaterialKeyHash, MaterialKeyEqual> _materialCache;
};
