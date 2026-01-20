#pragma once
#include "../../Core/Systems/SystemSubscribed.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"

class BufferUpdateSystem : public SystemSubscribed<BufferUpdateSystem, TransformComponent, TextureInfoComponent, MeshInfoComponent>
{
public:
	void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) override;
	void processEntity(Entity entity, GeneralManager& manager, float dt) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};