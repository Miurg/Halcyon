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

	gm.addComponent<ControlComponent>(gm.getContext<MainCameraContext>());

	SkyboxFactory::loadSkybox("assets/textures/spree_bank_4k.hdr", gm);

	Entity gameObjectEntity1 = gm.createEntity();
	gm.addComponent<NameComponent>(gameObjectEntity1, "Bistro Model");
	gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
	gm.addComponent<LocalTransformComponent>(gameObjectEntity1, -5.0f, 0.0f, 0.0f);
	gm.addComponent<RelationshipComponent>(gameObjectEntity1);

	Entity dautherEntity = ModelFactory::loadModel("assets/models/SunTemple.glb", 0, *bufferManager, *dSetComponent,
	                                               *dManager, gm, *textureManager, *modelManager);

	gm.subscribeEntity<TransformSystem>(gameObjectEntity1);

	RelationshipComponent* real = gm.getComponent<RelationshipComponent>(gameObjectEntity1);
	real->addChild(gameObjectEntity1, dautherEntity, gm);

	//Entity gameObjectEntity2 = gm.createEntity();
	//gm.addComponent<NameComponent>(gameObjectEntity2, "Emissive Model");
	//gm.addComponent<GlobalTransformComponent>(gameObjectEntity2);
	//gm.addComponent<LocalTransformComponent>(gameObjectEntity2, -5.0f, 5.0f, 3.0f);
	//gm.addComponent<RelationshipComponent>(gameObjectEntity2);

	//Entity dautherEntity2 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager,
	//                                                *dSetComponent, *dManager, gm, *textureManager, *modelManager);

	//gm.subscribeEntity<TransformSystem>(gameObjectEntity2);
	////gm.subscribeEntity<RotationSystem>(gameObjectEntity2);
	//RelationshipComponent* real2 = gm.getComponent<RelationshipComponent>(gameObjectEntity2);
	//real2->addChild(gameObjectEntity2, dautherEntity2, gm);

	//Entity gameObjectEntity3 = gm.createEntity();
	//gm.addComponent<NameComponent>(gameObjectEntity3, "Emissive Model");
	//gm.addComponent<GlobalTransformComponent>(gameObjectEntity3);
	//gm.addComponent<LocalTransformComponent>(gameObjectEntity3, 0.0f, 5.0f, 0.0f);
	//gm.addComponent<RelationshipComponent>(gameObjectEntity3);

	//Entity dautherEntity3 = ModelFactory::loadModel("assets/models/CompareRoughness.glb", 0, *bufferManager,
	//                                                *dSetComponent, *dManager, gm, *textureManager, *modelManager);

	//gm.subscribeEntity<TransformSystem>(gameObjectEntity3);
	//gm.subscribeEntity<RotationSystem>(gameObjectEntity3);
	//RelationshipComponent* real3 = gm.getComponent<RelationshipComponent>(gameObjectEntity3);
	//real3->addChild(gameObjectEntity3, dautherEntity3, gm);
}