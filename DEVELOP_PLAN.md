# simple3d Development Plan

This document describes the order in which to grow the engine after the base
refactoring is complete (see `REFACTORING_PLAN.md`).

Key idea: **separate the "lighting model" (Phong / PBR / Unlit) from the**
**material itself.** Right now lighting is hard-wired into the
`defaultColorLight.fs` shader and into `Material3D::Bind`; we'll extract it into
a separate `ILightingModel` abstraction that the material holds as exactly one
instance. Then switching to PBR becomes a one-line change:

```cpp
mat->SetLightingModel(std::make_unique<PBRLightingModel>());
```

Task status legend: `[ ]` — not done, `[~]` — in progress, `[x]` — done.

---

## Stage 1. Introducing `ILightingModel`

Goal: extract the `Light`/`Material` structs, the `viewPos`/`light.*`/
`material.shininess` uniforms and the lighting formula itself out of the shader +
`Material3D::Bind` into a swappable entity.

### How it differs from the existing `Filter3D`

| | `Filter3D` (modulator) | `ILightingModel` (synthesizer) |
|---|---|---|
| Produces | a value into its own slot (`BASE_COLOR` / `SPEC_STRENGTH` / `N` / `EMISSIVE`) | the final `color` |
| Dependencies | none — each filter is independent | reads the results of **all** filters |
| How many per material | 0..N | exactly one |
| Access to `RenderContext` | not needed | needed (light/viewPos come from the scene) |
| Position in `main` | in one of the 5 slot markers | strictly after all slot filters |
| `Bind` signature | `Bind(program, firstTextureUnit)` | `Bind(program, firstTextureUnit, ctx)` |

This is **a different entity by contract**, therefore:

1. `ILightingModel` **does not inherit** from `Filter3D` and is not stored in `_filters`.
2. The `Bind` signatures are intentionally different — a filter doesn't need
   `RenderContext`, and extending its API for the sake of a single consumer is
   the "one common interface for everything" anti-pattern. A filter should stay
   simple.
3. `Material3D::Bind` binds them **sequentially**, but via two different
   calls — see item 1.3.

### Substages

- [x] **1.1. Declare `ILightingModel`.** A standalone interface,
  **not** tied to `Filter3D` through inheritance or a common base type.
  ```cpp
  // src/materials/lighting/ILightingModel.h
  class ILightingModel {
  public:
      virtual ~ILightingModel() = default;

      // GLSL: struct Light, struct Material, uniform Light light; uniform Material material;
      // plus the in-variables the model expects from the vertex shader (FragPos, Normal, TBN).
      virtual std::string GetDeclarations() const = 0;

      // GLSL: code that consumes BASE_COLOR/SPEC_STRENGTH/N/EMISSIVE
      // and writes the final color.
      virtual std::string GetLightingCode() const = 0;

      // Binding of uniforms and textures on every draw.
      // firstTextureUnit — the starting unit allocated by the material from the
      // shared counter (after all Filter3D::Bind calls).
      virtual void Bind(GLuint program, GLuint firstTextureUnit, const RenderContext& ctx) = 0;

      // How many texture units are needed (for PBR — IBL maps + BRDF LUT).
      // Symmetric to Filter3D::GetUniformsCount(), but it's a separate method
      // of a separate interface — there is deliberately no common base type.
      virtual unsigned int GetTextureUnitCount() const { return 0; }
  };
  ```
  - File: `src/materials/lighting/ILightingModel.h`.

- [x] **1.2. Markers in `defaultColorLight.fs`.** Replace the inline lighting
  with 2 markers that are substituted during `Material3D::Build()`.
  ```glsl
  // Delete the current "uniform vec3 viewPos; uniform Material material; uniform Light light; ..." block.
  // __LIGHTING_DECLS__

  void main()
  {
      vec3 BASE_COLOR    = uBaseColor.rgb;
      vec3 SPEC_STRENGTH = vec3(1.0);
      vec3 N             = normalize(Normal);
      vec3 EMISSIVE      = vec3(0.0);

      // __APPLY_BASE_COLOR_FILTERS__
      // __APPLY_SPECULAR_FILTERS__
      // __APPLY_NORMAL_FILTERS__

      // __APPLY_LIGHTING__   ← LightingModel::GetLightingCode() ends with an assignment to color
  }
  ```
  - File: `assets/shaders/defaultColorLight.fs`.

- [x] **1.3. `Material3D` owns a `unique_ptr<ILightingModel>`.**
  ```cpp
  class Material3D : public MaterialBase {
      std::unique_ptr<ILightingModel> _lighting;   // separate from _filters
  public:
      void SetLightingModel(std::unique_ptr<ILightingModel> model);
  };
  ```
  - In `Material3D::BuildFragmentShader()`, after `InjectFilters`, substitute
    `__LIGHTING_DECLS__` and `__APPLY_LIGHTING__` from `_lighting`.
  - In `Material3D::Bind`, bind the filters and the lighting **sequentially, but**
    **via two different calls with different contracts** — this is the very
    "semantic cleanliness" for which `ILightingModel` is not a subclass of
    `Filter3D`:
    ```cpp
    GLuint nextUnit = 0;

    // 1) Filters — narrow contract without ctx
    for (const auto& f : _filters) {
        f->Bind(_shader->GetProgram(), nextUnit);
        nextUnit += f->GetUniformsCount();
    }

    // 2) Lighting — extended contract with ctx
    if (_lighting) {
        _lighting->Bind(_shader->GetProgram(), nextUnit, ctx);
        nextUnit += _lighting->GetTextureUnitCount();
    }
    ```
  - If `_lighting == nullptr` at `Build()` time — throw
    `std::runtime_error("Material3D has no lighting model")`. Or, in the
    `Material3D` ctor, set `PhongLightingModel` by default (decide during
    implementation).
  - Files: `src/materials/Material3D.h/.cpp`.

