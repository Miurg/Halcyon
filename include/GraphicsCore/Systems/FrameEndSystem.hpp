#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API FrameEndSystem : public Orhescyon::SystemCore<FrameEndSystem>
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