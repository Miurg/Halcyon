#include "Platform/Components/CursorPositionComponent.hpp"
#include "Platform/Components/KeyboardStateComponent.hpp"
#include "Platform/Components/MouseStateComponent.hpp"
#include "Platform/Components/ScrollDeltaComponent.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Components/WindowSizeComponent.hpp"
#include "Core/GeneralManager.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"
#include "Game/Systems/ControlSystem.hpp"
#include "Graphics/Systems/RenderSystem.hpp"
#include "Graphics/Systems/BufferUpdateSystem.hpp"
#include "Graphics/Systems/FrameBeginSystem.hpp"
#include "Graphics/Systems/PresentSystem.hpp"

class CoreInit
{
public:
	static void Run(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

		gm.registerSystem<InputSolverSystem>();
		gm.registerSystem<ControlSystem>();
		gm.registerSystem<BufferUpdateSystem>();
		gm.registerSystem<FrameBeginSystem>();
		gm.registerSystem<RenderSystem>();
		gm.registerSystem<PresentSystem>();

#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
	};
};