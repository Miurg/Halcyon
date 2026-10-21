#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"

using Orhescyon::GeneralManager;

/**
 * @brief Closes the render frame: compiles the render graph, records it into the frame slot's command buffer, submits
 * it to the GPU and presents the finished image.
 *
 * Gated by frameValid: if the begin system bailed out (fence timeout, resize, out-of-date swap chain), this system does
 * nothing and the same slot is retried on the next run.
 *
 * Per run, in order:
 *  - Compile the render graph and execute it into the current slot's command buffer.
 *  - Submit the buffer to the graphics queue: wait on the semaphore signaled by image acquisition, signal
 * renderFinished once drawing is done, and the slot's in-flight fence once the GPU has finished the commands.
 *  - Present the image on the present queue; out-of-date/suboptimal triggers a swap chain recreate for the next frame.
 *  - Advance to the next frame slot (round-robin over MAX_FRAMES_IN_FLIGHT) and bump frameNumber.
 */

class HALCYON_API FrameEndSystem : public Orhescyon::SystemCore<FrameEndSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};