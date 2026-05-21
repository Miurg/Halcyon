#include "PresentPass.hpp"

#include "../RenderGraph/RenderGraph.hpp"

void PresentPass::addToGraph(Orhescyon::GeneralManager& /*gm*/, RenderGraph& rg, uint32_t /*frame*/)
{
	rg.addPass("Present", {}, {{"swapChainImage", RGResourceUsage::Present}}, {}, nullptr);
}
