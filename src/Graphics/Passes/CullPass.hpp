#pragma once
#include "IPass.hpp"

class CullPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
};
