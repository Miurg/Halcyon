#pragma once

#include "HalcyonExport.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <Orhescyon/Entitys/EntityManager.hpp>

struct HALCYON_API GraphicsSettingsComponent
{
	//bool enableFrustumCulling = true;
	//bool enableShadowCulling = true;
	bool enableGtao = true;
	bool appliedGtao = true;
	bool enableGodRays = true;
	bool enableAutoExposure = true;
	bool appliedAutoExposure = true;
	bool enableFxaa = true;
	bool enableBloom = true;
	bool enableVignette = true;
	float bloomThreshold = 1.0f;
	float bloomKnee = 0.3f;
	float bloomIntensity = 0.08f;
	// Color grading (tone mapping)
	int toneMapper = 0;
	int gradingSpace = 2; // 0 = Display (post-tonemap), 1 = Linear (HDR), 2 = Log (pre-tonemap)
	float colorExposure = 0.0f; // EV compensation
	float contrast = 1.0f;
	float saturation = 1.0f;
	float temperature = 0.0f;
	float tint = 0.0f;
	//bool enableToneMapping = true;
	//bool enableGammaCorrection = true;
	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e4;
	vk::SampleCountFlagBits appliedMsaaSamples =
	    vk::SampleCountFlagBits::e1; // TODO: get rid of this and just recreate pipelines when msaa changes
	Orhescyon::Entity selectedEntity = Orhescyon::Entity::invalid();
	bool aabbAlwaysOnTop = true;
	GraphicsSettingsComponent() = default;
};
