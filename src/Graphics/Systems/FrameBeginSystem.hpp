#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameEndSystem.hpp"
#include "../../Platform/Systems/DeltaTimeSystem.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/FrameImageComponent.hpp"

using Orhescyon::GeneralManager;
class FrameBeginSystem : public Orhescyon::SystemCore<FrameBeginSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(DeltaTimeSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameEndSystem)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(CurrentFrameComponent), typeid(FrameImageComponent)};
	}
};