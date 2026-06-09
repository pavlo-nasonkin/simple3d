# simple3d Refactoring Plan

This document describes the order in which to fix the issues found during the
engine review. Tasks are sorted by priority: runtime blockers first, then
architecture, then stability, then style and optimizations.

Task status legend: `[ ]` — not done, `[~]` — in progress, `[x]` — done.

---

## Stage 0. Preparation

- [x] **0.1. Enable C++17.** In `CMakeLists.txt` change `set(CMAKE_CXX_STANDARD 11)` to `17` (or `20`). Unlocks `std::filesystem`, `std::string_view`, structured bindings.
- [x] **0.2. Create the `refactor/engine-core` branch.** All changes in stages 1–4 should go in incremental commits, one item per commit.
- [x] **0.3. Minimal smoke scene.** In `main.cpp`, next to `nanosuit`, load a single `BoxModel` without animation — so we can see the results of changes even before `ExternalModel` is fixed.

---

## Stage 1. Runtime blockers (things without which the scene won't render or will crash)

Goal of this stage: get the engine running up to displaying the cube and nanosuit without crashes.

- [x] **1.1. Replace `glm::mat4()` with `glm::mat4(1.0f)`.** In modern GLM (vcpkg ships ≥ 0.9.9) `glm::mat4()` creates a zero matrix — objects collapse to a point.
  - Files: `src/Pivot3D.cpp` (`render`), `src/ExternalModel.cpp` (`render`), `src/camera/FreeLookCamera.cpp` (`handleMouseMove`).
  - Check: after the fix, the cube is visible on screen.

