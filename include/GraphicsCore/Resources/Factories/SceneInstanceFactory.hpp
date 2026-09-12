#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"
#include "GraphicsCore/Resources/Managers/SceneTemplateManager.hpp"
#include <cstdint>

class HALCYON_API SceneInstanceFactory
{
public:
	static SceneTemplateHandle loadSceneInstance(
	    const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
	    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
	    TextureManager& textureManager, RenderAssetManager& renderAssetManager,
	    SceneTemplateManager& sceneTemplateManager, MaterialManager& materialManager, VulkanDevice& vulkanDevice,
	    VmaAllocator allocator, int sceneIndex = -1);
	static bool unloadSceneInstance(SceneTemplateHandle sceneTemplateHandle, RenderAssetHandle renderAssetHandle,
	                                SceneTemplateManager& sceneTemplateManager,
	                                RenderAssetManager& renderAssetManager, TextureManager& textureManager,
	                                MaterialManager& materialManager, uint32_t frameNumber);
};
