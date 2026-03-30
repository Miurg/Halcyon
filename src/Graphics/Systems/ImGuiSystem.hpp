#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include <Orhescyon/Systems/SystemCore.hpp>
#include <vector>

using Orhescyon::GeneralManager;
class ImGuiSystem : public Orhescyon::SystemCore<ImGuiSystem>
{
public:
	void update(GeneralManager& gm) override;
	void onRegistered(GeneralManager& gm) override;
	void onShutdown(GeneralManager& gm) override;
	uint32_t frameCount = 0;
	float time = 0;
	uint32_t fps = 0;
	Entity selectedEntity = static_cast<Entity>(-1);

	std::vector<float> frameTimes;
	float avgFrameTime = 0.0f;
	float onePercentLowFrameTime = 0.0f;
	
	bool autoShaderReload = false;

private:
	void drawEntityNode(Entity entity, GeneralManager& gm);
};