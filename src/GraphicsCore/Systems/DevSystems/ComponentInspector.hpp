#pragma once

#include <functional>
#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/AutoExposureSettingsComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "PhysicsCore/PhysContexts.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/Managers/PhysManager.hpp"
#include "PhysicsCore/JoltGlm.hpp"

using Orhescyon::Entity;
using Orhescyon::GeneralManager;

struct ComponentInspector
{
	const char* name;
	std::function<void(GeneralManager&, Entity)> inspect;
};

class InspectorRegistry
{
public:
	template <typename Component, typename InspectFn>
	void add(const char* name, InspectFn inspectFn)
	{
		_inspectors.push_back({name,
		                       [name, inspectFn](GeneralManager& gm, Entity entity)
		                       {
			                       if (auto* component = gm.getComponent<Component>(entity))
				                       if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen))
					                       inspectFn(gm, entity, *component);
		                       }});
	}

	void inspectAll(GeneralManager& gm, Entity entity) const
	{
		for (const auto& inspector : _inspectors) inspector.inspect(gm, entity);
	}

private:
	std::vector<ComponentInspector> _inspectors;
};

inline void pushTransformToBody(GeneralManager& gm, Entity entity, glm::vec3 worldPosition, glm::quat worldRotation)
{
	auto* physBody = gm.getComponent<PhysBodyComponent>(entity);
	if (!physBody) return;

	auto& bodyInterface = gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()
	                          ->physManager->physicsSystem->GetBodyInterface();
	bodyInterface.SetPositionAndRotation(physBody->bodyID,
	                                     JPH::RVec3(worldPosition.x, worldPosition.y, worldPosition.z),
	                                     toJolt(worldRotation), JPH::EActivation::Activate);
}

inline void inspectGlobalTransform(GeneralManager& gm, Entity entity, GlobalTransformComponent& transform)
{
	glm::vec3 position = transform.getGlobalPosition();
	glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(transform.getGlobalRotation()));
	glm::vec3 scale = transform.getGlobalScale();

	bool changed = false;
	if (ImGui::DragFloat3("G Position", glm::value_ptr(position),     0.1f)) changed = true;
	if (ImGui::DragFloat3("G Rotation", glm::value_ptr(eulerDegrees), 0.5f)) changed = true;
	if (ImGui::DragFloat3("G Scale",    glm::value_ptr(scale),        0.1f)) changed = true;
	if (!changed) return;

	glm::quat rotation = glm::quat(glm::radians(eulerDegrees));
	transform.setGlobalPosition(position);
	transform.setGlobalRotation(rotation);
	transform.setGlobalScale(scale);

	pushTransformToBody(gm, entity, position, rotation);
}

inline void inspectLocalTransform(GeneralManager& gm, Entity entity, LocalTransformComponent& transform)
{
	glm::vec3 position = transform.getLocalPosition();
	glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(transform.getLocalRotation()));
	glm::vec3 scale = transform.getLocalScale();

	bool changed = false;
	if (ImGui::DragFloat3("L Position", glm::value_ptr(position),     0.1f)) changed = true;
	if (ImGui::DragFloat3("L Rotation", glm::value_ptr(eulerDegrees), 0.5f)) changed = true;
	if (ImGui::DragFloat3("L Scale",    glm::value_ptr(scale),        0.1f)) changed = true;
	if (!changed) return;

	glm::quat rotation = glm::quat(glm::radians(eulerDegrees));
	transform.setLocalPosition(position);
	transform.setLocalRotation(rotation);
	transform.setLocalScale(scale);

	glm::vec3 worldPosition = position;
	glm::quat worldRotation = rotation;
	if (auto* relationship = gm.getComponent<RelationshipComponent>(entity))
	{
		if (relationship->parent != NULL_ENTITY)
		{
			auto* parent = gm.getComponent<GlobalTransformComponent>(relationship->parent);
			worldPosition = parent->getGlobalPosition() +
			                parent->getGlobalRotation() * (parent->getGlobalScale() * position);
			worldRotation = parent->getGlobalRotation() * rotation;
		}
	}
	pushTransformToBody(gm, entity, worldPosition, worldRotation);
}

