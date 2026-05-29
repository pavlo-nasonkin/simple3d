# План рефакторинга simple3d

Документ описывает порядок устранения проблем, найденных при ревью движка.
Задачи отсортированы по приоритету: сначала рантайм-блокеры, затем архитектура,
затем стабильность, затем стиль и оптимизации.

Обозначения статуса задач: `[ ]` — не сделано, `[~]` — в работе, `[x]` — готово.

---

## Этап 0. Подготовка

- [x] **0.1. Включить C++17.** В `CMakeLists.txt` поменять `set(CMAKE_CXX_STANDARD 11)` на `17` (или `20`). Открывает доступ к `std::filesystem`, `std::string_view`, structured bindings.
- [x] **0.2. Завести ветку `refactor/engine-core`.** Все правки этапов 1–4 идти инкрементальными коммитами по одному пункту.
- [x] **0.3. Минимальная smoke-сцена.** Сделать в `main.cpp` рядом с `nanosuit` загрузку одного `BoxModel` без анимации — чтобы видеть результаты правок ещё до починки `ExternalModel`.

---

## Этап 1. Рантайм-блокеры (то, без чего сцена не отрисуется или упадёт)

Цель этапа: запустить движок до отображения куба и nanosuit без падений.

- [x] **1.1. Заменить `glm::mat4()` на `glm::mat4(1.0f)`.** В современном GLM (vcpkg ставит ≥ 0.9.9) `glm::mat4()` создаёт нулевую матрицу — объекты схлопываются в точку.
  - Файлы: `src/Pivot3D.cpp` (`render`), `src/ExternalModel.cpp` (`render`), `src/camera/FreeLookCamera.cpp` (`handleMouseMove`).
  - Проверка: после правки куб виден на экране.

- [x] **1.2. Починить порядок TRS в `Pivot3D::render`.** Сейчас получается `S * R * T` вместо `T * R * S`, объект двигается в локальных координатах.
  ```cpp
  _model = glm::mat4(1.0f);
  _model = glm::translate(_model, _position);
  _model *= rotationMatrix();      // applyTransformRotation вынести в helper
  _model = glm::scale(_model, _scale);
  ```
  - Файлы: `src/Pivot3D.cpp`, `src/ExternalModel.cpp` (тот же блок повторён), `src/Mesh.cpp` (`applyTransformRotation` со сдвигом π/2 — пересмотреть, нужен ли он).

- [x] **1.3. Вызывать `init()` после добавления в сцену.**
  - В `Pivot3D::addChild` после привязки `_parent` вызывать `child->init()` (или ввести двухфазную инициализацию: добавить → setupScene).
  - Альтернатива: в `main.cpp` явно вызвать `soldier->init()` и `scene.init()`.
  - Файлы: `src/Pivot3D.cpp`, `main.cpp`.
  - Проверка: без этого `ExternalModel::_scene == nullptr` → крэш на первом кадре.

- [x] **1.4. Установить `Device3D::scene3D`.** Без этого все материалы (`Material3D::bind`, `ColorMaterial::bind`) падают по `nullptr` на запросе `getLightAmbient()` и т.п.
  - Сейчас `Device3D::scene3D` — `std::shared_ptr<Scene3D>`, никогда не присваивается. Простейший фикс: в `main.cpp` сделать `auto scene = std::make_shared<Scene3D>(...)` и `Device3D::scene3D = scene`.
  - Это требует превращения `scene` из стек-объекта в `shared_ptr` (см. 2.1).

- [x] **1.5. Починить баг копирования в `Vector3f(const float*)`.**
  ```cpp
  Vector3f(const float* p) { x = p[0]; y = p[1]; z = p[2]; }
  ```
  - Файл: `src/utils/Math3d.h`.

- [x] **1.6. Добавить `break;` в `FreeLookCamera::handleMouseButton`.** Сейчас case middle проваливается в case right.
  - Файл: `src/camera/FreeLookCamera.cpp`.

