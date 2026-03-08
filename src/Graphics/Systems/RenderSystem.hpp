#pragma once

#include <vector>
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include "../Components/GlobalTransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "BufferUpdateSystem.hpp"
using Orhescyon::GeneralManager;
class RenderSystem : public Orhescyon::SystemCore<RenderSystem, GlobalTransformComponent, MeshInfoComponent>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	virtual std::vector<std::type_index> getSystemDependencies() const override
	{
		return {typeid(BufferUpdateSystem)};
	}
};