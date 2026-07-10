# Halcyon

**Halcyon** is a custom rendering engine built from scratch in **C++** and **Vulkan**.

<img width="1919" height="977" alt="image_2026-07-10_06-17-30" src="https://github.com/user-attachments/assets/bd38d79f-0078-4f65-891e-5cc278ef9f09" />

![Image](https://github.com/user-attachments/assets/7f56ab3a-94a8-49e3-b51d-1e54397cefea)

## 🛠 Tech Stack

* **Language:** C++20
* **Graphics API:** Vulkan
* **Build System:** CMake (+ vcpkg for dependencies)

## Features

### Rendering
- **Forward rendering** pipeline
- **PBR (Cook-Torrance BRDF)** - metallic-roughness workflow with GGX distribution, correlated Smith-GGX visibility, Schlick Fresnel
- **Image Based Lighting (IBL)** - irradiance maps, prefiltered environment maps, BRDF LUT (split-sum)
- **GPU Driven Rendering** with bindless textures
- **Render Graph**
- **GTAO**
- **FXAA** with **MSAA**
- **Directional shadows** with PCF 3×3 filtering

### Architecture
- **ECS core** - POD components, explicit system subscriptions, zero overhead
- **VMA** - Vulkan Memory Allocator for GPU memory management
- **Jolt** - as physics core

### Assets & Tooling
- **glTF 2.0** model loading
- **Dear ImGui** debug UI *(dev builds only)*
- **Shader hot-reload** *(dev builds only)*

## Getting Started

### Prerequisites

| Tool | Notes |
|------|-------|
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | Requires **Desktop development with C++** workload |
| [CMake](https://cmake.org/) ≥ 3.20 | |
| [Ninja](https://ninja-build.org/) | Or install via Visual Studio Installer |
| [vcpkg](https://github.com/microsoft/vcpkg) | Set `VCPKG_ROOT` environment variable after installing |
| [Vulkan SDK](https://vulkan.lunarg.com/) | Includes `slangc` — no separate Slang install needed |

### Build from source

```bash
# Orhescyon is a git submodule — clone recursively
git clone --recursive https://github.com/Miurg/Halcyon.git
cd Halcyon

# Configure (vcpkg dependencies are fetched automatically via the preset toolchain)
cmake --preset x64-debug

# Build
cmake --build out/build/x64-debug
```

> For a release build replace `x64-debug` with `x64-release`.
>
> The debug preset builds single-threaded for cleaner error output.

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `HALCYON_DEV_TOOLS` | `ON` | In-engine dev tools: ImGui debug UI + shader hot-reload. Set `OFF` for distributable SDK builds. |
| `HALCYON_BUILD_EXAMPLES` | `ON` when top-level | Build the sample in `examples/`. |
| `HALCYON_SHADER_OUTPUT_DIR` | `<build>/shaders` | Where compiled `.spv` shaders are written. |
| `BUILD_SHARED_LIBS` | `OFF` | Build as a shared library (experimental). |

## 📦 Using Halcyon as an SDK

### 1. Build & install the SDK

```bash
cmake -S . -B build_sdk -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DHALCYON_DEV_TOOLS=OFF -DHALCYON_BUILD_EXAMPLES=OFF \
  -DCMAKE_INSTALL_PREFIX=/path/to/HalcyonSDK \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake

cmake --build build_sdk
cmake --install build_sdk
```

This produces a package: `include/`, `lib/Halcyon.lib`, `lib/cmake/Halcyon/`, `share/Halcyon/shaders/`.

### 2. Consume it from your project

```cmake
find_package(Halcyon CONFIG REQUIRED)

add_executable(mygame main.cpp)
target_link_libraries(mygame PRIVATE Halcyon::Halcyon)

# Copy the engine's stock shaders next to your executable
halcyon_copy_shaders(mygame)
```

```cpp
#include <Halcyon.hpp>
#include "MyGame.hpp" // your IStartUp implementation

int main()
{
    MyGame game;
    return App::create().addStartUp(game).run();
}
```

The `examples/` project is a working reference — it builds both in-source (via `add_subdirectory`)
and against an installed SDK (via `find_package`), depending on how you configure it.

### Shaders at runtime

The engine locates its compiled `.spv` shaders at runtime in this order:

1. the `HALCYON_SHADER_DIR` environment variable, if set;
2. a `shaders/` folder next to the executable (what `halcyon_copy_shaders` produces);
3. the path baked in at build time.

So for a redistributable, either ship `shaders/` next to your `.exe`, or point
`HALCYON_SHADER_DIR` at wherever you placed them.

## 🎯 Project Goals

The main goal of the project is to create a forward rendering engine capable of displaying
excellent images without temporal techniques.