inline void inspectDirectLight(GeneralManager&, Entity, DirectLightComponent& light)
{
	ImGui::SeparatorText("Sun Lighting");
	ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
	ImGui::DragFloat("Intensity", &light.color.w, 0.01f, 0.0f, 1000.0f);

	ImGui::SeparatorText("Ambient Lighting");
	ImGui::ColorEdit3("Ambient Color", glm::value_ptr(light.ambient));
	ImGui::DragFloat("Ambient Intensity", &light.ambient.w, 0.001f, 0.0f, 10.0f);

	ImGui::SeparatorText("Shadow Settings");
	ImGui::DragFloat("Shadow Distance", &light.shadowDistance, 1.0f);
	ImGui::DragFloat("Shadow Caster Range", &light.shadowCasterRange, 10.0f);
}

inline void inspectCamera(GeneralManager&, Entity, CameraComponent& camera)
{
	ImGui::DragFloat("FOV", &camera.fov, 0.1f, 1.0f, 179.0f);
	ImGui::DragFloat("Near Plane", &camera.zNear, 0.01f, 0.001f, 10.0f);
	ImGui::DragFloat("Far Plane", &camera.zFar, 1.0f, 10.0f, 10000.0f);
	ImGui::DragFloat("Ortho size", &camera.orthoSize, 0.1f, 0.1f, 100.0f);
}

