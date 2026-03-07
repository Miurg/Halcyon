
#include <vector>
#include "../../Orhescyon/Systems/SystemCore.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Orhescyon/GeneralManager.hpp"
#include "../../Orhescyon/Entitys/EntityManager.hpp"
using Orhescyon::GeneralManager;
class SpawnSystem : public Orhescyon::SystemCore<SpawnSystem>
{

public:
	int j = 0;
	int k = 0;
	float time = 0;
	std::vector<Entity> entities;
	void update(GeneralManager& gm) override;
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