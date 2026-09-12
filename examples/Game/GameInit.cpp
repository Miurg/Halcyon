#include "GameInit.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Game/Systems/ControlSystem.hpp>
#include <Game/Components/ControlComponent.hpp>

#include <GraphicsCore/Systems/DeltaTimeSystem.hpp>
#include <GraphicsCore/Systems/FrameBeginSystem.hpp>
#include <GraphicsCore/GraphicsContexts.hpp>
#include <GraphicsCore/Components/NameComponent.hpp>
#include <GraphicsCore/Components/RelationshipComponent.hpp>
#include <SmithCore/Renderables.hpp>

void GameInit::Run(GeneralManager& gm)
{
	// Fly-camera controls, attached to the engine's built-in main camera.
	gm.registerSystem<ControlSystem>()
	    .after<DeltaTimeSystem>()
	    .before<FrameBeginSystem>()
	    .reads<DeltaTimeComponent, KeyboardStateComponent, CameraComponent, ControlComponent>()
	    .writes<GlobalTransformComponent, CursorPositionComponent>();
	gm.addComponentImmediate<ControlComponent>(gm.getContext<MainCameraContext>());

	Orhescyon::Entity cube = gm.createEntityImmediate();
	gm.addComponentImmediate<NameComponent>(cube, "Cube");
	Smith::Renderables::forgeTransform(gm, cube, glm::vec3(0.0f, 0.0f, -5.0f), glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

	Orhescyon::Entity sceneInstance =
	    Smith::Renderables::forgeSceneInstance(gm, "assets/models/cube.gltf");
	gm.getComponent<RelationshipComponent>(cube)->addChild(cube, sceneInstance, gm);
}
