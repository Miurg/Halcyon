#pragma once
#include "IPass.hpp"
#include "../Resources/Managers/ResourceHandles.hpp"

class VignettePass : public IPass
{
public:
	void onInit(Orhescyon::GeneralManager& gm) override;
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;

private:
	DSetHandle _dset;
};