- [x] **1.7. Починить декларацию `KeyboardInput::_keys`.**
  - В `.cpp`: `bool KeyboardInput::_keys[1024] = {};` (вместо `bool KeyboardInput::_keys[];`).
  - Файл: `src/input/KeyboardInput.cpp`.

- [x] **1.8. Защититься от моделей без анимации.** В `ExternalModel::BoneTransform` и `ReadNodeHeirarchy` проверять `_scene->mNumAnimations > 0` перед обращением к `mAnimations[0]`.
  - Файл: `src/ExternalModel.cpp`.

- [x] **1.9. Проверка результата `SOIL_load_image`.** Логировать `SOIL_last_result()`, не передавать `nullptr` в `glTexImage2D`.
  - Файл: `src/resources/TextureManager.h`.

---

## Этап 2. Архитектурные блокеры (lifetime + scene graph)

Цель этапа: иерархия трансформов работает, объекты гарантированно живут и
освобождаются корректно.

- [x] **2.1. Сцена как `shared_ptr`.** Создавать `Scene3D` через `std::make_shared`, потому что `Pivot3D` наследует `std::enable_shared_from_this`. Без этого любой `shared_from_this()` на сцене — `bad_weak_ptr`.
  - Файлы: `main.cpp`.

- [x] **2.2. Родитель = `std::weak_ptr<Pivot3D>`.** Сейчас `_parent` — `shared_ptr`, что создаёт цикл и утечку графа.
  - Заменить: `std::weak_ptr<Pivot3D> _parent;`.
  - Везде, где `_parent` используется: `auto p = _parent.lock(); if (p) ...`.
  - Файлы: `src/Pivot3D.h`, `src/Pivot3D.cpp`.

- [x] **2.3. Убрать `shared_from_this()` из `~Pivot3D`.** В деструкторе weak_ptr уже expired, будет `bad_weak_ptr` → `std::terminate`.
  - Удалить блок «отвязки от родителя» в деструкторе (она и не нужна, если parent — weak_ptr и сам не держит уничтожаемого объекта).
  - Файл: `src/Pivot3D.cpp`.

- [x] **2.4. Иерархия трансформов.** Дочерние узлы должны учитывать world-матрицу родителя.
  - Вариант A (минимальный): в `Pivot3D::render` принимать `const glm::mat4& parentWorld = glm::mat4(1.0f)`, считать `_world = parentWorld * local`, передавать детям.
  - Вариант B (правильный): пробрасывать `RenderContext& ctx` — см. 3.1.
  - Файлы: `src/Pivot3D.h/.cpp`, `src/Scene3D.cpp`, `src/Mesh.cpp`, `src/ExternalModel.cpp`, `src/Model.cpp`.
  - Проверка: вложенный куб (`scene.addChild(parent); parent->addChild(child); child.setPosition(...)`) рендерится со сложением координат.

- [x] **2.5. Удалять GL-ресурсы в деструкторах.**
  - `Mesh::~Mesh`: `glDeleteVertexArrays(1, &vertexAttributesArray); glDeleteBuffers(...)`.
  - `Shader::~Shader`: `glDeleteProgram(Program)` (с защитой `if (Program != 0)`).
  - Запретить копирование `Shader` (или сделать move-only), чтобы не было двойного `glDeleteProgram` той же программы.
  - Файлы: `src/Mesh.cpp`, `src/Shader.h/.cpp`.

- [x] **2.6. `removeListener` в инпутах.** Сейчас при уничтожении объекта‑слушателя его указатель остаётся в static-векторе → dangling pointer.
  - Добавить в `MouseInput`/`KeyboardInput`/`UpdateBroadcaster` метод `removeListener` и вызывать его из деструкторов `FreeLookCamera`, `FirstPersonCamera`, `ObjectSelector`.
  - Файлы: `src/input/MouseInput.*`, `src/input/KeyboardInput.*`, `src/UpdateBroadcaster.*`, `src/camera/*`, `src/object_selector/ObjectSelector.*`.

