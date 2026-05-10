#include "MainLoop.hpp"
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include "PhysicsCore/Components/PhysTickRateComponent.hpp"
#include "PhysicsCore/PhysContexts.hpp"
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
		    const auto* tickRate = gm.getContextComponent<PhysTickRateContext, PhysTickRateComponent>();

		    using clock = std::chrono::steady_clock;
		    auto nextStepTime = clock::now() + std::chrono::duration_cast<clock::duration>(
		                                           std::chrono::duration<double>(1.0 / tickRate->rate));
		    int missedSteps = 0;
		    try
		    {
			    while (physRunning.load(std::memory_order_relaxed))
			    {
				    std::this_thread::sleep_until(nextStepTime);
				    gm.update("physics");

				    const auto interval =
				        std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / tickRate->rate));
				    nextStepTime += interval;
				    const auto now = clock::now();

				    if (now > nextStepTime + interval)
				    {
					    if (++missedSteps >= tickRate->maxConsecutiveMissedSteps)
					    {
						    nextStepTime = now + interval;
						    missedSteps = 0;
					    }
				    }
				    else
				    {
					    missedSteps = 0;
				    }
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
