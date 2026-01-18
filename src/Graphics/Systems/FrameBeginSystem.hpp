#pragma once

#include <vector>
#include "../../Core/Systems/SystemContextual.hpp"
#include "../Components/TransformComponent.hpp"
#include "../Resources/Components/TextureInfoComponent.hpp"
#include "../Resources/Components/MeshInfoComponent.hpp"

class FrameBeginSystem
    : public SystemContextual<FrameBeginSystem>
{
public:
	void update(float deltaTime, GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
};