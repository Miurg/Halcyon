#pragma once
#include "IPass.hpp"

class ToneMappingPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
};
