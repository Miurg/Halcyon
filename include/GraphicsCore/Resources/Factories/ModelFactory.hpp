#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API ModelFactory
{
public:
	static Orhescyon::Entity loadModel(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
	                        BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
	                        GeneralManager& gm, TextureManager& textureManager, ModelManager& modelManager,
	                        MaterialManager& materialManager, VulkanDevice& vulkanDevice, VmaAllocator allocator);
	static bool unloadModel(Orhescyon::Entity modelRootEntity, GeneralManager& gm, ModelManager& modelManager,
	                        TextureManager& textureManager, MaterialManager& materialManager);
};