inline void inspectGtaoSettings(GeneralManager&, Entity, GtaoSettingsComponent& gtao)
{
	ImGui::SliderInt("Kernel Size", &gtao.kernelSize, 1, 32);
	ImGui::DragFloat("Radius", &gtao.radius, 0.1f, 0.1f, 50.0f);
	ImGui::DragFloat("Bias", &gtao.bias, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Power", &gtao.power, 0.1f, 0.1f, 20.0f);
	ImGui::SliderInt("Num Directions", &gtao.numDirections, 1, 16);
	ImGui::DragFloat("Max Screen Radius", &gtao.maxScreenRadius, 0.01f, 0.01f, 1.0f);
	ImGui::DragFloat("Fade start", &gtao.fadeStart, 10.0f, 1000.0f);
	ImGui::DragFloat("Fade end", &gtao.fadeEnd, 10.0f, 1000.0f);
	ImGui::SliderFloat("Mip Bias", &gtao.mipBias, -4.0f, 4.0f);
	ImGui::SliderFloat("Blur Depth Tolerance", &gtao.blurDepthTolerance, 0.001f, 1.0f);
	ImGui::SliderFloat("Pyramid Edge Range", &gtao.pyramidEdgeRange, 0.0f, 2.0f);
	ImGui::SliderFloat("Multi-bounce Albedo", &gtao.multiBounceAlbedo, 0.0f, 1.0f);
	ImGui::SliderFloat("Thickness Scale", &gtao.thicknessScale, 0.1f, 5.0f);
}

inline int msaaSamplesToIndex(vk::SampleCountFlagBits samples)
{
	switch (samples)
	{
	case vk::SampleCountFlagBits::e1:  return 0;
	case vk::SampleCountFlagBits::e2:  return 1;
	case vk::SampleCountFlagBits::e4:  return 2;
	case vk::SampleCountFlagBits::e8:  return 3;
	case vk::SampleCountFlagBits::e16: return 4;
	case vk::SampleCountFlagBits::e32: return 5;
	case vk::SampleCountFlagBits::e64: return 6;
	default: return 0;
	}
}

inline vk::SampleCountFlagBits msaaIndexToSamples(int index)
{
	switch (index)
	{
	case 0:  return vk::SampleCountFlagBits::e1;
	case 1:  return vk::SampleCountFlagBits::e2;
	case 2:  return vk::SampleCountFlagBits::e4;
	case 3:  return vk::SampleCountFlagBits::e8;
	case 4:  return vk::SampleCountFlagBits::e16;
	case 5:  return vk::SampleCountFlagBits::e32;
	case 6:  return vk::SampleCountFlagBits::e64;
	default: return vk::SampleCountFlagBits::e1;
	}
}

inline void drawMsaaSelector(GeneralManager& gm, GraphicsSettingsComponent& settings)
{
	auto* vulkanDeviceComponent = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>();
	if (!vulkanDeviceComponent || !vulkanDeviceComponent->vulkanDeviceInstance) return;

	vk::SampleCountFlagBits maxSamples = vulkanDeviceComponent->vulkanDeviceInstance->maxMsaaSamples;
	int maxIndex = msaaSamplesToIndex(maxSamples);

	const char* items[] = {"Off (1x)", "2x", "4x", "8x", "16x", "32x", "64x"};
	int current = msaaSamplesToIndex(settings.msaaSamples);

	if (current > maxIndex)
	{
		current = maxIndex;
		settings.msaaSamples = msaaIndexToSamples(maxIndex);
	}

	if (ImGui::Combo("MSAA", &current, items, maxIndex + 1)) settings.msaaSamples = msaaIndexToSamples(current);
}

inline void inspectGraphicsSettings(GeneralManager& gm, Entity, GraphicsSettingsComponent& settings)
{
	ImGui::Checkbox("Enable GTAO", &settings.enableGtao);
	ImGui::Checkbox("Enable FXAA", &settings.enableFxaa);
	ImGui::Checkbox("Enable Bloom", &settings.enableBloom);
	ImGui::Checkbox("Enable Vignette", &settings.enableVignette);
	ImGui::Checkbox("Enable Auto Exposure", &settings.enableAutoExposure);
	if (settings.enableBloom)
	{
		ImGui::DragFloat("Bloom Threshold", &settings.bloomThreshold, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Bloom Knee", &settings.bloomKnee, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Bloom Intensity", &settings.bloomIntensity, 0.001f, 0.0f, 1.0f);
	}

	ImGui::SeparatorText("Color Grading");
	const char* gradingSpaces[] = {"Display (post-tonemap)", "Linear (HDR)", "Log"};
	ImGui::Combo("Grading Space", &settings.gradingSpace, gradingSpaces, IM_ARRAYSIZE(gradingSpaces));
	ImGui::SliderFloat("Exposure", &settings.colorExposure, -4.0f, 4.0f);
	ImGui::SliderFloat("Contrast", &settings.contrast, 0.0f, 2.0f);
	ImGui::SliderFloat("Saturation", &settings.saturation, 0.0f, 2.0f);
	ImGui::SliderFloat("Temperature", &settings.temperature, -1.0f, 1.0f);
	ImGui::SliderFloat("Tint", &settings.tint, -1.0f, 1.0f);
	if (ImGui::Button("Reset Color Grading"))
	{
		settings.gradingSpace = 2;
		settings.colorExposure = 0.0f;
		settings.contrast = 1.0f;
		settings.saturation = 1.0f;
		settings.temperature = 0.0f;
		settings.tint = 0.0f;
	}

	drawMsaaSelector(gm, settings);
}

inline void inspectAutoExposure(GeneralManager& gm, Entity, AutoExposureSettingsComponent& autoExposure)
{
	auto* graphicsSettings = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
	if (graphicsSettings && !graphicsSettings->enableAutoExposure)
	{
		ImGui::TextDisabled("Auto Exposure is disabled in Graphics Settings.");
		return;
	}

	ImGui::DragFloat("Brighten Speed", &autoExposure.tauUp, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Darken Speed", &autoExposure.tauDown, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Darkest Exposure", &autoExposure.minEV, 0.1f, -20.0f, 0.0f);
	ImGui::DragFloat("Brightest Exposure", &autoExposure.maxEV, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("Target Brightness", &autoExposure.targetLuminance, 0.01f, 0.0f, 2.0f);
	ImGui::SliderFloat("Shadow Cutoff", &autoExposure.lowPercent, 0.0f, 1.0f);
	ImGui::SliderFloat("Highlight Cutoff", &autoExposure.highPercent, 0.0f, 1.0f);
}

inline void inspectPointLight(GeneralManager&, Entity, PointLightComponent& light)
{
	ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
	ImGui::DragFloat("Intensity", &light.intensity, 1.0f, 0.0f, 1000.0f);
	ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.1f, 100.0f);
	ImGui::DragFloat3("Direction", glm::value_ptr(light.direction), 0.1f);
	ImGui::DragFloat("Inner Cone Angle", &light.innerConeAngle, 0.1f, 0.0f, 180.0f);
	ImGui::DragFloat("Outer Cone Angle", &light.outerConeAngle, 0.1f, 0.0f, 180.0f);
	const char* typeItems[] = {"Point Light", "Spot Light"};
	int type = light.type;
	if (ImGui::Combo("Type", &type, typeItems, IM_ARRAYSIZE(typeItems))) light.type = type;
}

inline void inspectLightProbeGrid(GeneralManager&, Entity, LightProbeGridComponent& grid)
{
	ImGui::DragFloat3("Origin", &grid.origin.x, 0.1f);

	int count[3] = {grid.count.x, grid.count.y, grid.count.z};
	if (ImGui::DragInt3("Count", count, 1, 1, 64))
	{
		grid.count.x = glm::max(count[0], 1);
		grid.count.y = glm::max(count[1], 1);
		grid.count.z = glm::max(count[2], 1);
	}

	ImGui::DragFloat("Spacing", &grid.spacing, 0.05f, 0.01f, 10.0f);
	ImGui::DragFloat("Capture Range", &grid.captureRange, 1.0f, 1.0f, 1000.0f);

	ImGui::SeparatorText("GI Ambient");
	ImGui::ColorEdit3("GI Ambient Color", glm::value_ptr(grid.giAmbientColor));
	ImGui::DragFloat("GI Ambient Intensity", &grid.giAmbientIntensity, 0.001f, 0.0f, 10.0f);
	ImGui::DragFloat("GI Bounce Multiplier", &grid.giBounceMultiplier, 0.01f, 0.0f, 10.0f);

	if (ImGui::Button("Bake Global Illumination", ImVec2(200, 20))) grid.needBake = true;

	ImGui::Checkbox("Visualize Probes", &grid.debugVisualize);
	if (grid.debugVisualize) ImGui::SliderFloat("Probe Scale", &grid.debugScale, 0.05f, 2.0f);
}

inline void inspectReflectionProbe(GeneralManager&, Entity, ReflectionProbeComponent& probe)
{
	ImGui::DragFloat3("Probe Origin", &probe.origin.x, 0.1f);
	ImGui::DragFloat3("Influence Half-Extent", &probe.halfExtent.x, 0.1f, 0.1f, 1000.0f);
	ImGui::DragFloat("Capture Range##refl", &probe.captureRange, 1.0f, 1.0f, 1000.0f);

	ImGui::SeparatorText("Capture GI");
	ImGui::ColorEdit3("GI Ambient Color##refl", glm::value_ptr(probe.giAmbientColor));
	ImGui::DragFloat("GI Ambient Intensity##refl", &probe.giAmbientIntensity, 0.001f, 0.0f, 10.0f);
	ImGui::DragFloat("GI Bounce Multiplier##refl", &probe.giBounceMultiplier, 0.01f, 0.0f, 10.0f);

	if (ImGui::Button("Bake Reflection Probe", ImVec2(200, 20))) probe.needBake = true;
}

inline InspectorRegistry buildInspectorRegistry()
{
	InspectorRegistry registry;
	registry.add<GlobalTransformComponent>("Global Transform", &inspectGlobalTransform);
	registry.add<LocalTransformComponent>("Local Transform", &inspectLocalTransform);
	registry.add<DirectLightComponent>("Light Component", &inspectDirectLight);
	registry.add<CameraComponent>("Camera Component", &inspectCamera);
	registry.add<GtaoSettingsComponent>("GTAO Settings", &inspectGtaoSettings);
	registry.add<GraphicsSettingsComponent>("Graphics Settings", &inspectGraphicsSettings);
	registry.add<AutoExposureSettingsComponent>("Auto Exposure Settings", &inspectAutoExposure);
	registry.add<PointLightComponent>("Point Light Component", &inspectPointLight);
	registry.add<LightProbeGridComponent>("Global Illumination Component", &inspectLightProbeGrid);
	registry.add<ReflectionProbeComponent>("Reflection Probe Component", &inspectReflectionProbe);
	return registry;
}
