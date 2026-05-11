#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <vector>
#include "FrameBeginSystem.hpp"
#include "BufferUpdateSystem.hpp"
#include "../Components/NameComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/PointLightComponent.hpp"
#include "../Components/SsaoSettingsComponent.hpp"
#include "../Components/GraphicsSettingsComponent.hpp"
#include "../Components/LightProbeGridComponent.hpp"
#include "../../Game/Components/ControlComponent.hpp"
#include "../Components/DeltaTimeComponent.hpp"

using Orhescyon::GeneralManager;
class ImGuiSystem : public Orhescyon::SystemCore<ImGuiSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(BufferUpdateSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(NameComponent),     typeid(RelationshipComponent), typeid(CameraComponent),
		        typeid(ControlComponent),  typeid(GraphicsSettingsComponent),
		        typeid(DeltaTimeComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(GlobalTransformComponent), typeid(LocalTransformComponent), typeid(DirectLightComponent),
		        typeid(PointLightComponent),      typeid(SsaoSettingsComponent),   typeid(LightProbeGridComponent)};
	}

	uint32_t frameCount = 0;
	float time = 0;
	uint32_t fps = 0;
	Orhescyon::Entity selectedEntity = static_cast<Orhescyon::Entity>(-1);

	std::vector<float> frameTimes;
	float avgFrameTime = 0.0f;
	float onePercentLowFrameTime = 0.0f;
	
	bool autoShaderReload = false;

private:
	void drawEntityNode(Orhescyon::Entity entity, GeneralManager& gm);
};