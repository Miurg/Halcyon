
#include <vector>
#include "../../Core/Systems/SystemCore.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Core/GeneralManager.hpp"
#include "../../Core/Entitys/EntityManager.hpp"

class SpawnSystem : public SystemCore<SpawnSystem>
{

public:
	int j = 0;
	int k = 0;
	float time = 0;
	std::vector<Entity> entities;
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	void onEntitySubscribed(Entity entity, GeneralManager& gm) override
	{
		entities.push_back(entity);
	}
	void onEntityUnsubscribed(Entity entity, GeneralManager& gm) override
	{
		entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
	}
};