#pragma once

#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "BufferUpdateSystem.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/RelationshipComponent.hpp"
#include "FrameBeginSystem.hpp"
#include "DeltaTimeSystem.hpp"

using Orhescyon::GeneralManager;
class TransformSystem : public Orhescyon::SystemCore<TransformSystem, GlobalTransformComponent, LocalTransformComponent,
                                                   RelationshipComponent>
{
public:
	struct Agent
	{
		Entity entity;
		GlobalTransformComponent* global;
		LocalTransformComponent* local;
		RelationshipComponent* relationship;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
	std::vector<std::type_index> getAfterSystems() override
	{
		return {typeid(DeltaTimeSystem)};
	}
	std::vector<std::type_index> getBeforeSystems() override
	{
		return {typeid(FrameBeginSystem)};
	}
	std::vector<std::type_index> getReadComponents() override
	{
		return {typeid(RelationshipComponent)};
	}
	std::vector<std::type_index> getWriteComponents() override
	{
		return {typeid(LocalTransformComponent), typeid(GlobalTransformComponent)};
	}

private:
	static void applyPendingToLocal(LocalTransformComponent* local);
	static void applyPendingToGlobal(GlobalTransformComponent* global);
};