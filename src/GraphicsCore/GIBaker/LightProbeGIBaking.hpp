#pragma once
#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/ResourceStructures.hpp"
#include "GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "GraphicsCore/Resources/Managers/Bindings.hpp"
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/Passes/PassCommands.hpp"
#include "GraphicsCore/VulkanUtils.hpp"
#include "GraphicsCore/VulkanConst.hpp"

#include <vk_mem_alloc.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <array>
#include <vector>
#include <iostream>


using Orhescyon::GeneralManager;

// Constants

static constexpr uint32_t kCaptureSize = 128;
static constexpr vk::Format kCaptureFormat = vk::Format::eR16G16B16A16Sfloat;
static constexpr int kProbesPerSubmit = 32; // keeps each submit's GPU work under the Windows TDR budget

// Implementation-only structs

struct FaceDesc
{
	glm::vec3 forward, up;
};

struct BakeContext
{
	VulkanDevice* device;
	BufferManager* bufferManager;
	TextureManager* textureManager;
	ModelManager* modelManager;
	DescriptorManagerComponent* descriptorManagerComponent;
	PipelineManager* pipelineManager;
	GlobalDSetComponent* globalDSet;
	ModelDSetComponent* modelDSet;
	BindlessTextureDSetComponent* bindlessDSet;
	SkyboxComponent* skybox;
	DrawInfoComponent* drawInfo;
	LightProbeGridComponent* grid;
	DirectLightComponent* lightComponent;
	VmaAllocator allocator;
	bool hasSkybox;
};

struct TempImages
{
	TextureHandle captureHandle;
	vk::Image captureImage{}; // non-owning
	std::vector<vk::raii::ImageView> faceViews;
	VkImage depthRaw{};
	VmaAllocation depthAlloc{};
	std::vector<vk::raii::ImageView> depthViews; // one layer per face — faces render with no inter-dependencies
};

// Bakes a uniform grid of SH light probes.
// Slot 0 (skybox fallback) is untouched - baking starts at slot 1.
class LightProbeGIBaking
{
public:
    static void bakeAll(GeneralManager& gm);
    static void resetProbes(GeneralManager& gm);

private:
	static void bakeShadowMap(const BakeContext& ctx);
	static void recordProbe(vk::raii::CommandBuffer& cmd, const BakeContext& ctx, const TempImages& tmp,
	                        uint32_t regionBase, int slot, glm::vec3 pos, float radius);
};
