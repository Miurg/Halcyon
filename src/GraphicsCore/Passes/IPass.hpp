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

	virtual void onInit(Orhescyon::GeneralManager& gm) {}
	virtual void onShutdown(Orhescyon::GeneralManager& gm) {}
	virtual void onResize(Orhescyon::GeneralManager& gm, uint32_t width, uint32_t height) {}
	virtual void onSettingsChanged(Orhescyon::GeneralManager& gm) {}

	virtual void addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame) = 0;

	virtual bool isEnabled(Orhescyon::GeneralManager& gm) const
	{
		return true;
	}
};
