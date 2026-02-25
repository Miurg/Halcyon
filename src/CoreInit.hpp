#pragma once
#include "Core/GeneralManager.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"
#include "Game/Systems/ControlSystem.hpp"
#include "Graphics/Systems/RenderSystem.hpp"
#include "Graphics/Systems/BufferUpdateSystem.hpp"
#include "Graphics/Systems/FrameBeginSystem.hpp"
#include "Graphics/Systems/FrameEndSystem.hpp"
#include "Graphics/Systems/CameraMatrixSystem.hpp"
#include "Game/Systems/RotationSystem.hpp"
#include "Graphics/Systems/TransformSystem.hpp"
#include "Game/Systems/SpawnSystem.hpp"
#include "Graphics/Systems/ImGuiSystem.hpp"
#include "Platform/Systems/DeltaTimeSystem.hpp"


class CoreInit
{
public:
	static void Run(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

		gm.registerSystem<DeltaTimeSystem>();
		gm.registerSystem<InputSolverSystem>();
		gm.registerSystem<ControlSystem>();
		gm.registerSystem<RotationSystem>();
		gm.registerSystem<SpawnSystem>();
		gm.registerSystem<TransformSystem>();

		gm.registerSystem<FrameBeginSystem>();
		gm.registerSystem<ImGuiSystem>();
		gm.registerSystem<CameraMatrixSystem>();
		gm.registerSystem<BufferUpdateSystem>();
		gm.registerSystem<RenderSystem>();
		gm.registerSystem<FrameEndSystem>();

#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
	};
};