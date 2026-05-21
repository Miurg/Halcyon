#pragma once
#include <cstdint>

namespace Orhescyon
{
class GeneralManager;
}

class RenderGraph;

class IPass
{
public:
	virtual ~IPass() = default;

	virtual void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) = 0;

	virtual bool isEnabled(Orhescyon::GeneralManager& gm) const { return true; }
};
