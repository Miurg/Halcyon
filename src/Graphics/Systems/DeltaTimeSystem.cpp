#include "DeltaTimeSystem.hpp"
#include "../GraphicsContexts.hpp"
#include "../Components/DeltaTimeComponent.hpp"
#include "../../Platform/Window.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void DeltaTimeSystem::update(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("DeltaTimeSystem");
#endif


	DeltaTimeComponent* dt = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>();
	float currentTime = static_cast<float>(Window::getTime());
	dt->deltaTime = currentTime - dt->lastFrameTime;
	dt->lastFrameTime = currentTime;
	dt->totalTime += dt->deltaTime;
}

void DeltaTimeSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "DeltaTimeSystem registered!" << std::endl;
}

void DeltaTimeSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "DeltaTimeSystem shutdown!" << std::endl;
}
