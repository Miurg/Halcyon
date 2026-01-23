#pragma once
#include "../../Core/Systems/SystemCore.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "../../Core/Entitys/EntityManager.hpp"

class BufferUpdateSystem
    : public SystemCore<BufferUpdateSystem, TransformComponent, TextureInfoComponent, MeshInfoComponent>
{
public:
	std::vector<Entity> entities;
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override
	{
		entities.push_back(entity);
	}
};