- [x] **1.4. `Filter3D::Bind` stays unchanged.** This is an
  explicit record of a design decision, not a task. The temptation to "just make
  a single `Bind(program, firstTextureUnit, ctx)` for everything" must be
  rejected up front:
  - the filters (`TextureMapFilter`, `ColorFilter`, `NormalMapFilter`) don't use
    `ctx` — extending their API for one consumer violates Interface Segregation;
  - a single contract erases the difference between "input modulator" and
    "lighting model", i.e. breaks the very semantic boundary for which we're
    introducing `ILightingModel`;
  - if tomorrow a filter appears that needs `ctx` (e.g. a "depth fog" filter that
    reads `ctx.camera`) — that's a signal that it's actually not a filter but
    another top-level abstraction like PostProcess. Not a reason to extend
    `Filter3D::Bind`.

---

## Stage 2. Base implementations of `ILightingModel`

- [x] **2.1. `PhongLightingModel`** — migration of the existing logic.
  - `GetDeclarations()` returns the current block:
    ```glsl
    struct Material { float shininess; };
    struct Light { vec3 position; vec3 ambient; vec3 diffuse; vec3 specular; };
    uniform vec3 viewPos;
    uniform Material material;
    uniform Light light;
    ```
  - `GetLightingCode()` returns what is currently in `main` after the filter
    markers — the ambient/diffuse/specular computation and the final `color = ...`.
  - `Bind(prog, ctx)` sets `viewPos`, `light.position`/`ambient`/`diffuse`/
    `specular`, `material.shininess`. This is the code currently in
    `Material3D::Bind` (lines ~110–135) — it moves here verbatim.
  - Files: `src/materials/lighting/PhongLightingModel.h/.cpp`.

- [x] **2.2. `UnlitLightingModel`** — for light sources and UI materials.
  - `GetDeclarations()` — empty.
  - `GetLightingCode()` — `color = vec4(BASE_COLOR + EMISSIVE, 1.0);`
    (ignores light, uses base color and emission).
  - `Bind(prog, ctx)` — empty.
  - **Bonus:** lets us remove the separate `light_source_shader.vs/.fs` from
    `assets/shaders/` — the lamp in `Scene3D::initLightView` will use the same
    `defaultColorLight.fs` template, just with the unlit model.
  - Files: `src/materials/lighting/UnlitLightingModel.h/.cpp`,
    `src/Scene3D.cpp`.

- [x] **2.3. Migrate the consumers.** Everywhere a `Material3D` is currently
  created, add `SetLightingModel(make_unique<PhongLightingModel>())`
  (or make it the default in the ctor, see 1.3).
  - Files: `src/models/BoxModel.cpp`, `src/models/ExternalModel.cpp`,
    `src/Scene3D.cpp`.

