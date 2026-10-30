#pragma once

#include "HalcyonExport.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "Shared/GpuStructs.h"
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <vector>

using Orhescyon::GeneralManager;

/**
 * @brief Builds the per-frame GPU draw data: batches entities by mesh and
 *        fills the transform, per-object and indirect-draw buffers that the
 *        cull and render passes execute.
 */

class HALCYON_API BufferUpdateSystem
    : public Orhescyon::SystemCore<BufferUpdateSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;

private:
	struct Agent
	{
		GlobalTransformComponent* transform;
		MeshInfoComponent* meshInfo;
	};

	std::vector<std::vector<Agent>> batchMeshTransf;
	std::vector<int> baseTransformPerBatch;
};
