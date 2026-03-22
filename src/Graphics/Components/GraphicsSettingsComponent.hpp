#pragma once

struct GraphicsSettingsComponent
{
	//bool enableFrustumCulling = true;
	//bool enableShadowCulling = true;
	bool enableSsao = true;
	bool enableFxaa = true;
	//bool enableBloom = true;
	//bool enableToneMapping = true;
	//bool enableGammaCorrection = true;
	GraphicsSettingsComponent() = default;
};