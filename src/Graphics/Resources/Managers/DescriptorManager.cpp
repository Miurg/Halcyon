#include "DescriptorManager.hpp"
#include "BufferManager.hpp"
#include "SharedBindings.hpp"

DescriptorManager::DescriptorManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice)
{
	std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000 * MAX_FRAMES_IN_FLIGHT),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
	                                           1000 * MAX_FRAMES_IN_FLIGHT + MAX_BINDLESS_TEXTURES),
	                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000 * MAX_FRAMES_IN_FLIGHT)};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.flags =
	    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
	poolInfo.maxSets = 1000 * 3 * MAX_FRAMES_IN_FLIGHT + 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();

	descriptorPool = vk::raii::DescriptorPool(vulkanDevice.device, poolInfo);

	std::array imguiPoolSize{vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
	                         vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)};

	vk::DescriptorPoolCreateInfo imguiPoolInfo;
	imguiPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	imguiPoolInfo.maxSets = 1000 * static_cast<uint32_t>(imguiPoolSize.size());
	imguiPoolInfo.poolSizeCount = static_cast<uint32_t>(imguiPoolSize.size());
	imguiPoolInfo.pPoolSizes = imguiPoolSize.data();

	imguiPool = vk::raii::DescriptorPool(vulkanDevice.device, imguiPoolInfo);

	//===Global (Set 0): camera + sun===
	vk::DescriptorSetLayoutBinding cameraBinding(BINDING_CAMERA, vk::DescriptorType::eStorageBuffer, 1,
	                                             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute |
	                                                 vk::ShaderStageFlagBits::eFragment,
	                                             nullptr);
	vk::DescriptorSetLayoutBinding sunBinding(BINDING_SUN, vk::DescriptorType::eStorageBuffer, 1,
	                                          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
	                                          nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 2> globalBindings = {cameraBinding, sunBinding};

	vk::DescriptorSetLayoutCreateInfo globalInfo({}, static_cast<uint32_t>(globalBindings.size()),
	                                             globalBindings.data());
	globalSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, globalInfo);

	//===Textures (Set 2): textureArray (bindless) + shadowMap===
	vk::DescriptorSetLayoutBinding textureBinding(
	    BINDING_TEXTURE_ARRAY, vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES,
	                                              vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute,
	                                              nullptr);
	vk::DescriptorSetLayoutBinding shadowBinding(BINDING_SHADOW_MAP, vk::DescriptorType::eCombinedImageSampler, 1,
	                                             vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding materialBinding(BINDING_MATERIAL_BUFFER, vk::DescriptorType::eStorageBuffer, 1,
	                                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
	                                               nullptr);
	vk::DescriptorSetLayoutBinding cubemapSamplerBinding(BINDING_CUBEMAP_SAMPLER,
	                                                     vk::DescriptorType::eCombinedImageSampler, 1,
	                                                     vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding cubemapStorageBinding(BINDING_CUBEMAP_STORAGE, vk::DescriptorType::eStorageImage, 1,
	                                                     vk::ShaderStageFlagBits::eCompute, nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 5> textureBindings = {textureBinding, shadowBinding, materialBinding,
	                                                                 cubemapSamplerBinding, cubemapStorageBinding};

	std::array<vk::DescriptorBindingFlags, 5> textureBindingFlags = {
	    vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
	    vk::DescriptorBindingFlags{}, // shadowMap: no special flags
	    vk::DescriptorBindingFlags{}, // materialBinding
	    vk::DescriptorBindingFlags{}, // cubemapSampler
	    vk::DescriptorBindingFlags{}  // cubemapStorage
	};
	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(textureBindingFlags.size());
	bindingFlagsInfo.pBindingFlags = textureBindingFlags.data();

	vk::DescriptorSetLayoutCreateInfo textureInfo;
	textureInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
	textureInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
	textureInfo.pBindings = textureBindings.data();
	textureInfo.pNext = &bindingFlagsInfo;
	textureSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, textureInfo);

	//===Model (Set 1): primitives + transform + indirectDraw + visibleIndices===
	vk::DescriptorSetLayoutBinding primitivesBinding(
	    BINDING_MODEL_DATA, vk::DescriptorType::eStorageBuffer, 1,
	    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute,
	    nullptr);
	vk::DescriptorSetLayoutBinding transformBinding(BINDING_TRANSFORM_DATA, vk::DescriptorType::eStorageBuffer, 1,
	                                                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute,
	                                                nullptr);
	vk::DescriptorSetLayoutBinding commandBinding(BINDING_INDIRECT_DRAW_DATA, vk::DescriptorType::eStorageBuffer, 1,
	                                              vk::ShaderStageFlagBits::eCompute, nullptr);
	vk::DescriptorSetLayoutBinding indicesBinding(BINDING_VISIBLE_INDICES, vk::DescriptorType::eStorageBuffer, 1,
	                                              vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex,
	                                              nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 4> modelBindings = {primitivesBinding, transformBinding, commandBinding,
	                                                               indicesBinding};
	vk::DescriptorSetLayoutCreateInfo modelInfo({}, static_cast<uint32_t>(modelBindings.size()), modelBindings.data());
	modelSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, modelInfo);

	//===SCREEN SPACE (shared: FXAA, SSAO, SSAOBlur)===
	std::array<vk::DescriptorSetLayoutBinding, 3> screenSpaceBindings{};
	for (uint32_t i = 0; i < 3; i++)
	{
		screenSpaceBindings[i].binding = i;
		screenSpaceBindings[i].descriptorCount = 1;
		screenSpaceBindings[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		screenSpaceBindings[i].pImmutableSamplers = nullptr;
		screenSpaceBindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
	}

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(screenSpaceBindings.size());
	layoutInfo.pBindings = screenSpaceBindings.data();

	screenSpaceSetLayout = vk::raii::DescriptorSetLayout(vulkanDevice.device, layoutInfo);
}

DescriptorManager::~DescriptorManager()
{
	descriptorPool.reset();
	globalSetLayout.~DescriptorSetLayout();
	textureSetLayout.~DescriptorSetLayout();
	modelSetLayout.~DescriptorSetLayout();
	screenSpaceSetLayout.~DescriptorSetLayout();
	imguiPool.reset();
}

DSetHandle DescriptorManager::allocateBindlessTextureDSet()
{
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, *textureSetLayout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);
	std::vector descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);
	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

void DescriptorManager::updateBindlessTextureSet(vk::ImageView textureImageView, vk::Sampler textureSampler,
                                                 BindlessTextureDSetComponent& dSetComponent, int textureNumber)
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.sampler = textureSampler;
	imageInfo.imageView = textureImageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrite.dstBinding = BINDING_TEXTURE_ARRAY;
	descriptorWrite.dstArrayElement = textureNumber;
	descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	dSetComponent.bindlessTextureBuffer = TextureHandle{textureNumber};

	vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
}

