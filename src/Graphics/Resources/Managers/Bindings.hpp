#pragma once
#include <cstdint>

namespace Bindings
{
namespace Global
{
constexpr uint32_t Camera = 0;
constexpr uint32_t Sun = 1;
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
} // namespace Bindings
