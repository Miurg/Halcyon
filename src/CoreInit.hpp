#include "Platform/Components/CursorPositionComponent.hpp"
#include "Platform/Components/KeyboardStateComponent.hpp"
#include "Platform/Components/MouseStateComponent.hpp"
#include "Platform/Components/ScrollDeltaComponent.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/Components/WindowSizeComponent.hpp"
#include "Core/GeneralManager.hpp"
#include "Platform/Systems/InputSolverSystem.hpp"

class CoreInit
{
public:
	static void Run(GeneralManager& gm)
	{
#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

		gm.registerSystem<InputSolverSystem>();

#ifdef _DEBUG
		std::cout << "COREINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
	};
};