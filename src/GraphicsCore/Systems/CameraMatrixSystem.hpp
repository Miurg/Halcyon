#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameBeginSystem.hpp"
#include "BufferUpdateSystem.hpp"
#include "../Components/CameraComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/DirectLightComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"

using Orhescyon::GeneralManager;
class CameraMatrixSystem : public Orhescyon::SystemCore<CameraMatrixSystem>
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
		return {typeid(CameraComponent), typeid(GlobalTransformComponent), typeid(DirectLightComponent),
		        typeid(CurrentFrameComponent)};
	}
};