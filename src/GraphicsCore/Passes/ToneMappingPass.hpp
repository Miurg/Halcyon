#pragma once
#include "IPass.hpp"
#include "../Resources/Managers/ResourceHandles.hpp"

class ToneMappingPass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	void onSettingsChanged(Orhescyon::GeneralManager& gm) override;

private:
	DSetHandle _dSetMainColor;
	DSetHandle _dSetExposure;
};
