# simple3d

A small, modern C++ 3D rendering engine built on **OpenGL 3.3 Core**. It is a
learning-oriented but feature-rich renderer with physically based rendering
(PBR), image-based lighting (IBL), shadow mapping, a skybox, and a composable
material system with pluggable lighting models.
<img width="802" height="604" alt="image" src="https://github.com/user-attachments/assets/02893c81-22b0-4bc9-9741-4b71277d1d7e" />

## Features

- **Pluggable lighting models** — lighting is decoupled from the material via an
  `ILightingModel` interface. Switching shading models is a one-liner:
  ```cpp
  mat->SetLightingModel(std::make_unique<PBRLightingModel>());
  ```
  Bundled models: `PBRLightingModel`, `PhongLightingModel`, `UnlitLightingModel`.
- **Physically based rendering (PBR)** — metallic/roughness workflow with
  base color, normal, metallic, roughness and AO maps.
- **Image-based lighting (IBL)** — irradiance, prefiltered specular and BRDF LUT
  baked at runtime from an HDR equirectangular environment map.
- **Composable material filters** — materials build their fragment shader from
  independent `Filter3D` slots (color, texture, normal map, …) that are injected
  into the shader at build time.
- **Directional shadow mapping** — configurable shadow-map resolution and area.
- **HDR skybox** — equirectangular `.hdr` files converted to a cubemap.
- **Model loading** via Assimp — supports FBX, OBJ, DAE and other formats,
  including skinned meshes (`SkinnedMaterial3D`).
- **Object picking** — GPU-based object selection (`ObjectSelector` /
  `ObjectIdMaterial`).
- **Cameras & input** — first-person and free-look cameras with an event-driven
  keyboard/mouse input layer (GLFW backend).

## Tech stack

- **Language:** C++20
- **Graphics API:** OpenGL 3.3 Core
- **Build system:** CMake (>= 3.15)
- **Dependencies:**
  - [GLFW](https://www.glfw.org/) — windowing and input (via vcpkg)
  - [Assimp](https://github.com/assimp/assimp) — model import (via vcpkg)
  - [GLEW](http://glew.sourceforge.net/) — OpenGL extension loading (bundled in `include/`)
  - [GLM](https://github.com/g-truc/glm) — math (header-only, bundled in `include/glm`)
  - [SOIL2](https://github.com/SpartanJ/SOIL2) — image loading (bundled in `libs/SOIL`)

## Project layout

```
simple3d/
├── src/
│   ├── camera/        # First-person and free-look cameras
│   ├── events/        # Event dispatcher and listener interfaces
│   ├── input/         # Keyboard/mouse abstractions + GLFW backends
│   ├── lighting/      # ILightingModel: PBR, Phong, Unlit
│   ├── materials/     # Material3D, filters, shader factory
│   │   └── filters/   # Composable shader filters (color, texture, normal map)
│   ├── models/        # Box, plane, mesh and external (imported) models
│   ├── object_selector/
│   ├── render/        # Skybox, IBL baker, shadow map, GL buffer wrappers
│   ├── resources/     # Texture/cubemap/HDR loaders, texture manager
│   ├── utils/
│   ├── Engine.*       # Engine singleton (timing, subsystems)
│   └── Scene3D.*      # Scene graph root, lighting, environment, shadows
├── assets/            # Shaders, models, materials, HDR environments
├── include/           # Bundled GLEW + GLM headers
├── libs/SOIL/         # Bundled SOIL2 library and DLLs
├── cmakeModules/      # Find* scripts for SOIL/Assimp/GLFW/GLEW/GLM
├── main.cpp           # Demo entry point
└── CMakeLists.txt
```

## Getting started

### Prerequisites

- A C++20-capable compiler (MSVC, Clang, or GCC)
- CMake 3.15 or newer
- [vcpkg](https://github.com/microsoft/vcpkg) for GLFW and Assimp
- A GPU/driver supporting OpenGL 3.3 Core

### Install dependencies (vcpkg)

```bash
vcpkg install glfw3 assimp
```

### Configure & build

GLFW and Assimp are resolved through the vcpkg toolchain file. Point CMake at it
when configuring:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

The executable is written next to the build directory so it can find the
`assets/` folder (the demo loads assets via `../assets/...` relative paths) and
the SOIL2 DLL is copied beside the binary automatically on Windows.

### Run

```bash
./build/simple3d
```

The demo loads an HDR environment, builds IBL, enables shadows, and renders a
car and a crate model on a textured ground plane.

## How it works

- **`Scene3D`** is the scene-graph root. It owns the directional light, the IBL
  environment (`SceneEnvironment`), the skybox and the shadow map, and drives the
  render pass.
- **`Material3D`** owns a list of `Filter3D` slots plus exactly one
  `ILightingModel`. At build time it assembles the fragment shader by injecting
  filter code into shader markers and appending the lighting model's GLSL.
- **`ILightingModel`** is intentionally *not* a `Filter3D`: filters are
  independent modulators that write into a slot (`BASE_COLOR`, `SPEC_STRENGTH`,
  `N`, `EMISSIVE`), while the lighting model is the single consumer that reads
  every slot and produces the final color. See `DEVELOP_PLAN.md` for the design
  rationale.

## Documentation

- `DEVELOP_PLAN.md` — roadmap and design notes.
- `REFACTORING_PLAN.md` — refactoring notes.
