#pragma once

#include <vector>
#include "../../Core/Systems/SystemSubscribed.hpp"
#include "../../Graphics/Components/TransformComponent.hpp"


class RotationSystem: public SystemSubscribed<RotationSystem,TransformComponent>
{
public:
	void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) override;
	void processEntity(Entity entity, GeneralManager& manager, float dt) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};