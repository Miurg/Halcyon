#pragma once

#include <vulkan/vulkan_raii.hpp>

struct GraphicsSettingsComponent
{
	//bool enableFrustumCulling = true;
	//bool enableShadowCulling = true;
	bool enableSsao = true;
	bool enableFxaa = true;
	//bool enableBloom = true;
	//bool enableToneMapping = true;
	//bool enableGammaCorrection = true;
	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e4;
	vk::SampleCountFlagBits appliedMsaaSamples =
	    vk::SampleCountFlagBits::e1; // TODO: get rid of this and just recreate pipelines when msaa changes
	GraphicsSettingsComponent() = default;
};