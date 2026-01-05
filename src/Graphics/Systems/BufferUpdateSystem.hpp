#pragma once
#include "../../Core/Systems/SystemSubscribed.hpp"
#include "../Components/TransformComponent.hpp"

class BufferUpdateSystem : public SystemSubscribed<BufferUpdateSystem, TransformComponent>
{
public:
	void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) override;
	void processEntity(Entity entity, GeneralManager& manager, float dt) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};