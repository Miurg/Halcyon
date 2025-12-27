#pragma once

#include "../VulkanDevice.hpp"
#include "../FrameData.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>
#include "../SwapChain.hpp"
#include "../PipelineHandler.hpp"
#include "../GameObject.hpp"
#include "../Resources/Managers/VertexIndexBuffer.hpp"
#include "../VulkanConst.hpp"
#include "../../Platform/Window.hpp"
#include "../../Core/Systems/SystemSubscribed.hpp"
#include "../Components/VulkanDeviceComponent.hpp"
#include "../../Platform/PlatformContexts.hpp"
#include "../Components/SwapChainComponent.hpp"
#include "../Components/PipelineHandlerComponent.hpp"
#include "../../Platform/Components/WindowComponent.hpp"
#include "../Components/FrameDataComponent.hpp"
#include "../Components/CurrentFrameComponent.hpp"
#include "../Components/GameObjectComponent.hpp"
#include "../Components/CameraComponent.hpp"

class RenderSystem : public SystemSubscribed<RenderSystem, GameObjectComponent>
{
public:
	void update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) override;
	void processEntity(Entity entity, GeneralManager& manager, float dt) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	void updateUniformBuffer(uint32_t currentImage, std::vector<GameObject*>& gameObjects,
	                         SwapChain& swapChain, uint32_t currentFrame, CameraComponent* mainCamera);

};