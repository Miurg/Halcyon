#include "SmithCore/Renderables.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Systems/TransformSystem.hpp"

void Smith::Renderables::forgeTransform(Orhescyon::GeneralManager& gm, Orhescyon::Entity e, glm::vec3 pos,
                                        glm::quat rot)
{
	if (!gm.hasComponent<LocalTransformComponent>(e))
	{
		gm.addComponentImmediate<LocalTransformComponent>(e);
	}

	if (!gm.hasComponent<GlobalTransformComponent>(e))
	{
		gm.addComponentImmediate<GlobalTransformComponent>(e, pos, rot);
	}

	if (!gm.hasComponent<RelationshipComponent>(e))
	{
		gm.addComponentImmediate<RelationshipComponent>(e);
	}

	if (!gm.isSubscribedTo<TransformSystem>(e))
	{
		gm.subscribeEntityImmediate<TransformSystem>(e);
	}

	// Raise the dirty flag so TransformSystem applies pos/rot; construction alone does not.
	GlobalTransformComponent* global = gm.getComponent<GlobalTransformComponent>(e);
	global->setGlobalPosition(pos);
	global->setGlobalRotation(rot);
}