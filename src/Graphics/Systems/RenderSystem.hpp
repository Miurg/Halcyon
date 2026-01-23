#pragma once

#include <vector>
#include "../../Core/Systems/SystemCore.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"
#include "BufferUpdateSystem.hpp"


class RenderSystem : public SystemCore<RenderSystem, TextureInfoComponent, TransformComponent, MeshInfoComponent>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	virtual std::vector<std::type_index> getSystemDependencies() const override
	{
		return {typeid(BufferUpdateSystem)};
	}
};