- [x] **2.4. Clean up `defaultColorLight.fs`.** After 1.2 + 2.1 the file should
  contain only the vertex inputs (`FragPos`, `Normal`, `TexCoords`, `TBN`),
  `uBaseColor`, and the `main()` skeleton with the slot markers. No lighting,
  no `Light`/`Material` structs, no `viewPos`.
  - This shader becomes the "universal lit template"; it could be renamed, for
    example, to `default_material.fs` (but that's already style).

---

## Stage 3. Preparing the infrastructure for PBR

Goal: extend the filter slots for PBR inputs **without** touching
`ILightingModel`. Phong still works; it simply ignores the new slots.

- [x] **3.1. New `FilterSlot`s.** Add to `Filter3D::FilterSlot`:
  - `Metallic` — a `float` channel, mapped to the `M` variable in the shader.
  - `Roughness` — a `float` channel, mapped to `R`.
  - `AO` (ambient occlusion) — a `float` channel, mapped to `AO`.
  - The `defaultColorLight.fs` shader declares defaults:
    ```glsl
    float M  = 0.0;
    float R  = 0.5;
    float AO = 1.0;
    ```
    plus new markers:
    ```glsl
    // __APPLY_METALLIC_FILTERS__
    // __APPLY_ROUGHNESS_FILTERS__
    // __APPLY_AO_FILTERS__
    ```
  - `kSlotTarget` and `kSlotMarkers` in `Material3D::InjectFilters` get the new
    entries added.
  - Files: `src/materials/filters/Filter3d.h`,
    `src/materials/Material3D.cpp`,
    `assets/shaders/defaultColorLight.fs`.

- [x] **3.2. Filters with a `float` result.** Currently all filters return
  `vec3`/`vec4`. For `Metallic`/`Roughness`/`AO` the target is a `float`. Extend
  `Filter3D::ResultType` with a `FLOAT` value, and in `InjectFilters` pick the
  swizzle (`.r` for a `float` target from a `vec4` filter).
  - Alternative: introduce a specialized `TextureChannelFilter` that returns a
    `float` directly.
  - Files: `src/materials/filters/Filter3d.h`, `src/materials/Material3D.cpp`.
  - Implemented via the `.r` swizzle in `InjectFilters`. NB: `ResultType::FLOAT`
    was added to the enum but isn't actually used (the swizzle variant won) — it
    can be removed as dead code.

- [x] **3.3. Mapping Assimp types onto the new slots.** In
  `ExternalModel::ProcessMesh` add reading of:
  - `aiTextureType_METALNESS` → `FilterSlot::Metallic`
  - `aiTextureType_DIFFUSE_ROUGHNESS` → `FilterSlot::Roughness`
  - `aiTextureType_AMBIENT_OCCLUSION` → `FilterSlot::AO`
  - With the same fallback to legacy types we currently use for
    `aiTextureType_NORMALS` ↔ `aiTextureType_HEIGHT` for obj.
  - The backpack has these maps (`roughness.jpg`, `ao.jpg`), but they're mapped
    through non-standard names — needs checking.
  - Files: `src/models/ExternalModel.cpp`,
    `assets/models/backpack/backpack.mtl` (possibly).

---

## Stage 4. `PBRLightingModel` (Cook-Torrance)

Goal: implement a physically correct lighting model that reads the new
`Metallic`/`Roughness`/`AO` slots.

- [x] **4.1. PBR math.** `GetLightingCode()` returns the classic
  Cook-Torrance BRDF:
  - GGX/Trowbridge-Reitz normal distribution function
  - Schlick fresnel
  - Smith geometry
  - F0 = mix(vec3(0.04), BASE_COLOR, M)
  - diffuse = (1 - F) * (1 - M) * BASE_COLOR / π
  - specular = (D * F * G) / (4 * NdotV * NdotL)
  - color = (kD * diffuse + specular) * radiance * NdotL + ambient * AO
  - File: `src/materials/lighting/PBRLightingModel.h/.cpp`.

- [x] **4.2. Direct lights only.** In the first step we account for only one
  point light from `Scene3D` (the same as Phong does now). IBL is the next
  task.

- [x] **4.3. Test scene.** In `main.cpp` add a flag (or a second object) that
  uses `PBRLightingModel`. Compare it visually with the Phong version of the
  same backpack — on the buckles there should be a noticeable difference in the
  speculars (Phong gives a soft highlight, PBR gives a more focused one that
  accounts for the roughness map).
  - File: `main.cpp`.

- [ ] **4.4. Directional light (sun) for PBR.** Currently `PBRLightingModel`
  uses the same point light (`light.position`) as Phong. Introduce a separate
  directional source and use it as the main direct light in PBR. Don't touch
  Phong — it keeps working on the old point light.

  - **4.4.1. New source in `Scene3D`.** Add a directional light as a
    separate entity (without breaking the existing `_lightPosition/_lightAmbient/...`):
    ```cpp
    glm::vec3 _dirLightDirection { -0.5f, -1.0f, -0.3f }; // where it shines (from sun toward scene)
    glm::vec3 _dirLightColor     {  1.0f,  0.96f, 0.90f }; // sun tint, normalized
    float     _dirLightIntensity =  3.0f;                  // compensates for the division by π
    ```
    plus getters/setters by analogy with the current ones.
    - Files: `src/Scene3D.h`, `src/Scene3D.cpp`.

  - **4.4.2. `PBRLightingModel` reads the directional light.**
    - `GetDeclarations()`: add
      ```glsl
      struct DirLight { vec3 direction; vec3 color; };
      uniform DirLight dirLight;
      ```
    - `GetLightingCode()`: replace the `L`/`radiance` computation with directional:
      ```glsl
      vec3 L = normalize(-dirLight.direction); // no attenuation
      vec3 radiance = dirLight.color;
      ```
      The rest of Cook-Torrance (D/F/G, `kD`, ambient, tone map) — unchanged.
    - `Bind()`: set `dirLight.direction` and `dirLight.color`
      (`= color * intensity`) from `ctx.scene3D`. Remove the `light.position`
      read if it's no longer used in the PBR code.
    - **Direction convention:** store the "where it shines" vector, and in the
      shader invert it (`-direction`) to get the "toward the source" vector.
    - File: `src/materials/lighting/PBRLightingModel.h/.cpp`.

  - **4.4.3. Setup in the scene.** In `main.cpp`/scene initialization set the
    sun's direction and tint (`setDirLightDirection(...)`, `setDirLightColor(...)`).
    - File: `main.cpp`.

  - **Notes:**
    - A directional light has no position and no attenuation (`1/d²` is not
      applied) — that's what distinguishes it from a point light.
    - Brightness is controlled by `intensity` (because of the `/π` direct light
      would otherwise be dim), not by cranking up the color channels — the tint
      stays normalized.
    - Ambient stays flat (`light.ambient ≈ 0.03`) until IBL arrives (Stage 5).
    - This refines/replaces item 4.2: "direct lights only" now = a single
      directional source instead of a point one.

---

## Stage 5. IBL — image-based lighting (optional)

This is the transition from "technically PBR without an environment" to "PBR in
a studio".

- [x] **5.1. `SceneEnvironment` — a separate scene entity.**
  ```cpp
  class SceneEnvironment {
      std::shared_ptr<TextureCube> irradiance;        // diffuse IBL
      std::shared_ptr<TextureCube> prefilteredSpec;   // specular IBL
      std::shared_ptr<Texture2D>   brdfLUT;
  };
  ```
  Stored in `Scene3D`, exposed via `ctx.scene3D->getEnvironment()`.
  - Files: `src/render/SceneEnvironment.h`, `src/Scene3D.h/.cpp`,
    `src/render/RenderContext.h`.

- [x] **5.2. `TextureCube` class.** Currently there's only `Texture2D`.
  - Files: `src/resources/TextureCube.h/.cpp`.
  - Made a RAII class (move-only, like `GLBuffer`/`GLVertexArray`), with
    `Bind(unit)`/`Unbind()` and a `CreateEmpty(...)` factory for an empty cubemap
    used as a render target / IBL.

- [x] **5.3. HDR equirectangular → cubemap loader.** A single `.hdr` file
  → cubemap via a one-time render pass.
  - File: `src/resources/HDRLoader.cpp` (`HDRLoader::EquirectFileToCubemap`).
  - The loader is self-contained (its own RGBE parser `32-bit_rle_rgbe`), with no
    external dependencies: SOIL is a prebuilt DLL and doesn't export stb symbols,
    and its `stb_image.h` is heavily customized. **`.exr` is not supported**
    (would need tinyexr — a separate task if needed).
  - Bake: equirect → `GL_RGB16F` 2D texture → render a unit cube 6× with a
    90° projection into the cubemap faces via an FBO; GL state
    (viewport/FBO/cull) is saved and restored.

- [x] **5.4. Precompute irradiance + prefiltered specular + BRDF LUT.**
  These three cubemaps are computed once when the environment is loaded.
  - Algorithm: see [LearnOpenGL/PBR/IBL](https://learnopengl.com/PBR/IBL/Diffuse-irradiance).
  - Files: `src/render/IBLBaker.h/.cpp`.
  - `IBLBaker::BakeIrradiance(env, 32)` — convolution over the hemisphere (diffuse IBL).
  - `IBLBaker::BakePrefiltered(env, 128, 5)` — GGX importance sampling, roughness
    over mip levels; before baking, mips are generated on env (for sampling quality).
  - `IBLBaker::BakeBRDFLUT(512)` — an RG16F LUT (scale/bias for F0), independent of the environment.
  - The shaders are inline in `IBLBaker.cpp`; rendering into cubemap faces / a quad
    via an FBO, GL state is saved via `GLStateGuard`.
  - **Not yet wired into `Scene3D`/`PBRLightingModel`** — that's item 5.5.

- [x] **5.5. `PBRLightingModel` uses IBL.** In `Bind()` it binds
  irradiance, prefilteredSpec, brdfLUT from `ctx.scene3D->GetEnvironment()`
  onto texture units allocated via `GetTextureUnitCount() == 3`.
  In `GetLightingCode()` ambient is added via IBL instead of the flat
  `light.ambient`.
  - `SceneEnvironment` got `Set/IsValid/Irradiance/PrefilteredSpec/BrdfLUT`,
    `Scene3D::SetEnvironment(...)` + `GetEnvironment()`.
  - Shader: `uHasIBL`, `irradianceMap`/`prefilterMap`/`brdfLUT`,
    `FresnelSchlickRoughness`; ambient = `kD*irradiance*albedo + prefiltered*(F*brdf.x+brdf.y)`,
    with a fallback to the flat `ambientLight` when there's no environment.
  - Baking the maps and `SetEnvironment` — in `main.cpp` right after loading the cubemap.

- [x] **5.6. Skybox.** Drawing the cubemap as the scene background — a separate
  vertex+fragment shader. This is not part of `ILightingModel`; it's a separate
  pass in `Scene3D::Render`.
  - Files: `src/render/Skybox.h/.cpp`, `assets/shaders/skybox.vsh/.fsh`.
  - `Scene3D::SetSkybox(cubemap)` creates a `Skybox`; it's drawn last with
    `glDepthFunc(GL_LEQUAL)` and the `gl_Position = pos.xyww` trick (depth = 1.0),
    view without translation. Skipped in the id-pass (`material != nullptr`).
    The linear HDR from the cubemap is tone-mapped with the same Reinhard+gamma as PBR.

---

## Stage 6. Splitting `Mesh` into `Geometry` + `Mesh-instance`

**Independent of stages 1–5.** Can be done in parallel (after
`REFACTORING_PLAN 5.9` — it's a prerequisite).

### Problem

Currently `Mesh` plays two incompatible roles at once:

1. **Scene node** — `is-a Pivot3D`, has position/rotation/scale, parent,
   children.
2. **GPU resource** — owns VAO/VBO/EBO.

Because of this:

- **Geometry is duplicated in VRAM.** 100 identical box models →
  100 copies of the box VBO. Each `BoxModel::processMesh` does its own
  `glGenBuffers` and `glBufferData`.
- **A `shared_ptr<Mesh>` can't be shared as "the same geometry".**
  If you put one `shared_ptr<Mesh>` in two places in the scene, they'll
  share a single transform — i.e. that's "one object rendered twice in the
  same spot", not "two instances at different positions".

The standard solution (Unity: `Mesh` + `MeshRenderer`; Unreal:
`UStaticMesh` + `UStaticMeshComponent`) is to split the roles into two types.

### Substages

- [x] **6.1. `Geometry` class** — an immutable owner of GL resources, with no
  transform and no hierarchy. Shared via `std::shared_ptr<Geometry>`.
  ```cpp
  // src/render/Geometry.h
  class Geometry {
      GLVertexArray _vao;
      GLBuffer _vbo;
      GLBuffer _ebo;
      std::vector<GLBuffer> _secondaryVbos;   // for skinning etc.
      GLsizei _indicesCount = 0;
  public:
      // Constructed once with a layout + raw data.
      template <typename V>
      Geometry(const VertexLayout& layout,
               const std::vector<V>& vertices,
               const std::vector<GLuint>& indices);

      void AddSecondaryBuffer(const VertexLayout& layout,
                              std::span<const std::byte> data);

      // Move-only automatically (thanks to GLBuffer/GLVertexArray).
      Geometry(Geometry&&) = default;
      Geometry& operator=(Geometry&&) = default;

      void Draw() const {
          _vao.Bind();
          glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_INT, nullptr);
          GLVertexArray::Unbind();
      }

      GLsizei IndicesCount() const { return _indicesCount; }
  };
  ```
  - Extracted from the current `Mesh` — takes everything GL-related.
  - File: `src/render/Geometry.h/.cpp`.

- [x] **6.2. `Mesh` becomes a lightweight scene node.**
  ```cpp
  // src/models/Mesh.h
  class Mesh : public Pivot3D {
      std::shared_ptr<Geometry>    _geometry;  // ← shared
      std::shared_ptr<MaterialBase> _material; // ← can also be shared
  public:
      Mesh(std::shared_ptr<Geometry> geom, std::shared_ptr<MaterialBase> mat);

      void Render(const RenderContext& ctx, MaterialBase* material) override {
          RenderContext context = ctx;
          context.model = ctx.model * LocalMatrix();
          if (!material) material = _material.get();

          if (_geometry->IndicesCount() > 0) {
              material->Bind(context, this);
              _geometry->Draw();
              material->Unbind();
          }
          for (auto& child : _children) {
              child->Render(context, material);
          }
      }
  };
  ```
  - The size of `Mesh` minus Pivot3D is three pointers. On a scene of thousands
    of objects that's negligible RAM.
  - `SetupMesh` disappears — its work moves into `Geometry::Geometry`.
  - Files: `src/models/Mesh.h/.cpp`.

- [x] **6.3. `GeometryRegistry` — a cache via `weak_ptr`.**
  ```cpp
  // src/render/GeometryRegistry.h
  class GeometryRegistry {
      std::map<std::string, std::weak_ptr<Geometry>, std::less<>> _cache;
  public:
      template <typename Factory>
      std::shared_ptr<Geometry> GetOrCreate(std::string_view key, Factory&& factory) {
          if (auto it = _cache.find(key); it != _cache.end()) {
              if (auto alive = it->second.lock()) return alive;
          }
          auto geometry = std::make_shared<Geometry>(factory());
          _cache[std::string(key)] = geometry;  // weak copy
          return geometry;
      }

      void Cleanup() { _cache.clear(); }
  };
  ```
  - **`weak_ptr`, not `shared_ptr`** — important: if all Mesh instances that
    used a given Geometry are destroyed, and the Registry only holds a
    `weak_ptr`, the real Geometry is deleted automatically. VRAM is freed
    without an explicit call. If instances exist — we reuse it.
  - Stored in `Engine` next to `TextureManager` and `ShaderFactory`.
  - `Engine::Cleanup()` calls `_geometryRegistry.Cleanup()` before
    `glfwTerminate`.
  - Files: `src/render/GeometryRegistry.h/.cpp`, `src/Engine.h/.cpp`.

- [x] **6.4. Migrate `BoxModel`.** (Сделано вместе с 6.2 — смена API `Mesh` этого
  требует.) Также мигрирован `PlaneModel` (ключ кэша включает `tiling`, т.к. UV
  зависят от него). The box geometry becomes a singleton
  through the Registry.
  ```cpp
  std::shared_ptr<Mesh> BoxModel::processMesh()
  {
      auto mat = MakeBoxMaterial(_color);   // as now

      auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
          "primitive:box",
          [] { return Geometry(VertexLayouts::Standard(), boxVertices, boxIndices); });

      return std::make_shared<Mesh>(geometry, mat);
  }
  ```
  - 100 box models → one box VBO in VRAM, 100 materials (different colors),
    100 lightweight Mesh instances with different transforms.
  - File: `src/models/BoxModel.cpp`.

- [x] **6.5. Migrate `ExternalModel`.** (Сделано вместе с 6.2.) The cache key is the file path +
  the mesh index (a single .obj/.fbx can contain several mesh parts).
  ```cpp
  std::string geometryKey = _path + "#" + std::to_string(mesh->mMaterialIndex)
                                + "#" + std::string(mesh->mName.C_Str());

  auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
      geometryKey,
      [&] {
          Geometry g(VertexLayouts::Standard(), vertices, indices);
          if (!bones.empty()) {
              g.AddSecondaryBuffer(VertexLayouts::Skinning(),
                                   std::as_bytes(std::span(bones)));
          }
          return g;
      });

  return std::make_shared<Mesh>(geometry, material);
  ```
  - Loading the same `backpack.obj` twice with different `ExternalModel`s
    → Assimp still parses the file twice (this could also be cached at the
    `aiScene` level as a separate task), but **there's exactly one VBO in
    VRAM**.
  - File: `src/models/ExternalModel.cpp`.

### What is NOT done in this stage

- **GPU instancing** (`glDrawElementsInstanced` + a per-instance VBO with
  transforms). This is a separate optimization that saves **draw calls**, not
  VRAM. After 6.x the geometry is already a separate entity — instancing will
  sit on top without rework.
- **`aiScene` cache** (identical `.obj` files are parsed again). This is a
  separate task at the asset-loading level.
- **Material cache.** A material can already be shared via
  `shared_ptr<MaterialBase>` (no structure duplicates it), but currently each
  `BoxModel`/`ExternalModel` creates its own. Building a `MaterialRegistry` is a
  separate story, not a blocker.

---

## Stage 7. Cast shadows (directional shadow mapping)

Goal: objects (the car, the crate) cast shadows on the floor from the
**directional sun** (`dirLight`, introduced in Stage 4.4). Point/Phong light and
omni shadows (cube shadow map) are out of scope; the focus is a single
directional source for PBR.

### What already exists and is reused
- **FBO + offscreen render** — the pattern is established in
  `HDRLoader`/`IBLBaker` (`GLStateGuard`, save/restore viewport+FBO).
- **Per-pass override material** — `Pivot3D::Render(ctx, material)` already knows
  how to render the whole hierarchy with one material (this is how the id-pass of
  `ObjectSelector` works with `ObjectIdMaterial`). The depth pass is the same
  technique.
- **World fragment position** — `FragPos` (world) already goes to the fragment
  shader, so `fragPosLightSpace` can be computed in the FS without touching the
  vertex shaders.

### Substages

- [x] **7.1. `ShadowMap` class.** Owns a depth FBO + a depth texture
  (`GL_DEPTH_COMPONENT24`, `GL_NEAREST`, `GL_CLAMP_TO_BORDER` with border=1.0,
  so there's no shadow outside the map). `Begin()` binds the FBO + sets the
  viewport to the map's resolution (default 2048²) and clears depth; `End()`
  restores the previous FBO/viewport. `glDrawBuffer/glReadBuffer = NONE`
  (depth-only completeness).
  - Files: `src/render/ShadowMap.h/.cpp`. **Done.**
  - Node flags: `Pivot3D` got `_castShadows`/`_receiveShadows`
    (default true) + getters/setters. `castShadows` will be checked in the
    depth pass (7.4), `receiveShadows` will be passed through to PBR (7.5).

- [x] **7.2. Light-space matrix.** `ShadowMap::ComputeLightSpaceMatrix(dir, center, radius)`:
  an ortho cube `2*radius` + `lookAt` from `center - dir*radius` along `dirLight.direction`,
  depth `[0, 2*radius]`, with `up` protection when the light points straight down. **Done.**
  For a directional light — an orthographic projection + `lookAt` along `dirLight.direction`:
  ```cpp
  glm::mat4 lightView = glm::lookAt(-dir * d, vec3(0), up);
  glm::mat4 lightProj = glm::ortho(-S, S, -S, S, near, far);
  lightSpaceMatrix = lightProj * lightView;
  ```
  The first variant is a fixed cube around the origin (S, d, near/far are
  parameters). Fitting it to the camera frustum / CSM is a separate future task.
  - Method `ShadowMap::ComputeLightSpace(const glm::vec3& dir, ...)`.

- [x] **7.3. Depth material and pass.** `DepthMaterial : MaterialBase`
  (a thin subclass) + the `depth.vsh`/`depth.fsh` shaders. **Variant 1**: depth.vsh
  uses the ordinary `model/view/projection` that the base `MaterialBase::Bind`
  sets anyway; in the shadow pass `view/projection` are the light matrices via
  `RenderContext` (no changes to `MaterialBase`). The fsh is empty (we only write
  depth). The vsh takes only `location 0` (position) — extra VAO attributes are
  ignored. **Done.**
  - **Skinned meshes (TODO)**: a skeletal depth-shader variant is needed (like
    `shader_skin.vsh`), otherwise animated models will cast a shadow in the bind
    pose. The first step is static geometry.
  - Files: `src/materials/DepthMaterial.h/.cpp`,
    `assets/shaders/depth.vsh`, `assets/shaders/depth.fsh`.

- [x] **7.4. Integration into `Scene3D::Render`.** **Done.** `Scene3D::EnableShadows(size, radius)`
  creates a `ShadowMap` + `DepthMaterial`. In `Render` (only when `material==nullptr`):
  a shadow pass with `shadowCtx.view=I`, `projection=lightSpace`, `shadowPass=true`, then
  the main pass receives `lightSpaceMatrix`/`shadowMap`/`hasShadows` in the `RenderContext`.
  `_castShadows` is checked in `Pivot3D::Render` and `Mesh::Render` (subtree skipped in the shadow pass).
  Frame order:
  1. shadow pass: `_shadowMap->Begin()`; `RenderContext shadowCtx` with
     `view=lightView`, `projection=lightProj`; `Pivot3D::Render(shadowCtx, depthMaterial)`;
     `_shadowMap->End()`.
  2. main pass: as now, but `glm::mat4 lightSpaceMatrix` and `GLuint shadowMapTex`
     (+ a `hasShadows` flag) are added to the `RenderContext`.
  - `Scene3D` owns a `std::shared_ptr<ShadowMap>` and a `DepthMaterial`.
  - The `lightSpaceMatrix`/`shadowMapTex`/`hasShadows` fields are added to
    `src/render/RenderContext.h`.
  - The shadow is computed only in the normal pass (like the skybox: `material == nullptr`),
    and skipped in the id-pass.

- [x] **7.5. `PBRLightingModel` samples the shadow map.** **Done.** The shadow multiplies only
  `Lo` (direct); it doesn't touch IBL/ambient. PCF 3×3 + slope-scaled bias, `proj.z>1` is clipped.
  `receiveShadows` is passed from `Mesh` through `Material3D::Bind` → `RenderContext` →
  `uHasShadows = hasShadows && receiveShadows`. The shadow map is on `firstTextureUnit+3`,
  `GetTextureUnitCount()→4`.
  - Declarations: `uniform sampler2D shadowMap; uniform mat4 uLightSpaceMatrix; uniform bool uHasShadows;`
  - In code: `vec4 fpLS = uLightSpaceMatrix * vec4(FragPos,1.0);` → NDC→[0,1],
    PCF 3×3 over `textureSize`, comparison against bias.
  - **The shadow multiplies only the direct contribution**: `Lo *= (1.0 - shadow);`
    IBL/ambient is **not** dimmed by the shadow (it's indirect light — otherwise
    shadows would become black and unphysical).
  - `Bind()`: the shadow map is bound to the next free unit after IBL
    (`firstTextureUnit + 3`), `GetTextureUnitCount()` → 4. `uLightSpaceMatrix`/`uHasShadows`
    are set from `ctx`.
  - Bias: `max(0.0008 * (1.0 - NdotL), 0.0005)` + optional normal-offset, to
    remove shadow acne; `GL_CLAMP_TO_BORDER` border=1 removes the shadow beyond the map.

- [ ] **7.6. Tuning (as needed).**
  - Front-face culling in the depth pass (or polygon offset) against peter-panning/acne.
  - Tuning `S`/`near`/`far` of the ortho frustum to the scene; the map resolution.
  - Later: cascaded shadow maps (CSM) for large open scenes,
    a skinned depth variant, omni shadows for a point light.

### Anti-patterns we avoid
- **Dimming ambient/IBL with the shadow.** The shadow is only for the direct
  dirLight; indirect environment light by definition arrives "around" the direct
  ray.
- **Custom FBO fiddling in every pass.** State save/restore goes through a shared
  `GLStateGuard` (as in IBL/HDR), not manual `glGetIntegerv` in five places.
- **A hard-coded shadow-map texture unit.** The unit is allocated through the
  `firstTextureUnit + GetTextureUnitCount()` accounting, like the IBL maps.

---

## Stage 8. Editor (ImGui)

Goal: import models → save them as **prefabs** → place prefabs in the scene →
save/load the **scene** (at startup a scene can be loaded).

**Stack chosen:** Dear ImGui (docking branch) on top of the existing GLFW window +
`nativefiledialog-extended` (open/save dialogs) + `nlohmann/json`
(serialization). GLFW remains the window/input; ImGui is an immediate-mode UI on
top of the GL context. Dependencies — via vcpkg.

### Prerequisites
- **Stage 6 (Geometry split)** — an actual prerequisite: a prefab instanced
  multiple times requires the split of "geometry (shared) + instance node (own
  transform)". Without it, two instances of a prefab would stand in the same spot
  (see the "share a `shared_ptr<Mesh>`" anti-pattern). The editor can be started
  before 6, but multiple instances of a single prefab will be incorrect before 6.
- Picking already exists (`ObjectSelector`, id-pass) — reused for selection.
- The FBO pattern already exists (`ShadowMap`/`IBLBaker`) — reused for the viewport.

### Substages

- [ ] **8.1. ImGui integration.** Hook up `imgui[docking-experimental,
  glfw-binding,opengl3-binding]`. Init after `glewInit`, shutdown before
  `glfwTerminate`. In the main loop: `NewFrame` → build the UI → `Render` +
  `ImGui_ImplOpenGL3_RenderDrawData`. Enable docking + (optionally) viewports.
  - Files: `src/editor/EditorUI.h/.cpp`, edits to `main.cpp`, `CMakeLists.txt`.
  - NB: ImGui must receive input before the camera — pass through
    `io.WantCaptureMouse/Keyboard`, so that clicks on panels don't rotate the
    camera.

- [ ] **8.2. Viewport as a panel (render-to-texture).** Render the scene into an
  offscreen FBO (color `GL_RGBA8` + depth), show it via `ImGui::Image` in a
  docked "Viewport" window. Resize the FBO to the panel size. The camera uses the
  viewport size, not the window size (fix `SetScreenWidth/Height`, and the center
  for the FPS camera).
  - Files: `src/render/Framebuffer.h/.cpp` (generalize the FBO), `src/editor/ViewportPanel`.

- [ ] **8.3. Hierarchy panel + inspector.** A tree of `Scene3D` (traverse
  `Children()`), node selection (synced with `ObjectSelector`). Inspector:
  position/rotation/scale, name, the `castShadows/receiveShadows` flags, material
  parameters (roughnessScale, baseColor). Edits are written straight into
  `Pivot3D`/`Material3D`.
  - **Material inspector (extension)**: a list of the material's filters with the
    ability to add/remove/reorder, change slot/blend and pick a texture — the
    same fields that are serialized in 8.5a (one source of truth:
    `Filter3D::Serialize` + `FilterFactory`). Requires the filters to be
    "inspectable" (type + parameters available to the UI).
  - Files: `src/editor/HierarchyPanel`, `src/editor/InspectorPanel`.

- [ ] **8.4. Model import.** An "Import" button → an NFD dialog (`.fbx/.obj/.gltf…`)
  → `ExternalModel(path)` → adding to the scene. The texture resolver by the
  Megascans convention already works (Stage 3).
  - File: `src/editor/AssetBrowserPanel` (+ NFD).

- [ ] **8.5. Prefab format (JSON).** A prefab = a serialized subtree of nodes:
  a reference to the geometry source (model path / primitive type), the
  **material** (see below), the local transform, child nodes, shadow flags.
  - `Prefab::Save(path)` / `Prefab::Load(path)` → an instance of a `Pivot3D` subtree.
  - Files: `src/scene/Prefab.h/.cpp`, `src/scene/Serialization.h/.cpp`.

- [ ] **8.5a. Material serialization (by fields and filters).** The material is
  saved **not by reference**, but by its own structure, so it can be assembled/
  edited by hand (the future material inspector, item 8.3):
  - material fields: lighting model (`Phong`/`PBR`/`Unlit`), `roughnessScale`,
    `cullFace`, base color, etc.;
  - **a list of filters** in order, each `{ type, slot, blendMode, params }`,
    where `params` depends on the type: `TextureMapFilter`/`NormalMapFilter` → a
    texture path (+ an sRGB flag when it appears), `ColorFilter` → a color, etc.
  - **A filter registry (factory by type-string)**: deserialization needs a
    `FilterFactory::Create(typeName, paramsJson)` — otherwise you can't build the
    right `Filter3D` subclass from a type string. Each `Filter3D` must be able to
    `Serialize()`/know its own `type`.
  - Textures **inside** filters — by path (relative to the asset root), not by
    binary.
  - Files: `src/materials/MaterialSerializer.*`, `src/materials/filters/FilterFactory.*`,
    the `Serialize`/`type` methods in `Filter3D` and its subclasses.
  - This is direct groundwork for the **material inspector**: adding/removing
    filters, changing slot/blend, picking a texture — the same fields as in JSON.

- [ ] **8.6. Scene format (save/load).** A scene = a list of prefab instances
  (a reference to the prefab + world transform + overrides), plus environment
  parameters: directional light (dir/color/intensity), ambient, the HDR
  environment (path to the `.hdr`), shadow parameters (radius/strength), the
  camera.
  - `Scene3D::Save(path)` / `Scene3D::Load(path)`.
  - Files: `src/scene/SceneSerializer.h/.cpp`, edits to `Scene3D`.

- [ ] **8.7. Load a scene at startup.** A command-line argument or the "last
  scene" from a config; if specified — load it instead of the hardcode in
  `main.cpp`. A `File → New/Open/Save/Save As` menu.

- [ ] **8.8. Transform gizmo (optional).** `ImGuizmo` (translate/rotate/scale)
  in the viewport over the selected node. Convenient, but not a blocker for the MVP.

### Notes / decisions
- **Widgets don't drive the scene directly through globals.** The editor works
  with the selected node via `ObjectSelector`, and changes go through the
  `Pivot3D`/`Material3D` setters (the same contract as at runtime).
- **Input:** when `io.WantCaptureMouse`, the camera/picking ignore the mouse —
  otherwise dragging over the UI rotates the camera. This is the first thing that
  breaks without the check.
- **Serialization by reference.** Paths to models/textures/HDR are relative to
  the asset root, so scenes/prefabs are portable.
- **An MVP without Stage 6** is possible (one instance per prefab), but multiple
  instances of a single prefab require 6 (geometry sharing).

---

## Relationship to `REFACTORING_PLAN.md`

- **3.4 (cache `glGetUniformLocation`)** must be done **before** stage 1.
  Otherwise `ILightingModel::Bind` would call `glGetUniformLocation` 6–10 times
  on every draw — whereas we specifically want the new abstraction not to make
  the engine slower.

- **3.5 (`bindStandardUniforms`)** partially overlaps with this plan:
  what used to be a "shared uniform" (`viewPos`, `light.*`) moves into
  `ILightingModel`, and only `model`/`view`/`projection` stays in `MaterialBase`.
  After stage 1, item 3.5 closes automatically.

- **5.9 (Extensible `Vertex`)** becomes critical at two stages:
  - **Stage 5** (IBL/Skybox) — the skybox uses its own layout without UV.
  - **Stage 6** (Geometry split) — `Geometry::Geometry(layout, vertices, ...)`
    is type-independent only if the layout comes from outside. Without 5.9
    Geometry would depend rigidly on `struct Vertex` and couldn't store, for
    example, a `PBRVertex` or skybox vertices.
  - Without 5.9 every new vertex format breaks the existing VAO in Mesh.

- **2.5 / 5.4 (RAII GL resources)** — a prerequisite for Stage 6. `Geometry` =
  a bundle of `GLBuffer` + `GLVertexArray`, and ownership via
  `shared_ptr<Geometry>` works correctly precisely because the RAII wrappers
  destroy GL resources deterministically. Without them an "extra" Geometry in the
  cache would mean a VBO leak in VRAM until the program ends.

---

## Anti-patterns we deliberately avoid

1. **`mat->SetLightingModel("PBR")`** — a string name instead of a type.
   It hides the dependency, loses autocompletion, and is only caught at
   runtime.
2. **`Material3D<TLightingModel>`** — a template parameter.
   It explodes Material3D into N versions and breaks dynamically changing the
   model at runtime.
3. **`LightingFilter : Filter3D`** — lighting as a Filter subclass.
   A Filter is a modulator (writes into a slot), Lighting is a synthesizer (reads
   all slots, writes the final color). These are different contracts; mixing them
   would lead to special cases in `Filter3D` like "if this is lighting, do it
   differently".
4. **A single `Bind(program, unit, ctx)` for Filter and LightingModel.**
   The temptation: "let's make a universal interface". The downside: filters
   don't need `ctx`, and extending the API for one consumer violates Interface
   Segregation. We deliberately keep two different signatures — that pins down
   the contract difference at the type level, not in comments.
5. **Lighting uniforms in Scene3D as statics.** The temptation: "there's one
   light per scene, let Scene3D bind it itself". The downside: it breaks the "a
   material owns what it binds" contract. In our scheme `ILightingModel::Bind`
   reads Scene3D through `ctx`, but writes the uniforms itself.
6. **Sharing a `shared_ptr<Mesh>` for "one geometry".** The temptation:
   "I already have a `shared_ptr<Mesh>`, I'll put it in two places in the scene
   and get one geometry in VRAM". The downside: `Mesh : Pivot3D` has **one**
   transform, parent and set of children. The two "instances" would stand in the
   same spot. This is an **architectural role mistake**, not an optimization —
   it's solved by Stage 6 (splitting Geometry and Mesh-instance), not by
   shared_ptr tricks.
