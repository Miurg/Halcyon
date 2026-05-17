#include "Renderables.hpp"
#include "../Graphics/Components/GlobalTransformComponent.hpp"
#include "../Graphics/Components/LocalTransformComponent.hpp"
#include "../Graphics/Components/RelationshipComponent.hpp"
#include "../Graphics/Systems/TransformSystem.hpp"

void Smith::Renderables::forgeTransform(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, glm::vec3 pos,
                                        glm::quat rot)
{
	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponent<LocalTransformComponent>(e);
	}

	if (!gm.hasComponent<GlobalTransformComponent>(e))
	{
		gm.addComponent<GlobalTransformComponent>(e, pos, rot);
	}

	if (!gm.hasComponent<RelationshipComponent>(e))
	{
		gm.addComponent<RelationshipComponent>(e);
	}

	if (!gm.isSubscribedTo<TransformSystem>(e))
	{
		gm.subscribeEntity<TransformSystem>(e);
	}
}