#include "GameInit.hpp"
#include "../Graphics/Resources/Factories/SkyboxFactory.hpp"
#include "../Graphics/Resources/Components/MeshInfoComponent.hpp"
#include "../Graphics/Resources/Managers/BufferManager.hpp"
#include "../Graphics/Resources/Components/TextureInfoComponent.hpp"
#include "../Graphics/GraphicsContexts.hpp"
#include "../Graphics/Components/BufferManagerComponent.hpp"
#include "../Graphics/Components/DescriptorManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "../Graphics/Components/GlobalTransformComponent.hpp"
#include "../Graphics/Components/LocalTransformComponent.hpp"
#include "../Graphics/Components/RelationshipComponent.hpp"
#include "../Graphics/Systems/TransformSystem.hpp"
#include "Systems/RotationSystem.hpp"
#include "../Graphics/Systems/RenderSystem.hpp"
#include "../Graphics/Resources/Factories/ModelFactory.hpp"
#include "../Graphics/Components/TextureManagerComponent.hpp"
#include "../Graphics/Components/ModelManagerComponent.hpp"
#include "../Graphics/Components/CameraComponent.hpp"
#include "Components/ControlComponent.hpp"
#include "../Graphics/Components/NameComponent.hpp"
#include "../Platform/PlatformContexts.hpp"
#include "../Graphics/Components/PointLightComponent.hpp"
#include "../Graphics/Systems/LightUpdateSystem.hpp"
#include "../Graphics/Components/LightProbeGridComponent.hpp"
#include "../PhysicsCore/Components/PhysBodyComponent.hpp"
#include "../PhysicsCore/Components/PhysManagerComponent.hpp"
#include "../PhysicsCore/PhysContexts.hpp"
#include "../Graphics/Systems/PhysSyncSystem.hpp"
#include "../PhysicsCore/Components/PhysTransformSnapshot.hpp"
#include "../PhysicsCore/Systems/PhysSnapshotSystem.hpp"

void GameInit::gameInitStart(GeneralManager& gm)
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
		Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity1, "Bistro_Godot");
		gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
		gm.addComponent<LocalTransformComponent>(gameObjectEntity1, 0.0f, 0.0f, 0.0f);
		gm.addComponent<RelationshipComponent>(gameObjectEntity1);
		gm.addComponent<PhysBodyComponent>(
		    gameObjectEntity1, physManager->createStaticBox(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 0.5f, 50.0f)));
		gm.addComponent<PhysTransformSnapshot>(gameObjectEntity1);

		Entity dautherEntity = ModelFactory::loadModel("assets/models/Sponza.glb", 0, *bufferManager, *dSetComponent,
		                                               *dManager, gm, *textureManager, *modelManager);
		gm.subscribeEntity<PhysSnapshotSystem>(gameObjectEntity1);
		gm.subscribeEntity<TransformSystem>(gameObjectEntity1);
		gm.subscribeEntity<PhysSyncSystem>(gameObjectEntity1);
		
		RelationshipComponent* real = gm.getComponent<RelationshipComponent>(gameObjectEntity1);
		real->addChild(gameObjectEntity1, dautherEntity, gm);
	}

	// Entity gameObjectEntity2 = gm.createEntity();
	// gm.addComponent<NameComponent>(gameObjectEntity2, "Emissive Model");
	// gm.addComponent<GlobalTransformComponent>(gameObjectEntity2);
	// gm.addComponent<LocalTransformComponent>(gameObjectEntity2, -5.0f, 5.0f, 3.0f);
	// gm.addComponent<RelationshipComponent>(gameObjectEntity2);

	// Entity dautherEntity2 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
	//                                                 *dSetComponent, *dManager, gm, *textureManager, *modelManager);

	// gm.subscribeEntity<TransformSystem>(gameObjectEntity2);
	////gm.subscribeEntity<RotationSystem>(gameObjectEntity2);
	// RelationshipComponent* real2 = gm.getComponent<RelationshipComponent>(gameObjectEntity2);
	// real2->addChild(gameObjectEntity2, dautherEntity2, gm);
	{
		Entity gameObjectEntity3 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity3, "Emissive Model");
		gm.addComponent<GlobalTransformComponent>(gameObjectEntity3);
		gm.addComponent<LocalTransformComponent>(gameObjectEntity3, 0.0f, 10.0f, 0.0f);
		gm.addComponent<RelationshipComponent>(gameObjectEntity3);
		gm.addComponent<PhysBodyComponent>(gameObjectEntity3,
		                                   physManager->createDynamicSphere(glm::vec3(0.0f, 10.0f, 0.0f), 0.5f));
		gm.addComponent<PhysTransformSnapshot>(gameObjectEntity3);
		Entity dautherEntity3 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
		                                                *dSetComponent, *dManager, gm, *textureManager, *modelManager);

		gm.subscribeEntity<PhysSyncSystem>(gameObjectEntity3);
		gm.subscribeEntity<TransformSystem>(gameObjectEntity3);
		gm.subscribeEntity<PhysSnapshotSystem>(gameObjectEntity3);
		// gm.subscribeEntity<RotationSystem>(gameObjectEntity3);
		RelationshipComponent* real3 = gm.getComponent<RelationshipComponent>(gameObjectEntity3);
		real3->addChild(gameObjectEntity3, dautherEntity3, gm);
	}

	{
		Entity gameObjectEntity4 = gm.createEntity();
		gm.addComponent<NameComponent>(gameObjectEntity4, "Emissive Model");
		gm.addComponent<GlobalTransformComponent>(gameObjectEntity4);
		gm.addComponent<LocalTransformComponent>(gameObjectEntity4, 0.0f, 12.0f, 0.1f);
		gm.addComponent<RelationshipComponent>(gameObjectEntity4);
		gm.addComponent<PhysBodyComponent>(gameObjectEntity4,
		                                   physManager->createDynamicSphere(glm::vec3(0.0f, 12.0f, 0.1f), 0.5f));
		gm.addComponent<PhysTransformSnapshot>(gameObjectEntity4);
		Entity dautherEntity4 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
		                                                *dSetComponent, *dManager, gm, *textureManager, *modelManager);
		gm.subscribeEntity<PhysSyncSystem>(gameObjectEntity4);
		gm.subscribeEntity<TransformSystem>(gameObjectEntity4);
		gm.subscribeEntity<PhysSnapshotSystem>(gameObjectEntity4);
		// gm.subscribeEntity<RotationSystem>(gameObjectEntity3);
		RelationshipComponent* real4 = gm.getComponent<RelationshipComponent>(gameObjectEntity4);
		real4->addChild(gameObjectEntity4, dautherEntity4, gm);
	}
		// Configure the probe grid.
	LightProbeGridComponent* probeGrid = gm.getContextComponent<LightProbeGridContext, LightProbeGridComponent>();
	probeGrid->origin = glm::vec3(-16.0f, 1.0f, -4.0f);
	probeGrid->count = glm::ivec3(6, 3, 4);
	probeGrid->spacing = 2.6f;
	gm.addComponent<NameComponent>(gm.getContext<LightProbeGridContext>(), "GI Grid");

	physManager->physicsSystem->OptimizeBroadPhase();
}