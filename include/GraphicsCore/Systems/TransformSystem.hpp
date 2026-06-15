#pragma once

#include "HalcyonExport.hpp"
#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Systems/FrameBeginSystem.hpp"
#include "GraphicsCore/Systems/DeltaTimeSystem.hpp"

using Orhescyon::GeneralManager;
class HALCYON_API TransformSystem : public Orhescyon::SystemCore<TransformSystem, GlobalTransformComponent, LocalTransformComponent,
                                                   RelationshipComponent>
{
public:
	struct HALCYON_API Agent
	{
		Orhescyon::Entity entity;
		GlobalTransformComponent* global;
		LocalTransformComponent* local;
		RelationshipComponent* relationship;
	};

	std::vector<Agent> _agents;

	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Orhescyon::Entity entity, GeneralManager& gm) override;
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