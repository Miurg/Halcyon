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
	void updateTransformRecursive(Entity entity, GeneralManager& gm, glm::vec3 parentPosition, glm::quat parentRotation,
	                              glm::vec3 parentScale);

public:
	std::vector<Entity> entities;
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override;
};