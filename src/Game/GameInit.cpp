#include "GameInit.hpp"
#include <iostream>
#include "../GraphicsCore/Resources/Factories/SkyboxFactory.hpp"
#include "../GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "../GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "../GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "../GraphicsCore/GraphicsContexts.hpp"
#include "../GraphicsCore/Components/BufferManagerComponent.hpp"
#include "../GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "../GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "../GraphicsCore/Components/LocalTransformComponent.hpp"
#include "../GraphicsCore/Components/RelationshipComponent.hpp"
#include "../GraphicsCore/Systems/TransformSystem.hpp"
#include "Systems/ControlSystem.hpp"
#include "Systems/RotationSystem.hpp"
#include "Systems/SpawnSystem.hpp"
#include "../GraphicsCore/Systems/RenderSystem.hpp"
#include "../GraphicsCore/Resources/Factories/ModelFactory.hpp"
#include "../GraphicsCore/Components/TextureManagerComponent.hpp"
#include "../GraphicsCore/Components/ModelManagerComponent.hpp"
#include "../GraphicsCore/Components/CameraComponent.hpp"
#include "Components/ControlComponent.hpp"
#include "../GraphicsCore/Components/NameComponent.hpp"
#include "../PlatformCore/PlatformContexts.hpp"
#include "../GraphicsCore/Components/PointLightComponent.hpp"
#include "../GraphicsCore/Systems/LightUpdateSystem.hpp"
#include "../GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../PhysicsCore/PhysContexts.hpp"
#include "../GraphicsCore/Systems/PhysSyncSystem.hpp"
#include "../PhysicsCore/Components/PhysTransformSnapshotComponent.hpp"
#include "../PhysicsCore/Systems/PhysSnapshotSystem.hpp"
#include "../SmithCore/Phys.hpp"
#include "../SmithCore/Renderables.hpp"
#include "../PhysicsCore/PhysShapes.hpp"
#include "../GraphicsCore/Components/ParticleEmitorComponent.hpp"
#include "../GraphicsCore/Systems/GPUParticlesSystem.hpp"

