#include "MainLoop.hpp"
#include "Platform/Components/WindowComponent.hpp"
#include "Platform/PlatformContexts.hpp"
#include "Platform/Window.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

void MainLoop::startLoop(GeneralManager& gm)
{
#ifdef TRACY_ENABLE
	tracy::SetThreadName("Main");
#endif
	Window* window = gm.getContextComponent<MainWindowContext, WindowComponent>()->windowInstance;
	std::atomic<bool> physRunning{true};
	std::jthread physThread(
	    [&]
	    {
#ifdef TRACY_ENABLE
		    tracy::SetThreadName("Physics");
#endif
		    while (physRunning.load(std::memory_order_relaxed))
		    {
			    gm.update("physics");
		    }
	    });

	while (!window->shouldClose())
	{
		window->pollEvents();
		gm.update();
#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}

	physRunning.store(false, std::memory_order_relaxed);
	physThread.join();
}