- [x] **1.2. Fix the TRS order in `Pivot3D::render`.** Currently it ends up as `S * R * T` instead of `T * R * S`; the object moves in local coordinates.
  ```cpp
  _model = glm::mat4(1.0f);
  _model = glm::translate(_model, _position);
  _model *= rotationMatrix();      // extract applyTransformRotation into a helper
  _model = glm::scale(_model, _scale);
  ```
  - Files: `src/Pivot3D.cpp`, `src/ExternalModel.cpp` (same block duplicated), `src/Mesh.cpp` (`applyTransformRotation` with the π/2 offset — review whether it's needed).

- [x] **1.3. Call `init()` after adding to the scene.**
  - In `Pivot3D::addChild`, after binding `_parent`, call `child->init()` (or introduce two-phase initialization: add → setupScene).
  - Alternative: in `main.cpp` explicitly call `soldier->init()` and `scene.init()`.
  - Files: `src/Pivot3D.cpp`, `main.cpp`.
  - Check: without this `ExternalModel::_scene == nullptr` → crash on the first frame.

- [x] **1.4. Set `Device3D::scene3D`.** Without this all materials (`Material3D::bind`, `ColorMaterial::bind`) crash on `nullptr` when calling `getLightAmbient()` etc.
  - Currently `Device3D::scene3D` is a `std::shared_ptr<Scene3D>` that is never assigned. Simplest fix: in `main.cpp` do `auto scene = std::make_shared<Scene3D>(...)` and `Device3D::scene3D = scene`.
  - This requires turning `scene` from a stack object into a `shared_ptr` (see 2.1).

- [x] **1.5. Fix the copy bug in `Vector3f(const float*)`.**
  ```cpp
  Vector3f(const float* p) { x = p[0]; y = p[1]; z = p[2]; }
  ```
  - File: `src/utils/Math3d.h`.

- [x] **1.6. Add `break;` in `FreeLookCamera::handleMouseButton`.** Currently the middle case falls through into the right case.
  - File: `src/camera/FreeLookCamera.cpp`.

- [x] **1.7. Fix the declaration of `KeyboardInput::_keys`.**
  - In the `.cpp`: `bool KeyboardInput::_keys[1024] = {};` (instead of `bool KeyboardInput::_keys[];`).
  - File: `src/input/KeyboardInput.cpp`.

- [x] **1.8. Guard against models without animation.** In `ExternalModel::BoneTransform` and `ReadNodeHeirarchy`, check `_scene->mNumAnimations > 0` before accessing `mAnimations[0]`.
  - File: `src/ExternalModel.cpp`.

- [x] **1.9. Check the result of `SOIL_load_image`.** Log `SOIL_last_result()`, don't pass `nullptr` to `glTexImage2D`.
  - File: `src/resources/TextureManager.h`.

---

## Stage 2. Architectural blockers (lifetime + scene graph)

Goal of this stage: the transform hierarchy works, objects are guaranteed to
stay alive and to be released correctly.

- [x] **2.1. Scene as a `shared_ptr`.** Create `Scene3D` via `std::make_shared`, because `Pivot3D` inherits from `std::enable_shared_from_this`. Without this any `shared_from_this()` on the scene is a `bad_weak_ptr`.
  - Files: `main.cpp`.

- [x] **2.2. Parent = `std::weak_ptr<Pivot3D>`.** Currently `_parent` is a `shared_ptr`, which creates a cycle and leaks the graph.
  - Replace with: `std::weak_ptr<Pivot3D> _parent;`.
  - Everywhere `_parent` is used: `auto p = _parent.lock(); if (p) ...`.
  - Files: `src/Pivot3D.h`, `src/Pivot3D.cpp`.

- [x] **2.3. Remove `shared_from_this()` from `~Pivot3D`.** In the destructor the weak_ptr is already expired, so it would be `bad_weak_ptr` → `std::terminate`.
  - Remove the "detach from parent" block in the destructor (it isn't needed if parent is a weak_ptr and doesn't itself hold the object being destroyed).
  - File: `src/Pivot3D.cpp`.

- [x] **2.4. Transform hierarchy.** Child nodes must account for the parent's world matrix.
  - Option A (minimal): in `Pivot3D::render` accept `const glm::mat4& parentWorld = glm::mat4(1.0f)`, compute `_world = parentWorld * local`, pass it to children.
  - Option B (proper): pass `RenderContext& ctx` through — see 3.1.
  - Files: `src/Pivot3D.h/.cpp`, `src/Scene3D.cpp`, `src/Mesh.cpp`, `src/ExternalModel.cpp`, `src/Model.cpp`.
  - Check: a nested cube (`scene.addChild(parent); parent->addChild(child); child.setPosition(...)`) renders with the coordinates summed.

- [x] **2.5. Delete GL resources in destructors.**
  - `Mesh::~Mesh`: `glDeleteVertexArrays(1, &vertexAttributesArray); glDeleteBuffers(...)`.
  - `Shader::~Shader`: `glDeleteProgram(Program)` (guarded with `if (Program != 0)`).
  - Forbid copying `Shader` (or make it move-only) so there's no double `glDeleteProgram` on the same program.
  - Files: `src/Mesh.cpp`, `src/Shader.h/.cpp`.

- [x] **2.6. `removeListener` in the inputs.** Currently when a listener object is destroyed its pointer stays in the static vector → dangling pointer.
  - Add a `removeListener` method to `MouseInput`/`KeyboardInput`/`UpdateBroadcaster` and call it from the destructors of `FreeLookCamera`, `FirstPersonCamera`, `ObjectSelector`.
  - Files: `src/input/MouseInput.*`, `src/input/KeyboardInput.*`, `src/UpdateBroadcaster.*`, `src/camera/*`, `src/object_selector/ObjectSelector.*`.

---

## Stage 3. Architecture (Device3D → RenderContext, materials)

Goal: remove global state and fragile dependencies through statics.

- [x] **3.1. Introduce `RenderContext`.**
  ```cpp
  struct RenderContext {
      glm::mat4 view;
      glm::mat4 projection;
      glm::mat4 model;            // the node's current world matrix
      Camera*   camera   = nullptr;
      Scene3D*  scene    = nullptr;
      const Pivot3D* currentObject = nullptr;
  };
  ```
  - Pass `const RenderContext&` (or `&`) into `Pivot3D::render(ctx, material)` and `MaterialBase::bind(ctx, mesh)`.
  - File: new `src/render/RenderContext.h`.

- [x] **3.2. Remove `Device3D`.** After 3.1 this class is no longer needed; in its place is `RenderContext`, which is owned by the scene and consumed per frame.
  - Delete `src/Device3D.h/.cpp`, fix all `Device3D::view/projection/...` → `ctx.view/...`.

- [x] **3.3. The material owns its own shader.**
  - `ShaderFactory` stores sources + the GLuint of compiled shaders (vs/fs), but **not** finished programs.
  - Each material assembles its own `Program` (links its combination of vs + fs + filters). This avoids mutual shader breakage in `Material3D::build` (see review item 11).
  - Alternative: the program cache key is a tuple (vs source + fs source **after** filters are applied).
  - Files: `src/materials/ShaderFactory.*`, `src/Material3D.cpp`.

- [x] **3.4. Cache `glGetUniformLocation`.** Currently `bind()` queries locations on every draw call.
  - In `MaterialBase` add `std::unordered_map<std::string, GLint> _uniformLocations` and a lazy cache.
  - Files: `src/materials/MaterialBase.*`, `src/Material3D.cpp`, `src/materials/ColorMaterial.cpp`, `src/materials/ObjectIdMaterial.cpp`, `src/materials/SkinnedMaterial3D.cpp`.

- [x] **3.5. Move the base uniform block into `MaterialBase::bindStandardUniforms(ctx)`.** Currently `Material3D::bind` and `ColorMaterial::bind` duplicate the same code (view/projection/model/light).
  - Files: `src/materials/MaterialBase.*`, `src/Material3D.cpp`, `src/materials/ColorMaterial.cpp`.

- [x] **3.6. Symmetry of `bind`/`unbind` for GL state.** `CullFace` should be reset in `unbind` (or, better, set anew on each `bind` — without depending on previous state).
  - File: `src/materials/MaterialBase.cpp`.

---

## Stage 4. Stability and window events

- [x] **4.1. Handle window resize.** `glfwSetFramebufferSizeCallback` → update `Device3D::sceenWidth/Height` (or the `Scene::viewport` field) and call `glViewport`, rebuild the camera's `projection`.
  - File: `main.cpp`.

- [x] **4.2. Fix the typo `sceenWidth` → `screenWidth`.** If we don't remove `Device3D` entirely.

- [x] **4.3. Use paths relative to the exe.** `"../assets/..."` breaks when launched from the IDE folder or an install. Done via `AssetPaths` (see 10.1).
  - Via `std::filesystem` determine the path next to `argv[0]`, and load `assets/` relative to it.
  - Files: `main.cpp`, `src/materials/ShaderFactory.cpp`, `src/utils/FileUtils.cpp`.

- [x] **4.4. Throw `std::exception`, not `const char*`.** Replace `throw "..."` with `throw std::invalid_argument(...)` / `std::runtime_error(...)`.
  - Files: `src/Pivot3D.cpp`, `src/materials/ShaderFactory.cpp`.

---

## Stage 5. Style and API cleanup

- [x] **5.1. Remove `using namespace std;` from `src/Mesh.h`.** Spell out `std::` explicitly in all dependent files.

- [x] **5.2. Remove the `make_shared<T>(T(...))` anti-pattern.** Replace with `make_shared<T>(args...)`. Especially important for `Mesh` (avoids copying `vector<Vertex>`).
  - Files: throughout, see `Material3D::clone`, `BoxModel::processMesh`, `ExternalModel::processMesh`, `Scene3D::Scene3D`, `ObjectSelector::ObjectSelector`, etc.

- [x] **5.3. `std::string` → `std::string_view`** in getter/setter signatures and cache arguments (`TextureManager`, `EventDispatcher`, `ShaderFactory`).

- [x] **5.4. `Mesh` owns its resources, not "raw" `GLuint`s.** Introduce `class GLBuffer` / `class GLVertexArray` RAII wrappers (or `std::unique_ptr` with a custom deleter).

- [x] **5.5. Unify the math on GLM.** Remove `Vector3f`/`Vector4f`/`Matrix4f`/`Quaternion` from `Math3d.h`, port skinning to `glm::mat4`/`glm::quat`. Add a helper for converting `aiMatrix4x4 → glm::mat4`.
  - Files: `src/utils/Math3d.*`, `src/ExternalModel.cpp`, `src/materials/SkinnedMaterial3D.cpp`.

- [ ] **5.6. Unify naming.** Agree on: private fields — `_camelCase`, methods — `camelCase`, classes — `PascalCase`, constants — `kUpperCamel` or `UPPER_SNAKE`. Bring the camera (Position/Yaw/Pitch) in line with the project style.

- [x] **5.7. Move the GLFW adapters.** `GLFWKeyboardInput.*` and `GLFWMouseInput.*` from the project root into `src/platform/glfw/`.

- [x] **5.8. `Pivot3D::translate`/`scale` are empty.** Either implement them (cumulative translate/scale), or remove them.

- [x] **5.9. Extensible `Vertex`.** Describe `VertexAttribute`/`VertexLayout` so the material/mesh can work with different vertex formats (for future PBR — add tangent/uv2/color).

---

## Stage 6. Optional / future groundwork

- [ ] **6.1. Object picking via FBO instead of re-rendering the scene.** Currently `ObjectSelector::pickObject` renders the scene into the default backbuffer, which causes flickering.
- [ ] **6.2. Uniform Buffer Object for view/projection/light.** One UBO per scene, all materials bind it, the repeated `glUniform*` calls go away.
- [ ] **6.3. Logging (spdlog/custom).** Instead of `std::cout` in shaders/SOIL — structured messages with levels.
- [ ] **6.4. Tests.** At least unit tests for `Pivot3D` (hierarchy, lookup by id), `EventDispatcher`, `Math3d` (if it stays).
- [ ] **6.5. CI.** GitHub Actions on Windows-MSVC with vcpkg, build + smoke run.

---

## "Engine works correctly" checklist

After completing stages 1+2:
- [ ] The cube is visible on screen and not degenerate.
- [ ] Transform hierarchy: a child object moves together with its parent.
- [ ] nanosuit loads and renders without a crash.
- [ ] The camera is controlled by the mouse without triggering the "right button" when clicking the wheel.
- [ ] Closing the window leaves no repeated delete/`glDelete*` warnings in the output.
- [ ] When the window is resized the image isn't stretched.

---
---

# Architecture & Code Review — round 2 (post PBR / IBL / shadows / editor)

> Added after a full re-review. Stages 0–5 above are essentially done, and the
> engine has since grown well beyond the original scope: PBR + IBL, directional
> shadow mapping, HDR skybox, an ImGui/ImGuizmo editor with docking, GPU object
> picking through an FBO, and a JSON+binary scene/prefab serializer. The items
> below are *new* findings against the current code. Same legend (`[ ]`/`[~]`/`[x]`)
> and priority order (correctness → architecture → rendering → resources → quality).

## What is already solid (so we don't regress it)

- Clean RAII for GL objects: `GLBuffer`, `GLVertexArray`, `Shader`, `ShadowMap`, `Framebuffer` are move-only and delete in their destructors.
- `Geometry` is immutable, shared via `shared_ptr`, and deduplicated by `GeometryRegistry` (weak-ptr cache) — one VBO/VAO for many `Mesh` instances.
- Lighting is decoupled from materials (`ILightingModel`) and filters compose the fragment shader by slot — a genuinely nice design.
- Uniform locations are cached (`UniformsLocationCache`) with heterogeneous `string_view` lookup; `RenderContext` replaced the old `Device3D` global.
- Serialization deduplicates geometry and materials and stores binary blobs out-of-band.

---

## Stage 7. Correctness / latent bugs (do first)

- [x] **7.1. FOV is passed in degrees but GLM expects radians.** `Camera::buildProjectionMatrix(float fow, ...)` calls `glm::perspective(fow, ...)`, and every caller passes `45.0f` (degrees). Modern GLM is radians-only, so the real vertical FOV ends up ≈58° by accident (45 rad wraps around). Fix: `glm::perspective(glm::radians(fow), ...)`.
  - Files: `src/camera/Camera.cpp`, callers in `main.cpp`, `src/editor/ViewportPanel.cpp`.

- [x] **7.2. Uninitialized matrix in the orbit camera.** In `FreeLookCamera::HandleMouseMove` the RMB-orbit branch declares `glm::mat4 view;` (uninitialized in default GLM) and then does `glm::translate(view, ...)` — i.e. it multiplies garbage. It "works" only because the very next line overwrites column 0–3 directly, but the intermediate translate/rotate chain is built on an undefined base. Initialize with `glm::mat4 view(1.0f);`. Also the trailing `view = glm::inverse(view);` result is discarded (dead code).
  - File: `src/camera/FreeLookCamera.cpp`.

- [x] **7.3. TextureManager cache key is the bare filename, not the full path.** `TextureManager::getTexture` keys `_textures` on `texturePath` (often just `Color.png` / `Normal.png`), ignoring `directory`. Two different assets that share a texture filename in different folders will collide and the second one silently gets the first one's texture. Fix: key on `directory + "/" + filename` (or a normalized absolute path).
  - File: `src/resources/TextureManager.cpp`.

- [ ] **7.4. Skinned meshes don't shade or cast shadows consistently.**
  - `SkinnedMaterial3D` is created without `SetLightingModel(...)`, so it falls back to `PhongLightingModel` (the `Material3D` default) while static meshes use PBR — animated models look different from everything else.
  - The shadow depth pass uses a single non-skinned `DepthMaterial` (`depth.vsh`), so skinned meshes are rasterized into the shadow map in bind pose — their shadows won't match the animation. Needs a skinned depth variant (or a `castShadows=false` opt-out for skinned meshes until then).
  - Files: `src/models/ExternalModel.cpp` (`ProcessMesh`), `src/Scene3D.cpp` (`EnableShadows`/shadow pass), `src/materials/DepthMaterial.*`.

- [ ] **7.5. Lighting models dereference `ctx.camera` / `ctx.scene3D` without null checks.** `PBRLightingModel::Bind` and `PhongLightingModel::Bind` assume both are non-null. Any render path that forgets to populate them is an immediate crash instead of a no-op. Add guards (or assert + early-out).
  - Files: `src/lighting/PBRLightingModel.cpp`, `src/lighting/PhongLightingModel.cpp`.

- [ ] **7.6. `ShaderFactory` reports compile failure but still returns the broken shader id.** `createShaderFromSource` logs and sets `status=1`, but callers (`GetCompiledShader*`) only log and cache the failed object anyway; linking then throws a less specific error. Surface the failure (throw / return optional) so the message points at the offending shader.
  - File: `src/materials/ShaderFactory.cpp`.

- [x] **7.7. `Pivot3D::_idCounter` collisions after load.** Done. `Pivot3D::SetId` now advances the
  global counter past any restored id (`_idCounter = max(_idCounter, id + 1)`), so nodes created after a
  scene/prefab load (e.g. adding an object in the editor) can't reuse a loaded node's id. This makes
  GPU picking and `NodeRef`/`BehRef` resolution reliable across load + edit, and keeps ids reproducible
  when reloading the same file. The camera node is created first (lowest id) and excluded from
  serialization, so loaded ids never collide with it.
  - *Remaining (minor):* ids still climb within a long session (24-bit picking ceiling at ~16.7M nodes);
    a per-scene id allocator/remap is a future option if needed.
  - Files: `src/Pivot3D.cpp`.

- [x] **7.8. Camera navigation regressed after the camera-node migration (DEVELOP 9.5).** Done.
  Added `OrbitCameraBehaviour` (`src/behaviours/OrbitCameraBehaviour.*`) restoring the old
  `FreeLookCamera` scheme: **MMB-drag pan** (moves the focus point in screen plane), **RMB-drag orbit**
  around a focus (the selected object's position, radius = distance to it; else a point in front of the
  camera), **wheel dolly** (changes distance to focus). It writes the camera node's position+rotation;
  `CameraBehaviour` derives the (roll-free) view. `MouseInput` gained `ConsumeScrollY()` (per-frame
  scroll accumulator for behaviour polling). Demo/editor camera switched from `FlyCameraBehaviour` to
  `OrbitCameraBehaviour` (`FlyCameraBehaviour` kept as an alternative). The roll fix
  (`CameraBehaviour` lookAt with world-up) stays. The detailed old/new comparison below is retained
  for reference.

  **Old `FreeLookCamera` (target reference):**
  - **MMB drag → pan** (truck/pedestal): `Position += Right*dx − Up*dy`, scaled by screen fraction.
  - **RMB drag → orbit/tumble around a focus point**: the focus is the **selected object**'s position
    (orbit radius = distance to it) or a default point `Position + Front*3`. It rebuilds the transform as
    `T(target) · Ry(yaw+90°) · Rx(pitch) · T(0,0,radius)` — i.e. the camera moves *on a sphere around the
    target* and keeps looking at it. (Order `Ry·Rx` ⇒ already roll-free.)
  - **Wheel → dolly**: `Position += Front*scroll` (zoom toward/away), disabled while panning.
  - **No keyboard movement.**

  **New `FlyCameraBehaviour` (current):**
  - **RMB held → look-in-place** (rotates around the *camera's own* position) **+ WASD/QE fly**.
  - No MMB pan, no wheel dolly, no orbit-around-target, ignores the current selection.

  **Differences to close:**
  1. **Pivot of rotation:** orbit around a focus point (old) vs. rotate around the camera position (new).
  2. **Buttons:** MMB-pan + RMB-orbit + wheel-dolly (old) vs. RMB-look + WASD (new).
  3. **Focus on selection:** old orbits around the selected node (via `ObjectSelector`); new ignores it.
  4. **Movement model:** old = orbit/pan/dolly only; new = WASD/QE fly.
  5. **Wheel:** old = dolly; new = unused.

  Implementation note: the controller must set the node **position** (on the sphere) and **orientation**
  (facing the target). Since `CameraBehaviour` builds the view via `lookAt(pos, pos+forward, worldUp)`,
  the orbit controller can compute `pos` on the sphere and a node rotation whose forward points at the
  target. Needs `MouseInput` scroll access for the behaviour (poll) — add a per-frame scroll accumulator
  like `IsButtonPressed`/mouse-delta (DEVELOP 9.4).
  - Files: new `src/behaviours/OrbitCameraBehaviour.*`, `src/behaviours/FlyCameraBehaviour.*` (keep as an
    alternative), `src/app/Application.cpp`, `src/input/MouseInput.*`; reference `src/camera/FreeLookCamera.cpp`.

---

## Stage 8. Architecture & design

- [x] **8.1. `main.cpp` is a ~300-line god-function.** Extracted an `Application` class (`src/app/Application.*`) that owns the window, scene, camera, editor + panels and the frame loop, with the lifecycle split into `InitWindow`/`SetupScene`/`BuildDemoScene`/`InitEditor`/`MainLoop`/`RenderEditor`/`RenderGame`/`Shutdown`. The hard-coded demo content lives in `BuildDemoScene()`. `main.cpp` is now ~6 lines (`Application app; return app.Run(argc, argv);`). Editor/panels are held via `unique_ptr` and released in `Shutdown()` while the GL context is still alive (fixes the teardown-order risk the old inner-scope relied on).
  - Files: `src/app/Application.h/.cpp`, `main.cpp`, `CMakeLists.txt`.
  - Follow-up: `BuildDemoScene` is still hard-coded content; once scene serialization is the default it can move to an `assets/scenes/default.json` and drop from code.

- [ ] **8.2. Dead / legacy light state in `Scene3D`.** `_lamp`, `_lightPosition`, `_lightAmbient/Diffuse/Specular` and `InitLightView()`/`InitEnvironment()` are remnants of the old point-light/Phong path. `_lamp` is built but never added to the graph (`AddChild` commented out). Only `PhongLightingModel` still reads the point light. Decide: either make the point light a first-class, serialized light, or delete the legacy fields and `InitLightView`.
  - Files: `src/Scene3D.h/.cpp`.

- [~] **8.3. Renderer layer (v1 done).** Extracted the per-frame pass orchestration
  (shadow → clear → main → skybox) out of `Scene3D::Render` into a standalone
  `Renderer::RenderScene(scene, ctx, overrideMaterial)` (`src/render/Renderer.*`).
  `Scene3D` no longer overrides `Render`: it's now a data/graph container that exposes the
  resources the renderer reads (`GetShadowMap`/`GetDepthMaterial`/`GetSkybox`,
  `GetEffectiveDirLight*`, shadow center/radius) + `RefreshActiveLights`. The renderer draws
  the graph via the base `Pivot3D::Render` traversal. Call sites (`Application::RenderGame`,
  `ViewportPanel`, `ObjectSelector::PickAt`) now call `Renderer::RenderScene`.
  - *Remaining:* a real `RenderPass` list (so transparency/post-process/9.6 plug in), a
    multi-light list instead of single effective dir-light, and moving resource *ownership*
    (shadow map / skybox / IBL) out of `Scene3D` into the renderer/frame-graph. This is the
    prerequisite the plan wanted before `DEVELOP_PLAN` 9.12 (MeshRenderer).
  - Files: `src/render/Renderer.*`, `src/Scene3D.*`, `main.cpp`/`app/Application.cpp`,
    `src/editor/ViewportPanel.cpp`, `src/object_selector/ObjectSelector.cpp`.

- [ ] **8.4. Two-phase init is implicit and easy to get wrong.** Objects must be constructed *and* have `Init()` called (which for `ExternalModel` does the whole Assimp load) — `AddChild` does not init. This couples object construction order to GL-context availability and is a common foot-gun. Consider a factory that returns fully-initialized nodes, or document the contract in `Pivot3D`.
  - Files: `src/Pivot3D.*`, `src/models/ExternalModel.cpp`, `main.cpp`.

- [ ] **8.5. `ExternalModel` keeps the whole Assimp scene alive forever.** It stores the `Assimp::Importer` (`_import`) and `_scene` as members so animation can be sampled each frame. For static meshes the imported scene (all vertices, materials, node tree) stays resident in RAM for the program's life with no benefit. Release `_import`/`_scene` after `Init()` when `mNumAnimations == 0`; for animated models, extract just the keyframe data into a compact owned structure.
  - Files: `src/models/ExternalModel.*`.

- [ ] **8.6. Picking granularity & UX.** `ObjectIdMaterial` encodes the leaf `Mesh` id, so clicking a model selects one sub-mesh, not the logical object. Decide whether picking should walk up to the owning `ExternalModel`/`Model`, and store that selection semantics in one place.
  - Files: `src/object_selector/ObjectSelector.cpp`, `src/materials/ObjectIdMaterial.cpp`.

- [ ] **8.7. Serialization bakes full generated GLSL into every material.** `RegisterMaterial` writes `compiled.vertex`/`compiled.fragment` (the whole post-injection shader) per material, gated by a hand-maintained `ShaderGenVersion()`. This bloats scene files, duplicates identical shaders, and silently invalidates on any generator change. Prefer storing only the filter/lighting description and regenerating on load (keep the compiled blob optional, as a cache).
  - Files: `src/scene/Serialization.cpp`, `src/materials/Material3D.*`.

- [ ] **8.8. `Mesh` and `Pivot3D` duplicate the render-traversal preamble.** Both reimplement the `shadowPass && !_castShadows` skip and the `context.model = ctx.model * LocalMatrix()` + child recursion. Factor the traversal into the base (template-method: base walks the graph, virtual `Draw(ctx)` for leaves).
  - Files: `src/Pivot3D.cpp`, `src/models/Mesh.cpp`.

---

## Stage 9. Rendering pipeline & performance

> Context: the entire render is an immediate-mode recursive walk of the scene
> graph. `Scene3D::Render` runs the shadow pass, clears, then `Pivot3D::Render`
> recurses the tree; every leaf `Mesh` does `material->Bind()` → `geometry->Draw()`
> → `material->Unbind()`. There is no render-queue, no batching and no culling, so
> cost scales linearly with the *total* node count regardless of what's on screen.
> The items below go roughly cheapest-win-first.

- [ ] **9.1. `RenderContext` is copied by value at every node, every frame.**
  - *Problem.* `Pivot3D::Render` does `RenderContext context = ctx;` (`src/Pivot3D.cpp:87`) and `Mesh::Render` does the same (`src/models/Mesh.cpp:23`) just to overwrite `context.model`. `RenderContext` is ~230 bytes (4× `glm::mat4` = 256 bytes alone: `view`, `projection`, `model`, `lightSpaceMatrix`, plus pointers and flags). For a tree of N nodes that's N full-struct copies per frame, per pass (main + shadow + pick).
  - *Why it matters.* Pure overhead that grows with scene size and pass count; also makes the hot path cache-unfriendly.
  - *How.* Split the context into a per-frame immutable part passed by `const&` and the changing part passed explicitly:

```cpp
// FrameContext: built once per pass, never copied
struct FrameContext {
    glm::mat4 view, projection, lightSpaceMatrix;
    Camera* camera; Scene3D* scene3D;
    unsigned int shadowMap; bool hasShadows, shadowPass;
};
// model + receiveShadows are the only things that change while walking
virtual void Render(const FrameContext& f, const glm::mat4& parentWorld,
                    MaterialBase* override = nullptr);
```

  Each node computes `world = parentWorld * LocalMatrix()` (a local, not a struct copy) and forwards `world` to children.
  - *Check.* No `RenderContext` (or successor) is copied in the traversal; only `glm::mat4` locals are produced. Visual output unchanged.
  - Files: `src/Pivot3D.*`, `src/models/Mesh.*`, `src/render/RenderContext.h`, `src/models/ExternalModel.cpp` (it overrides `Render`).

- [ ] **9.2. Per-draw re-upload of per-frame uniforms (UBO).**
  - *Problem.* `MaterialBase::Bind` re-sends `model`/`view`/`projection` for every mesh (`src/materials/MaterialBase.cpp:122-128`), and on top of that `PBRLightingModel::Bind` re-sends `viewPos`, `dirLight.*`, `ambientLight`, the three IBL sampler indices + `uMaxReflectionLod`, and all shadow uniforms **on every draw call** (`src/lighting/PBRLightingModel.cpp:139-185`), even though only `model` actually changes between meshes. With M meshes that's M× redundant `glUniform*`/`glActiveTexture`/`glBindTexture` for constants.
  - *Why it matters.* This is the single biggest cheap win at current mesh counts: `glUniform*` calls and especially the per-draw IBL/shadow texture rebinds dominate the CPU side of the frame.
  - *How.* Two complementary moves:
    1. Introduce a `Matrices` UBO (binding point 0) with `view`/`projection`/`viewPos` and a `Lighting` UBO (binding point 1) with `dirLight`, `ambient`, shadow matrix/flags. Fill them **once per frame** in the renderer; declare `layout(std140, binding=N)` blocks in the shaders. Only `model` stays a regular per-draw uniform.
    2. Bind the IBL cubemaps + BRDF LUT + shadow map **once per pass** to fixed texture units (they're constant for the whole frame), instead of per material `Bind`. Materials/filters then only manage their own object textures.
  - *Notes.* This formalizes the half-done item 6.2. Keep a non-UBO fallback path only if you still target GL contexts without UBO (3.1 core has it, so not needed).
  - *Check.* A frame with M meshes issues O(1) light/IBL/shadow uniform updates and O(M) `model` updates; RenderDoc shows the IBL/shadow textures bound once, not per draw.
  - Files: `src/materials/MaterialBase.cpp`, `src/lighting/*LightingModel.cpp`, `assets/shaders/*.fsh/.vsh`, new `src/render/Renderer.*` (owns the UBOs).

- [~] **9.3. Frustum culling + draw sorting (queue) done; instancing pending.**
  Implemented a render queue in `Renderer`: `Pivot3D::CollectDrawItems` walks the graph once into a
  flat `std::vector<DrawItem>` (world matrix + `MeshRenderer` + owner; honours the `shadowPass`/
  `_castShadows` skip), then `Renderer::DrawQueue` frustum-culls and sorts before drawing. (a) `Geometry`
  computes a local AABB from the position attribute at build time (`HasBounds`/`AABBMin`/`AABBMax`);
  (b) `Frustum` (`render/Frustum.h`, Gribb–Hartmann from `proj*view`) culls each item's world AABB —
  the main pass culls against the camera frustum, the shadow pass against the light's ortho box;
  (c) items are sorted by material then geometry to group state and set up instancing. The old
  `Pivot3D::Render` recursion is no longer used by the renderer.
  - *Remaining:* actual GPU **instancing** (`glDrawElementsInstanced` for identical material+geometry
    runs — they're now adjacent after the sort); optional state-dedup (skip re-binding the same material)
    and a proper transparent pass ordering (9.6).
  - Files: `src/render/Renderer.cpp`, `src/render/Frustum.h`, `src/render/DrawItem.h`,
    `src/render/Geometry.*`, `src/Pivot3D.*`.
  - *Problem.* The graph is fully traversed and drawn each frame regardless of visibility (`Scene3D::Render` → `Pivot3D::Render`), and draws happen in tree order, so the program/cull state churns: each `Mesh` does `Bind` (`glUseProgram`, `glEnable/glDisable(GL_CULL_FACE)`, uniform spam) → `Unbind`. Identical materials aren't grouped; off-screen objects still pay vertex/fragment cost.
  - *Why it matters.* Wasted GPU on invisible geometry and wasted CPU on redundant state changes; both scale badly as scenes grow (the demo already has a 100×100 ground + multi-part FBX models).
  - *How (incremental).*
    1. **AABB per `Geometry`.** Compute a local-space bounding box at build time (cheap, from the vertex data already held CPU-side in `Geometry::_vertexData`); transform to world by the node matrix for the test.
    2. **Frustum cull.** Extract the 6 planes from `projection * view`; skip nodes whose world AABB is fully outside. Build a flat list of visible `(mesh, world)` instead of drawing during the walk.
    3. **Sort the visible list** by program → material → geometry to minimize `glUseProgram`/texture/VAO switches; only toggle cull state when it actually changes.
    4. **(Later) Instancing.** Identical geometry+material (e.g. repeated props) drawn via `glDrawElementsInstanced` with a per-instance model matrix buffer.
  - *Check.* Looking away from all objects drops draw calls to ~0; frame time scales with *visible* meshes, not total. State-change count per frame drops after sorting.
  - Files: `src/render/Geometry.*` (AABB), new `src/render/Renderer.*` + `Frustum.*`, `src/Scene3D.cpp`.

- [ ] **9.4. Picking re-renders the whole scene into the id-FBO on every click.**
  - *Problem.* `ObjectSelector::PickAt` calls `Scene3D::Render(ctx, _colorMaterial.get())` (`src/object_selector/ObjectSelector.cpp:76`). The shadow pass is correctly skipped (guarded by `!material` in `Scene3D::Render`), and skybox too, but the full opaque scene is still rasterized into the id-FBO for a single-pixel read.
  - *Why it matters.* Currently fine (picking is user-initiated, one click), so this is explicitly **low priority** — listed so it isn't forgotten if picking ever becomes per-frame (e.g. hover-highlight) or scenes get huge.
  - *How (only if it becomes hot).* Cache the id-buffer and invalidate it only when the camera or scene graph changes; or render the id-pass into a small scissored region around the cursor instead of the full viewport.
  - *Check.* No id-pass render occurs on frames without a pick (or without camera/scene change).
  - File: `src/object_selector/ObjectSelector.cpp`.

- [ ] **9.5. Shadow quality knobs.**
  - *Problem.* One fixed directional map: size set at `EnableShadows(4096, 20.0f)`, ortho frustum is a fixed `2*radius` cube around `_shadowCenter` (`ShadowMap::ComputeLightSpaceMatrix`, `src/render/ShadowMap.cpp:58-75`). The depth pass uses a plain `DepthMaterial` with default back-face culling, and the filter is a fixed PCF 3×3 with a hardcoded slope bias `max(0.0015*(1-NdotL), 0.0005)` (`PBRLightingModel::ShadowFactor`, `src/lighting/PBRLightingModel.cpp:53-71`).
  - *Why it matters.* Back-face culling in the depth pass causes peter-panning (shadows detached from contact); the fixed bias gives acne at grazing angles; a single 40-unit box can't cover both a close prop and a large ground plane at good resolution; skinned meshes aren't represented at all (see 7.4).
  - *How.*
    1. **Front-face cull (or `glPolygonOffset`) in the depth pass** to push self-shadowing behind the surface and kill peter-panning.
    2. **Expose bias + PCF radius** as `Scene3D`/serialized params instead of GLSL constants.
    3. **Fit the light frustum** to the camera frustum (or scene AABB from 9.3) instead of a hand-set center/radius.
    4. **(Later) Cascaded shadow maps** for large scenes; **(Later)** a skinned depth shader so animated meshes cast correct shadows.
  - *Check.* No visible peter-panning on the ground/crate contact; no acne on grazing surfaces; shadow resolution adapts as the camera moves.
  - Files: `src/render/ShadowMap.cpp`, `src/Scene3D.cpp` (depth pass + params), `src/lighting/PBRLightingModel.cpp` (`ShadowFactor`), `src/materials/DepthMaterial.*`.

- [ ] **9.6. No transparency pass; fixed skybox ordering.**
  - *Problem.* The pipeline is opaque-only. `Scene3D::Render` draws all meshes, then the skybox last with depth `LEQUAL` (`src/render/Skybox.cpp`). There is no alpha-blended pass, no blend-state management, and no back-to-front sorting — anything with alpha < 1 would render with wrong occlusion.
  - *Why it matters.* Blocks glass/foliage/decals/particles and any UI-in-world; also, blending added ad-hoc into the opaque walk would break depth ordering.
  - *How.* Once the renderer (8.3) exists, add an explicit pass order: **opaque (depth write on)** → **skybox** → **transparent (depth test on, depth write off, `GL_BLEND`, sorted back-to-front by camera distance)**. Tag materials/filters as opaque vs. transparent so the renderer can bucket them.
  - *Check.* A blended quad behind/in-front of opaque geometry composites correctly from any camera angle; opaque path unchanged.
  - Files: `src/render/Skybox.cpp`, `src/materials/*` (opaque/transparent tag), future `src/render/Renderer.*`.

---

## Stage 10. Resource management

- [x] **10.1. Centralize asset paths (old 4.3).** Added `AssetPaths` (`src/utils/AssetPaths.*`): `Init(argv[0])` locates the `assets/` root by walking up from the executable dir (CWD fallback); `Resolve("shaders/shader.vsh")` returns an absolute path. All in-engine `"../assets/..."` literals now go through `AssetPaths::Resolve(...)` (`main.cpp`, `Scene3D`, `ObjectSelector`, `Skybox`, `BoxModel`/`PlaneModel`/`PrimitiveModel`/`ExternalModel`, `Serialization` defaults). `Init` is called first in `main`.
  - Note: existing serialized `*.json` still embed `"../assets/..."` paths (temporary test prefabs) — re-save them through the editor to bake absolute/asset-relative paths.
  - Files: `src/utils/AssetPaths.*`, `main.cpp`, `src/Scene3D.cpp`, `src/object_selector/ObjectSelector.cpp`, `src/render/Skybox.cpp`, `src/models/*`, `src/scene/Serialization.cpp`, `CMakeLists.txt`.

- [ ] **10.2. No unified resource/asset manager.** Textures are cached (`TextureManager`), geometry is cached (`GeometryRegistry`), but models, cubemaps, HDRs and baked IBL are not — re-importing the same model rebuilds everything. Introduce an `AssetManager`/handle system so models, environments and baked IBL are shared and reference-counted like textures/geometry.
  - Files: `src/resources/`, `src/models/ExternalModel.cpp`.

- [ ] **10.3. IBL is re-baked on every scene load.** `SceneSerializer::Load` → `SetEnvironmentFromHdr` re-runs the (expensive) irradiance/prefilter/BRDF bakes from the HDR each time. Cache baked cubemaps on disk (or in the asset manager) keyed by HDR path.
  - Files: `src/Scene3D.cpp`, `src/render/IBLBaker.cpp`, `src/resources/HDRLoader.cpp`.

- [ ] **10.4. `Texture2D` is a bare struct with a raw `GLuint id`.** Unlike the other GL wrappers it has no RAII — textures created via SOIL/`IBLBaker` are leaked or freed ad-hoc. Wrap it (move-only, `glDeleteTextures` in dtor) like `GLBuffer`.
  - Files: `src/resources/Texture2D.h`, `src/resources/TextureManager.cpp`, `src/render/IBLBaker.cpp`.

---

## Stage 11. Code quality, build, tooling

- [ ] **11.1. Logging (old 6.3) is now overdue.** Raw `std::cout` is scattered across `ShaderFactory`, `TextureManager`, `IBLBaker`, `SceneSerializer`, `MaterialBase` (the latter even dumps full shader source on every build). Add a leveled logger and gate the verbose shader dump behind a debug flag.
  - Files: across the codebase; `src/materials/MaterialBase.cpp` (remove/guard the full-source dump in `Build`).

- [ ] **11.2. Naming consistency (old 5.6, still open).** Camera exposes public `Position/Front/Up/Yaw/Pitch/Zoom` (PascalCase public fields) against the project's `_camelCase` private / `camelCase` method convention. The materials filter file is `Filter3d.*` while the class is `Filter3D`. Pick one convention and apply it.
  - Files: `src/camera/*`, `src/materials/filters/Filter3d.*`.

- [ ] **11.3. Split header-only `FirstPersonCamera`.** ~163 lines of implementation live in `FirstPersonCamera.h` (and it isn't in the CMake source list, only pulled transitively). Move the impl to a `.cpp` and add it to `SOURCE_FILES`.
  - Files: `src/camera/FirstPersonCamera.h`, `CMakeLists.txt`.

- [ ] **11.4. Manual `SOURCE_FILES` list in CMake.** Every file is enumerated by hand (148 entries), which is how `FirstPersonCamera` slipped through and how new files get forgotten. Consider `target_sources` with `CONFIGURE_DEPENDS` globbing per directory, or at least group the list by module.
  - File: `CMakeLists.txt`.

- [ ] **11.5. Tests & CI (old 6.4 / 6.5, still open).** With serialization, the scene graph, id-based lookup, `VertexLayout`, and the shader-injection string logic, there are now several pure-logic units worth testing headless (no GL). Add a small test target + a Windows-MSVC + vcpkg GitHub Actions build.

- [ ] **11.6. `Engine` timing precision.** `GetCurrentTimeMillis` uses `GetTickCount()` (~10–16 ms granularity, 49-day wrap) and feeds animation time. Switch to `glfwGetTime()` / `std::chrono::steady_clock` for sub-ms precision.
  - File: `src/Engine.cpp`.

- [ ] **11.7. Mixed responsibilities in `Engine`.** The singleton owns input, texture manager, geometry registry, render-mode helper, timing *and* the object selector (set externally). It's drifting into a service-locator. Longer term, split input/resources/timing into separate services injected where needed rather than reached through a global.
  - Files: `src/Engine.*`.

---

## Suggested order of attack

1. **Stage 7** (correctness): 7.1, 7.2, 7.3 are quick, high-impact fixes.
2. **10.1** (asset root) — unblocks running outside the IDE and is a prerequisite for tests/CI.
3. **8.1** (extract `Application`) — makes everything afterwards easier to change.
4. **9.2** (UBO) — the cheapest large rendering win.
5. Then iterate on the renderer abstraction (8.3) and resource manager (10.2).
