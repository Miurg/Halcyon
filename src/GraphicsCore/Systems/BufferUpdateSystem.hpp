#pragma once
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "FrameBeginSystem.hpp"
#include "FrameEndSystem.hpp"

using Orhescyon::GeneralManager;
class BufferUpdateSystem : public Orhescyon::SystemCore<BufferUpdateSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	struct Agent
	{
		Orhescyon::Entity entity;
		GlobalTransformComponent* transform;
		MeshInfoComponent* meshInfo;
	};

	std::vector<Agent> _agents;

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
		return {typeid(FrameEndSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(GlobalTransformComponent), typeid(MeshInfoComponent), typeid(CurrentFrameComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(DrawInfoComponent)};
	}
};