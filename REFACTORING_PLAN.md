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

- [ ] **4.3. Use paths relative to the exe.** `"../assets/..."` breaks when launched from the IDE folder or an install.
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
