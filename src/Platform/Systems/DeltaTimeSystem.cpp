#include "DeltaTimeSystem.hpp"
#include "../PlatformContexts.hpp"
#include "../Components/DeltaTimeComponent.hpp"
#include "../Window.hpp"

void DeltaTimeSystem::update(GeneralManager& gm)
{
	DeltaTimeComponent* dt = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>();
	float currentTime = static_cast<float>(Window::getTime());
	dt->deltaTime = currentTime - dt->lastFrameTime;
	dt->lastFrameTime = currentTime;
	dt->totalTime += dt->deltaTime;
}

void DeltaTimeSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "DeltaTimeSystem registered!" << std::endl;
	Entity deltaTimeEntity = gm.createEntity();
	gm.registerContext<DeltaTimeContext>(deltaTimeEntity);
	gm.addComponent<DeltaTimeComponent>(deltaTimeEntity);
}

void DeltaTimeSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "DeltaTimeSystem shutdown!" << std::endl;
}
