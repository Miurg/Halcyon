# Halcyon

**Halcyon** is a custom game engine currently in early development. It is built from scratch using **C++** and **Vulkan**, featuring a bespoke Entity Component System (ECS) architecture designed for high performance and strict data control.

## 🛠 Tech Stack

* **Language:** C++
* **Graphics API:** Vulkan
* **Build System:** CMake
* **Architecture:** Custom ECS

## ✨ Key Features

### 1. Custom ECS Architecture
Unlike generic ECS implementations, Halcyon uses a strict data-oriented approach:
* **Explicit System Subscription:** Systems explicitly subscribe to relevant component combinations, avoiding implicit overhead.

### 2. Vulkan Rendering
The engine has transitioned from OpenGL to Vulkan to leverage modern GPU capabilities and explicit resource management.

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

The primary goal of Halcyon is to explore advanced graphics programming and engine architecture.
* Mastering **Vulkan** API nuances.
* Refining a high-performance **ECS** with explicit control flow.
* Creating a robust foundation for future game prototypes.

---
*Note: This project is a work in progress.*
