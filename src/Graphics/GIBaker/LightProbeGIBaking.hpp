#pragma once
#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../Components/LightProbeGridComponent.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/TextureManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Components/SkyboxComponent.hpp"
#include "../Components/VMAllocatorComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Components/BindlessTextureDSetComponent.hpp"
#include "../Resources/ResourceStructures.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/TextureManager.hpp"
#include "../Resources/Managers/DescriptorManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Passes/RenderPasses.hpp"
#include "../VulkanUtils.hpp"
#include "../VulkanConst.hpp"

#include <vk_mem_alloc.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <array>
#include <iostream>


using Orhescyon::GeneralManager;

// Constants

static constexpr uint32_t kCaptureSize = 128;
static constexpr vk::Format kCaptureFormat = vk::Format::eR16G16B16A16Sfloat;

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
	VkImage depthRaw{};
	VmaAllocation depthAlloc{};
	vk::raii::ImageView depthView{nullptr};
};

// Bakes a uniform grid of SH light probes.
// Slot 0 (skybox fallback) is untouched - baking starts at slot 1.
class LightProbeGIBaking
{
public:
    static void bakeAll(GeneralManager& gm);

private:
	static void bakeShadowMap(const BakeContext& ctx);
	static void bakeProbe(const BakeContext& ctx, const TempImages& tmp, int slot, glm::vec3 pos, float radius,
	                      vk::ImageView skyboxView, vk::Sampler skyboxSampler);
};
