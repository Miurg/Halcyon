#pragma once

#include "../../Core/Systems/SystemCore.hpp"

class ImGuiSystem : public SystemCore<ImGuiSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	uint32_t frameCount = 0;
	float time = 0;
	uint32_t fps = 0;
	Entity selectedEntity = static_cast<Entity>(-1);

private:
	void drawEntityNode(Entity entity, GeneralManager& gm);
};