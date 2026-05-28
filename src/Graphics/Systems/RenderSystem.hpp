#pragma once

#include <memory>
#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "BufferUpdateSystem.hpp"
#include "FrameEndSystem.hpp"
#include "../Passes/IPass.hpp"

class RenderGraph;

using Orhescyon::GeneralManager;
class RenderSystem : public Orhescyon::SystemCore<RenderSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(BufferUpdateSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameEndSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(GlobalTransformComponent), typeid(MeshInfoComponent), typeid(DrawInfoComponent),
		        typeid(CurrentFrameComponent)};
	}

private:
	void importFrameResources(GeneralManager& gm, RenderGraph& rg, uint32_t imageIndex);
	void applySettingsChanges(GeneralManager& gm);

	std::vector<std::unique_ptr<IPass>> _passes;
	uint32_t _lastWidth = 0;
	uint32_t _lastHeight = 0;
};
