#pragma once
#include "../../Core/Systems/SystemCore.hpp"
#include "../Components/GlobalTransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "../../Core/Entitys/EntityManager.hpp"

class BufferUpdateSystem
    : public SystemCore<BufferUpdateSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	struct Agent
	{
		Entity entity;
		GlobalTransformComponent* transform;
		MeshInfoComponent* meshInfo;
	};

	std::vector<Agent> _agents;

	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override;
};