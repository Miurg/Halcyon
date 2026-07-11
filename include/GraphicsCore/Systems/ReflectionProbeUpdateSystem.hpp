#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <array>

using Orhescyon::GeneralManager;

class HALCYON_API ReflectionProbeUpdateSystem
    : public Orhescyon::SystemCore<ReflectionProbeUpdateSystem, ReflectionProbeComponent>
{
public:
	std::array<bool, MAX_REFLECTION_PROBES> _slotUsed{};

	int acquireSlot(ReflectionProbeComponent* probe);

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
};
