# Halcyon

**Halcyon** is a custom game engine currently in early development. It is built from scratch using **C++** and **Vulkan**.

![Image](https://github.com/user-attachments/assets/7f56ab3a-94a8-49e3-b51d-1e54397cefea)

![Image](https://github.com/user-attachments/assets/d2c4ad00-2d68-4bc0-abc0-60c7defaabb7)

## 🛠 Tech Stack

* **Language:** C++
* **Graphics API:** Vulkan
* **Build System:** CMake

## Features

### Rendering
- **Forward rendering** pipeline
- **PBR (Cook-Torrance BRDF)** — metallic-roughness workflow with GGX distribution, correlated Smith-GGX visibility, Schlick Fresnel
- **Image Based Lighting (IBL)** — irradiance maps, prefiltered environment maps, BRDF LUT (split-sum)
- **GPU Driven Rendering** with bindless textures
- **Render Graph**
- **HBAO**
- **FXAA**
- **Directional shadows** with PCF 3×3 filtering

### Architecture
- **ECS core** — POD components, explicit system subscriptions, zero overhead
- **VMA** — Vulkan Memory Allocator for GPU memory management

### Assets & Tooling
- **glTF 2.0** model loading
- **Dear ImGui** debug UI

## Getting Started

### Prerequisites

| Tool | Notes |
|------|-------|
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | Requires **Desktop development with C++** workload |
| [CMake](https://cmake.org/) ≥ 3.20 | |
| [Ninja](https://ninja-build.org/) | Or install via Visual Studio Installer |
| [vcpkg](https://github.com/microsoft/vcpkg) | Set `VCPKG_ROOT` environment variable after installing |
| [Vulkan SDK](https://vulkan.lunarg.com/) | Includes `slangc` — no separate Slang install needed |

### Steps
```bash
git clone https://github.com/Miurg/Halcyon.git
cd Halcyon

# Install dependencies
vcpkg install

# Configure (vcpkg dependencies are fetched automatically)
cmake --preset x64-debug

# Build
cmake --build out/build/x64-debug
```

For release build replace `x64-debug` with `x64-release`.

> **Note:** The debug preset builds single-threaded for cleaner error output.

## 🎯 Project Goals

The main goal of the project is to create a forward rendering engine capable of displaying excellent images without temporal techniques.
