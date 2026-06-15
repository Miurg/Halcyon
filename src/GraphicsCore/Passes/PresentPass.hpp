#pragma once
#include "GraphicsCore/Passes/IPass.hpp"

class PresentPass : public IPass
{
public:
	void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) override;
};
