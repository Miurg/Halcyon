#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/VulkanConst.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Systems/FrameBeginSystem.hpp"
#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include <array>

using Orhescyon::GeneralManager;

class HALCYON_API ReflectionProbeUpdateSystem
    : public Orhescyon::SystemCore<ReflectionProbeUpdateSystem, ReflectionProbeComponent>
{
public:
	struct HALCYON_API Agent
	{
		Orhescyon::Entity entity;
		ReflectionProbeComponent* probe;
	};

	std::vector<Agent> _agents;
	std::array<bool, MAX_REFLECTION_PROBES> _slotUsed{};

	int acquireSlot(ReflectionProbeComponent* probe);

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(BufferUpdateSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(ReflectionProbeComponent), typeid(CurrentFrameComponent)};
	}
};
