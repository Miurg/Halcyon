#pragma once

#include <vector>
#include "../../Core/Systems/SystemCore.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "BufferUpdateSystem.hpp"
#include "../Components/LocalTransformComponent.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Components/RelationshipComponent.hpp"

class TransformSystem
    : public SystemCore<TransformSystem, GlobalTransformComponent, LocalTransformComponent, RelationshipComponent>
{
private:
public:
	struct Agent
	{
		Entity entity;
		GlobalTransformComponent* global;
		LocalTransformComponent* local;
		RelationshipComponent* relationship;
	};

	std::vector<Agent> _agents;

	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
};