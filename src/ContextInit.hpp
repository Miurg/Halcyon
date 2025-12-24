#pragma once

#include "Core/GeneralManager.hpp"
#include "Core/Entitys/EntityManager.hpp"

#include "Graphics/DescriptorHandler.hpp"
#include "Graphics/Components/DescriptorHandlerComponent.hpp"
#include "Graphics/Contexts/MainDescriptorHandlerContext.hpp"

#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Components/KeyboardStateComponent.hpp"
#include "Platform/Components/MouseStateComponent.hpp"
#include "Platform/Components/CursorPositionComponent.hpp"
#include "Platform/Components/WindowSizeComponent.hpp"
#include "Platform/Components/ScrollDeltaComponent.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"


class ContextInit
{
	public:
	static void Run(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "CONTEXTINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

		// Window and input
		Entity windowAndInputEntity = gm.CreateEntity();
		gm.addComponent<WindowComponent>(windowAndInputEntity, &window);
		gm.addComponent<KeyboardStateComponent>(windowAndInputEntity);
		gm.addComponent<MouseStateComponent>(windowAndInputEntity);
		gm.addComponent<CursorPositionComponent>(windowAndInputEntity);
		gm.addComponent<WindowSizeComponent>(windowAndInputEntity, ScreenWidth, ScreenHeight);
		gm.addComponent<ScrollDeltaComponent>(windowAndInputEntity);
		gm.subscribeEntity<InputSolverSystem>(windowAndInputEntity);
		auto inputCtx = std::make_unique<InputDataContext>();
		inputCtx->InputInstance = windowAndInputEntity;
		gm.RegisterContext<InputDataContext>(std::move(inputCtx));
		auto windowCtx = std::make_unique<MainWindowContext>();
		windowCtx->WindowInstance = windowAndInputEntity;
		gm.RegisterContext<MainWindowContext>(std::move(windowCtx));


		 // Camera
		//Entity descriptorEntity = gm.CreateEntity();
		//gm.addComponent<DescriptorHandlerComponent>(descriptorEntity);
		//auto descriptorCtx = std::make_unique<MainDescriptorHandlerContext>();
		//descriptorCtx->descriptorInstance = descriptorEntity;
		//gm.registerContext<MainDescriptorHandlerContext>(std::move(descriptorCtx));

#ifdef _DEBUG
		std::cout << "CONTEXTINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
	};
}