#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <memory>
#include <vector>
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/GodRaysSettingsComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"

using Orhescyon::GeneralManager;
class FileInspector;
class HALCYON_API ImGuiSystem : public Orhescyon::SystemCore<ImGuiSystem>
{
public:
	ImGuiSystem();
	~ImGuiSystem() override;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

	uint32_t frameCount = 0;
	float time = 0;
	uint32_t fps = 0;
	Orhescyon::Entity selectedEntity = Orhescyon::Entity::invalid();

	std::vector<float> frameTimes;
	float avgFrameTime = 0.0f;
	float onePercentLowFrameTime = 0.0f;

private:
	std::unique_ptr<FileInspector> fileInspector;
	bool autoShaderReload = false;
	void drawEntityNode(Orhescyon::Entity entity, GeneralManager& gm);
};