#pragma once
#include "IPass.hpp"

class FXAAPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
	bool isEnabled(Orhescyon::GeneralManager& gm) const override;
};
