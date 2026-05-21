#pragma once
#include "IPass.hpp"

class DirectLightPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
};
