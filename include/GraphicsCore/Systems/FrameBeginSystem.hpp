#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"

using Orhescyon::GeneralManager;

/**
 * @brief Opens the render frame: waits for the GPU to release the current frame slot, reclaims retired resources, and
 * acquires a swap chain image for the new frame.
 *
 * Per run, in order:
 *  - Wait on the current slot's in-flight fence. Timeout skips the frame; device loss throws.
 *  - Collect deferred resource frees (textures, materials, geometry). Safe only after the fence wait. It is what proves
 * the GPU has finished the frames that still referenced them.
 *  - On pending window resize: recreate the swap chain and skip the frame.
 *  - Acquire the next swap chain image; on out-of-date: recreate the swap chain and skip the frame.
 *  - Reset the slot's fence and command buffer, publish the acquired image index via FrameImageComponent, mark the
 * frame valid and clear the render graph so passes register into a fresh frame.
 *
 * frameValid pairs this system with FrameEndSystem.
 * Every early return above leaves it false, so the end system does nothing and the same slot is retried on the next
 * run.
 */

class HALCYON_API FrameBeginSystem : public Orhescyon::SystemCore<FrameBeginSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};