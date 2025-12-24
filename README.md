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

## 🚀 Getting Started

### Prerequisites
* C++ Compiler (supporting C++17 or newer)
* CMake (3.20 or newer)
* Vulkan SDK installed on your machine

### Installation

1.  Clone the repository:
    ```bash
    git clone [https://github.com/Miurg/Halcyon.git](https://github.com/Miurg/Halcyon.git)
    cd Halcyon
    ```

2.  Build the project (out-of-source build is recommended):
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```

3.  Run the engine:
    * executable will be located in the `out` directory.

## 🎯 Project Goals

The primary goal of Halcyon is to explore advanced graphics programming and engine architecture.
* Mastering **Vulkan** API nuances.
* Refining a high-performance **ECS** with explicit control flow.
* Creating a robust foundation for future game prototypes.

---
*Note: This project is a work in progress.*
