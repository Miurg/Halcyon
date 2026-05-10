#include "MainLoop.hpp"
#include <iostream>
#include <exception>
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
	std::exception_ptr physException;

	std::jthread physThread(
	    [&]
	    {
#ifdef TRACY_ENABLE
		    tracy::SetThreadName("Physics");
#endif
		    try
		    {
			    while (physRunning.load(std::memory_order_relaxed))
			    {
				    gm.update("physics");
			    }
		    }
		    catch (...)
		    {
			    physException = std::current_exception();
			    physRunning.store(false, std::memory_order_relaxed);
			    window->setShouldClose(true);
		    }
	    });

	try
	{
		while (!window->shouldClose())
		{
			window->pollEvents();
			gm.update();
#ifdef TRACY_ENABLE
			FrameMark;
#endif
		}
	}
	catch (...)
	{
		physRunning.store(false, std::memory_order_relaxed);
		physThread.join();
		throw;
	}

	physRunning.store(false, std::memory_order_relaxed);
	physThread.join();

	if (physException)
	{
		std::rethrow_exception(physException);
	}
}
