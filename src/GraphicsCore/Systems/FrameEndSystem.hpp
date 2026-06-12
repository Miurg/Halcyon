#pragma once

#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/FrameImageComponent.hpp"

using Orhescyon::GeneralManager;
class FrameEndSystem : public Orhescyon::SystemCore<FrameEndSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(FrameImageComponent)};
	}
};