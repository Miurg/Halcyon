#pragma once

#include "HalcyonExport.hpp"
#include <cstdint>

struct HALCYON_API BufferHandle
{
	int id = -1;
};

struct HALCYON_API DSetHandle
{
	int id = -1;
};

struct HALCYON_API TextureHandle
{
	int id = -1;
};

struct HALCYON_API RenderAssetHandle
{
	int id = -1;
};

struct HALCYON_API MeshHandle
{
	int id = -1;
};

struct HALCYON_API MaterialHandle
{
	int id = -1;
};

struct HALCYON_API SamplerHandle
{
	int id = -1;
};

struct HALCYON_API SceneTemplateHandle
{
	int id = -1;
};

struct HALCYON_API SceneTemplateNodeHandle
{
	int id = -1;
};

struct HALCYON_API SceneTemplateTransformHandle
{
	int id = -1;
};

struct HALCYON_API SceneTemplateLightHandle
{
	int id = -1;
};
