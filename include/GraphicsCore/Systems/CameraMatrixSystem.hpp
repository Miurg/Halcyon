#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"

using Orhescyon::GeneralManager;

/**
 * @brief Fills the per-frame camera and sun UBOs consumed by culling and
 *        rendering.
 *
 * Camera (CameraData):
 *  - view/proj matrices (reversed-Z, Y flipped for Vulkan)
 *  - invViewProj, camera position, screen size
 *  - frustum planes from proj*view — the GPU cull passes test AABBs
 *    against them
 *
 * Sun (DirectionalLightData):
 *  - ortho light volume fitted around the camera frustum
 *    (capped at shadowDistance)
 *  - stabilized against camera motion: radius rounded up, origin snapped
 *    to whole shadow-map texels
 *  - light-space cull planes and camera-visible bounds in light space
 */

class HALCYON_API CameraMatrixSystem : public Orhescyon::SystemCore<CameraMatrixSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};