---

## Этап 3. Архитектура (Device3D → RenderContext, материалы)

Цель: убрать глобальное состояние и хрупкие зависимости через статику.

- [x] **3.1. Ввести `RenderContext`.**
  ```cpp
  struct RenderContext {
      glm::mat4 view;
      glm::mat4 projection;
      glm::mat4 model;            // текущая world-матрица узла
      Camera*   camera   = nullptr;
      Scene3D*  scene    = nullptr;
      const Pivot3D* currentObject = nullptr;
  };
  ```
  - Передавать `const RenderContext&` (или `&`) в `Pivot3D::render(ctx, material)` и `MaterialBase::bind(ctx, mesh)`.
  - Файл: новый `src/render/RenderContext.h`.

- [x] **3.2. Убрать `Device3D`.** После 3.1 этот класс не нужен; вместо него — `RenderContext`, который владеется сценой и расходуется фрейм.
  - Удалить `src/Device3D.h/.cpp`, поправить все `Device3D::view/projection/...` → `ctx.view/...`.

- [x] **3.3. Материал владеет собственным шейдером.**
  - `ShaderFactory` хранит исходники + GLuint скомпилированных шейдеров (vs/fs), но **не** готовые программы.
  - Каждый материал собирает собственный `Program` (линкует свою комбинацию vs + fs + filters). Это избавляет от взаимной поломки шейдеров в `Material3D::build` (см. п. 11 ревью).
  - Альтернатива: ключ кэша программ — кортеж (vs‑источник + fs‑источник **после** применения фильтров).
  - Файлы: `src/materials/ShaderFactory.*`, `src/Material3D.cpp`.

- [x] **3.4. Кэшировать `glGetUniformLocation`.** Сейчас в `bind()` локации запрашиваются на каждый draw call.
  - В `MaterialBase` завести `std::unordered_map<std::string, GLint> _uniformLocations` и lazy-кэш.
  - Файлы: `src/materials/MaterialBase.*`, `src/Material3D.cpp`, `src/materials/ColorMaterial.cpp`, `src/materials/ObjectIdMaterial.cpp`, `src/materials/SkinnedMaterial3D.cpp`.

- [x] **3.5. Базовый блок uniforms вынести в `MaterialBase::bindStandardUniforms(ctx)`.** Сейчас `Material3D::bind` и `ColorMaterial::bind` дублируют один и тот же код (view/projection/model/light).
  - Файлы: `src/materials/MaterialBase.*`, `src/Material3D.cpp`, `src/materials/ColorMaterial.cpp`.

- [x] **3.6. Симметрия `bind`/`unbind` для GL-state.** `CullFace` должен быть сброшен в `unbind` (или, лучше, выставлен заново при каждом `bind` — без зависимости от прошлого состояния).
  - Файл: `src/materials/MaterialBase.cpp`.

---

## Этап 4. Стабильность и оконные события

- [x] **4.1. Обрабатывать resize окна.** `glfwSetFramebufferSizeCallback` → обновлять `Device3D::sceenWidth/Height` (или поле `Scene::viewport`) и вызывать `glViewport`, пересобирать `projection` у камеры.
  - Файл: `main.cpp`.

- [x] **4.2. Исправить опечатку `sceenWidth` → `screenWidth`.** Если ещё не уберём `Device3D` полностью.

- [ ] **4.3. Использовать пути относительно exe.** `"../assets/..."` ломается при запуске из IDE-папки или инсталла.
  - Через `std::filesystem` определить путь рядом с `argv[0]`, относительно него грузить `assets/`.
  - Файлы: `main.cpp`, `src/materials/ShaderFactory.cpp`, `src/utils/FileUtils.cpp`.

- [x] **4.4. Бросать `std::exception`, а не `const char*`.** Заменить `throw "..."` на `throw std::invalid_argument(...)` / `std::runtime_error(...)`.
  - Файлы: `src/Pivot3D.cpp`, `src/materials/ShaderFactory.cpp`.

