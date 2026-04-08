#pragma once
#include <cstdint>

namespace Bindings
{
namespace Global
{
constexpr uint32_t Camera = 0;
constexpr uint32_t Sun = 1;
constexpr uint32_t PointLights = 2;
constexpr uint32_t PointLightCount = 3;
constexpr uint32_t SHProbes = 4;
constexpr uint32_t SHProbeCount = 5;
} // namespace Global

namespace Model
{
constexpr uint32_t Primitives = 0;
constexpr uint32_t Transforms = 1;
constexpr uint32_t IndirectDraw = 2;
constexpr uint32_t VisibleIndices = 3;
constexpr uint32_t CompactedDraw = 4;
constexpr uint32_t DrawCount = 5;
} // namespace Model

namespace Textures
{
constexpr uint32_t Array = 0;
constexpr uint32_t ShadowMap = 1;
constexpr uint32_t Materials = 2;
constexpr uint32_t CubemapSampler = 3;
constexpr uint32_t CubemapStorage = 4;
constexpr uint32_t PrefilteredMap = 6;
constexpr uint32_t BrdfLut = 7;
} // namespace Textures

namespace FXAA
{
constexpr uint32_t ColorInput = 0;
constexpr uint32_t SsaoInput = 1;
constexpr uint32_t ColorInput2 = 2;
} // namespace FXAA

namespace SSAO
{
constexpr uint32_t DepthInput = 0;
constexpr uint32_t NormalsInput = 1;
constexpr uint32_t NoiseInput = 2;
} // namespace SSAO

namespace SSAOBlur
{
constexpr uint32_t SsaoInput = 0;
constexpr uint32_t DepthInput = 1;
constexpr uint32_t NormalsInput = 2;
} // namespace SSAOBlur
namespace SSAOApply
{
constexpr uint32_t ColorInput = 0;
constexpr uint32_t SsaoInput = 1;
} // namespace SSAOApply
namespace ToneMapping
{
constexpr uint32_t OffscreenInput = 0;
}
namespace BloomDownsample
{
constexpr uint32_t InputTexture = 0;
}
namespace BloomUpsample
{
constexpr uint32_t CurrentTexture = 0;
constexpr uint32_t PreviousTexture = 1;
} // namespace BloomUpsample
namespace Vignette
{
constexpr uint32_t OffscreenInput = 0;
}
} // namespace Bindings
