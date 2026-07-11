#pragma once

#include "HalcyonExport.hpp"
#include <memory>
#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Passes/IPass.hpp"

class RenderGraph;

using Orhescyon::GeneralManager;
class HALCYON_API RenderSystem : public Orhescyon::SystemCore<RenderSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	void importFrameResources(GeneralManager& gm, RenderGraph& rg, uint32_t imageIndex);
	void applySettingsChanges(GeneralManager& gm);

	std::vector<std::unique_ptr<IPass>> _passes;
	uint32_t _lastWidth = 0;
	uint32_t _lastHeight = 0;
};