---

## Этап 5. Чистка стиля и API

- [x] **5.1. Убрать `using namespace std;` из `src/Mesh.h`.** Расставить `std::` явно во всех зависимых файлах.

- [x] **5.2. Убрать антипаттерн `make_shared<T>(T(...))`.** Заменить на `make_shared<T>(args...)`. Особенно важно для `Mesh` (избегаем копирования `vector<Vertex>`).
  - Файлы: повсеместно, см. `Material3D::clone`, `BoxModel::processMesh`, `ExternalModel::processMesh`, `Scene3D::Scene3D`, `ObjectSelector::ObjectSelector` и др.

- [x] **5.3. `std::string` → `std::string_view`** в сигнатурах геттеров/сеттеров и аргументах кэшей (`TextureManager`, `EventDispatcher`, `ShaderFactory`).

- [x] **5.4. `Mesh` владеет своими ресурсами, а не «голыми» `GLuint`.** Завести `class GLBuffer` / `class GLVertexArray` RAII-обёртки (или `std::unique_ptr` с кастомным deleter’ом).

- [ ] **5.5. Унифицировать математику на GLM.** Удалить `Vector3f`/`Vector4f`/`Matrix4f`/`Quaternion` из `Math3d.h`, перевести скиннинг на `glm::mat4`/`glm::quat`. Помощник для конвертации `aiMatrix4x4 → glm::mat4`.
  - Файлы: `src/utils/Math3d.*`, `src/ExternalModel.cpp`, `src/materials/SkinnedMaterial3D.cpp`.

- [ ] **5.6. Унифицировать именование.** Договориться: приватные поля — `_camelCase`, методы — `camelCase`, классы — `PascalCase`, константы — `kUpperCamel` или `UPPER_SNAKE`. Камеру (Position/Yaw/Pitch) привести к проектному стилю.

- [ ] **5.7. Перенести GLFW-адаптеры.** `GLFWKeyboardInput.*` и `GLFWMouseInput.*` из корня проекта в `src/platform/glfw/`.

- [x] **5.8. `Pivot3D::translate`/`scale` пустые.** Либо реализовать (накопительный сдвиг/масштаб), либо удалить.

- [x] **5.9. Расширяемый `Vertex`.** Описать `VertexAttribute`/`VertexLayout`, чтобы материал/меш могли работать с разными форматами вершин (для будущего PBR — добавить tangent/uv2/color).

---

## Этап 6. Опционально / задел на будущее

- [ ] **6.1. Object picking через FBO, а не пере-рендеринг сцены.** Сейчас `ObjectSelector::pickObject` рендерит сцену в дефолтный backbuffer, что приводит к мерцанию.
- [ ] **6.2. Uniform Buffer Object для view/projection/light.** Один UBO на сцену, все материалы биндят, отпадают повторные `glUniform*`.
- [ ] **6.3. Логирование (spdlog/собственное).** Вместо `std::cout` в шейдерах/SOIL — структурированные сообщения с уровнями.
- [ ] **6.4. Тесты.** Хотя бы юнит-тесты на `Pivot3D` (иерархия, поиск по id), `EventDispatcher`, `Math3d` (если оставится).
- [ ] **6.5. CI.** GitHub Actions на Windows-MSVC с vcpkg, сборка + smoke-запуск.

---

## Чек-лист «движок работает корректно»

После завершения этапа 1+2:
- [ ] Куб виден на экране и не вырожден.
- [ ] Иерархия трансформов: дочерний объект сдвигается вместе с родителем.
- [ ] nanosuit грузится и рисуется без крэша.
- [ ] Камера управляется мышью без срабатывания «правой кнопки» при клике колесом.
- [ ] Закрытие окна не оставляет повторных delete/`glDelete*`-предупреждений в выводе.
- [ ] При ресайзе окна картинка не растягивается.