#pragma region Run
void GameInit::Run(GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "GAMEINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	coreInit(gm);
	initScene(gm);

#ifdef _DEBUG
	std::cout << "GAMEINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region coreInit
void GameInit::coreInit(GeneralManager& gm)
{
	gm.registerSystem<ControlSystem>();
	gm.registerSystem<RotationSystem>();
	gm.registerSystem<SpawnSystem>();
}
#pragma endregion

#pragma region initScene
void GameInit::initScene(GeneralManager& gm)
{
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* textureManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager* modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	BindlessTextureDSetComponent* dSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	PhysManager* physManager = gm.getContextComponent<PhysManagerContext, PhysManagerComponent>()->physManager;

	gm.addComponent<ControlComponent>(gm.getContext<MainCameraContext>());
	gm.subscribeEntity<TransformSystem>(gm.getContext<MainCameraContext>());
	gm.subscribeEntity<TransformSystem>(gm.getContext<SunContext>());

	SkyboxFactory::loadSkybox("assets/textures/spree_bank_4k.hdr", gm);

	{
		Orhescyon::Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity1, "Bistro_Godot");
		Smith::Renderables::forgeTransform(gm, gameObjectEntity1, glm::vec3(0.0f, 0.0f, 0.0f),
		                                   glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

		Body body1{.pos = glm::vec3(0.0f, 0.0f, 0.0f), .motion = Motion::Static, .layer = Layers::NON_MOVING};
		Smith::Phys::forgeBody(gm, gameObjectEntity1, body1, Box{.halfExtents = {50.0f, 0.5f, 50.0f}});

		Orhescyon::Entity dautherEntity = ModelFactory::loadModel("assets/models/Bistro_Godot.glb", 0, *bufferManager, *dSetComponent,
		                                               *dManager, gm, *textureManager, *modelManager);

		RelationshipComponent* real = gm.getComponent<RelationshipComponent>(gameObjectEntity1);
		real->addChild(gameObjectEntity1, dautherEntity, gm);
	}

	// Orhescyon::Entity gameObjectEntity2 = gm.createEntity();
	// gm.addComponent<NameComponent>(gameObjectEntity2, "Emissive Model");
	// gm.addComponent<GlobalTransformComponent>(gameObjectEntity2);
	// gm.addComponent<LocalTransformComponent>(gameObjectEntity2, -5.0f, 5.0f, 3.0f);
	// gm.addComponent<RelationshipComponent>(gameObjectEntity2);

	// Orhescyon::Entity dautherEntity2 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
	//                                                 *dSetComponent, *dManager, gm, *textureManager, *modelManager);

	// gm.subscribeEntity<TransformSystem>(gameObjectEntity2);
	////gm.subscribeEntity<RotationSystem>(gameObjectEntity2);
	// RelationshipComponent* real2 = gm.getComponent<RelationshipComponent>(gameObjectEntity2);
	// real2->addChild(gameObjectEntity2, dautherEntity2, gm);
	{
		Orhescyon::Entity gameObjectEntity3 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity3, "Emissive Model");

		
		ParticleEmitorComponent particlesSetting = {
		    .spawnCount = 100,
		    .directionalVector = {0.0f, 1.0f, 0.0f},
		    .spawnRadius = {0.0f, 1.0f},
		    .timeToLive = {1.0f, 3.0f},
		    .velocity = {1.0f, 5.0f},
		    .scale = {0.05f, 0.3f},
		};
		gm.addComponent<ParticleEmitorComponent>(gameObjectEntity3, particlesSetting);

		Smith::Renderables::forgeTransform(gm, gameObjectEntity3, glm::vec3(0.0f, 10.0f, 0.0f),
		                                   glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

		Body body{.pos = glm::vec3(0.0f, 10.0f, 0.0f), .motion = Motion::Dynamic, .friction = 0.5f, .restitution = 0.1f};
		Smith::Phys::forgeBody(gm, gameObjectEntity3, body, Sphere{0.5f});

		Orhescyon::Entity dautherEntity3 =
		    ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager, *dSetComponent, *dManager, gm,
		                            *textureManager, *modelManager);
		gm.subscribeEntity<GPUParticlesSystem>(gameObjectEntity3);
		RelationshipComponent* real3 = gm.getComponent<RelationshipComponent>(gameObjectEntity3);
		real3->addChild(gameObjectEntity3, dautherEntity3, gm);
	}

	{
		Orhescyon::Entity gameObjectEntity4 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity4, "Emissive Model");

		ParticleEmitorComponent particlesSetting = {
			.spawnCount = 100,
		    .directionalVector = {0.0f, 1.0f, 0.0f},
		    .spawnRadius = {0.0f, 1.0f},
		    .timeToLive = {1.0f, 3.0f},
		    .velocity = {1.0f, 5.0f},
		    .scale = {0.05f, 0.3f},
		};
		gm.addComponent<ParticleEmitorComponent>(gameObjectEntity4, particlesSetting);
		Smith::Renderables::forgeTransform(gm, gameObjectEntity4, glm::vec3(0.0f, 12.0f, 0.1f),
		                                   glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

		Body body4{.pos = glm::vec3(0.0f, 12.0f, 0.1f), .motion = Motion::Dynamic};
		Smith::Phys::forgeBody(gm, gameObjectEntity4, body4, Sphere{0.5f});

		Orhescyon::Entity dautherEntity4 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
		                                                *dSetComponent, *dManager, gm, *textureManager, *modelManager);

		// gm.subscribeEntity<RotationSystem>(gameObjectEntity4);
		gm.subscribeEntity<GPUParticlesSystem>(gameObjectEntity4);
		RelationshipComponent* real4 = gm.getComponent<RelationshipComponent>(gameObjectEntity4);
		real4->addChild(gameObjectEntity4, dautherEntity4, gm);
	}
	// Configure the probe grid.
	//LightProbeGridComponent* probeGrid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	//probeGrid->origin = glm::vec3(-16.0f, 1.0f, -4.0f);
	//probeGrid->count = glm::ivec3(6, 3, 4);
	//probeGrid->spacing = 2.6f;
	//gm.addComponent<NameComponent>(gm.getContext<LightProbeGridContext>(), "GI Grid");

	physManager->physicsSystem->OptimizeBroadPhase();
}
#pragma endregion