void DescriptorManager::updateCubemapDescriptors(BindlessTextureDSetComponent& dSetComponent,
                                                 vk::ImageView cubemapImageView, vk::Sampler cubemapSampler,
                                                 vk::ImageView storageImageView)
{
	vk::DescriptorImageInfo samplerInfo;
	samplerInfo.sampler = cubemapSampler;
	samplerInfo.imageView = cubemapImageView;
	samplerInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo storageInfo;
	storageInfo.sampler = nullptr;
	storageInfo.imageView = storageImageView;
	storageInfo.imageLayout = vk::ImageLayout::eGeneral;

	std::array<vk::WriteDescriptorSet, 2> descriptorWrites;

	descriptorWrites[0].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[0].dstBinding = BINDING_CUBEMAP_SAMPLER;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &samplerInfo;

	descriptorWrites[1].dstSet = descriptorSets[dSetComponent.bindlessTextureSet.id][0];
	descriptorWrites[1].dstBinding = BINDING_CUBEMAP_STORAGE;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageImage;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &storageInfo;

	vulkanDevice.device.updateDescriptorSets(descriptorWrites, {});
}

void DescriptorManager::updateSingleTextureDSet(DSetHandle dIndex, int binding, vk::ImageView imageView,
                                                vk::Sampler sampler)
{
	for (size_t i = 0; i < descriptorSets[dIndex.id].size(); i++)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = descriptorSets[dIndex.id][i];
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vulkanDevice.device.updateDescriptorSets(descriptorWrite, {});
	}
}

DSetHandle DescriptorManager::allocateStorageBufferDSets(uint32_t count, vk::DescriptorSetLayout layout)
{
	std::vector<vk::DescriptorSetLayout> layouts(count, layout);
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = layouts.data();

	auto allocatedSets = (*vulkanDevice.device).allocateDescriptorSets(allocInfo);
	descriptorSets.push_back(allocatedSets);
	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}

void DescriptorManager::updateStorageBufferDescriptors(BufferManager& bManager, BufferHandle bNumber, DSetHandle dSet,
                                                       uint32_t binding)
{
	const size_t count = descriptorSets[dSet.id].size();
	assert(bManager.buffers[bNumber.id].buffer.size() >= count);

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(count);

	std::vector<vk::DescriptorBufferInfo> bufferInfos(count);

	for (size_t i = 0; i < count; ++i)
	{
		bufferInfos[i].buffer = bManager.buffers[bNumber.id].buffer[i];
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet write{};
		write.dstSet = descriptorSets[dSet.id][i];
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorType = vk::DescriptorType::eStorageBuffer;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfos[i];

		writes.push_back(write);
	}

	(*vulkanDevice.device).updateDescriptorSets(writes, {});
}

DSetHandle DescriptorManager::allocateFxaaDescriptorSet(vk::DescriptorSetLayout layout)
{
	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, layout);
	auto allocatedSets = vulkanDevice.device.allocateDescriptorSets(allocInfo);

	std::vector<vk::DescriptorSet> descriptors = {allocatedSets[0].release()};
	descriptorSets.push_back(descriptors);

	return DSetHandle{static_cast<int>(descriptorSets.size() - 1)};
}
