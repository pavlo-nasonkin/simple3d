# План развития simple3d

Документ описывает порядок развития движка после завершения базового
рефакторинга (см. `REFACTORING_PLAN.md`).

Ключевая идея: **отделить «модель освещения» (Phong / PBR / Unlit) от**
**самого материала.** Сейчас лайтинг жёстко зашит в шейдер
`defaultColorLight.fs` и в `Material3D::Bind`; вынесем его в отдельную
абстракцию `ILightingModel`, которую материал держит ровно одним
экземпляром. Тогда переход на PBR станет заменой одной строки:

```cpp
mat->SetLightingModel(std::make_unique<PBRLightingModel>());
```

Обозначения статуса задач: `[ ]` — не сделано, `[~]` — в работе, `[x]` — готово.

---

## Этап 1. Введение `ILightingModel`

Цель: вынести структуры `Light`/`Material`, uniform-ы `viewPos`/`light.*`/
`material.shininess` и саму lighting-формулу из шейдера + `Material3D::Bind`
в подменяемую сущность.

### Чем отличается от существующих `Filter3D`

| | `Filter3D` (модулятор) | `ILightingModel` (синтезатор) |
|---|---|---|
| Производит | значение в свой slot (`BASE_COLOR` / `SPEC_STRENGTH` / `N` / `EMISSIVE`) | финальный `color` |
| Зависимости | нет — каждый фильтр независим | читает результаты **всех** фильтров |
| Сколько в материале | 0..N | строго один |
| Доступ к `RenderContext` | не нужен | нужен (light/viewPos берутся из сцены) |
| Положение в `main` | в одном из 5 маркеров слотов | строго после всех slot-фильтров |
| Сигнатура `Bind` | `Bind(program, firstTextureUnit)` | `Bind(program, firstTextureUnit, ctx)` |

Это **другая сущность по контракту**, поэтому:

1. `ILightingModel` **не наследует** `Filter3D` и не лежит в `_filters`.
2. Сигнатуры `Bind` намеренно разные — фильтру `RenderContext` не нужен,
   и расширять его API ради одного потребителя — антипаттерн «один общий
   интерфейс для всего». Фильтр должен оставаться простым.
3. `Material3D::Bind` биндит их **последовательно**, но через два разных
   вызова — см. п. 1.3.

### Подэтапы

- [x] **1.1. Объявить `ILightingModel`.** Самостоятельный интерфейс,
  **не** связанный с `Filter3D` ни наследованием, ни общим базовым типом.
  ```cpp
  // src/materials/lighting/ILightingModel.h
  class ILightingModel {
  public:
      virtual ~ILightingModel() = default;

      // GLSL: struct Light, struct Material, uniform Light light; uniform Material material;
      // плюс in-переменные, которые модель ждёт от vertex-шейдера (FragPos, Normal, TBN).
      virtual std::string GetDeclarations() const = 0;

      // GLSL: код, который консьюмит BASE_COLOR/SPEC_STRENGTH/N/EMISSIVE
      // и пишет финальный color.
      virtual std::string GetLightingCode() const = 0;

      // Привязка uniform-ов и текстур при каждом draw.
      // firstTextureUnit — стартовый юнит, выделенный материалом из общего
      // счётчика (после всех Filter3D::Bind вызовов).
      virtual void Bind(GLuint program, GLuint firstTextureUnit, const RenderContext& ctx) = 0;

      // Сколько texture units нужно (для PBR — IBL maps + BRDF LUT).
      // Симметрично Filter3D::GetUniformsCount(), но это отдельный метод
      // отдельного интерфейса — общего базового типа намеренно нет.
      virtual unsigned int GetTextureUnitCount() const { return 0; }
  };
  ```
  - Файл: `src/materials/lighting/ILightingModel.h`.

- [x] **1.2. Маркеры в `defaultColorLight.fs`.** Заменить inline-лайтинг
  на 2 маркера, которые подставляются при `Material3D::Build()`.
  ```glsl
  // Текущий блок «uniform vec3 viewPos; uniform Material material; uniform Light light; ...» удалить.
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

      // __APPLY_LIGHTING__   ← LightingModel::GetLightingCode() заканчивается присваиванием color
  }
  ```
  - Файл: `assets/shaders/defaultColorLight.fs`.

- [x] **1.3. `Material3D` владеет `unique_ptr<ILightingModel>`.**
  ```cpp
  class Material3D : public MaterialBase {
      std::unique_ptr<ILightingModel> _lighting;   // отдельно от _filters
  public:
      void SetLightingModel(std::unique_ptr<ILightingModel> model);
  };
  ```
  - В `Material3D::BuildFragmentShader()` после `InjectFilters` подставить
    `__LIGHTING_DECLS__` и `__APPLY_LIGHTING__` из `_lighting`.
  - В `Material3D::Bind` биндить фильтры и лайтинг **последовательно, но**
    **через два разных вызова с разными контрактами** — это та самая
    «семантическая чистота», ради которой `ILightingModel` не подкласс
    `Filter3D`:
    ```cpp
    GLuint nextUnit = 0;

    // 1) Фильтры — узкий контракт без ctx
    for (const auto& f : _filters) {
        f->Bind(_shader->GetProgram(), nextUnit);
        nextUnit += f->GetUniformsCount();
    }

    // 2) Лайтинг — расширенный контракт с ctx
    if (_lighting) {
        _lighting->Bind(_shader->GetProgram(), nextUnit, ctx);
        nextUnit += _lighting->GetTextureUnitCount();
    }
    ```
  - Если `_lighting == nullptr` на момент `Build()` — бросить
    `std::runtime_error("Material3D has no lighting model")`. Либо в ctor
    `Material3D` по умолчанию ставить `PhongLightingModel` (решить при
    реализации).
  - Файлы: `src/materials/Material3D.h/.cpp`.

- [x] **1.4. `Filter3D::Bind` остаётся без изменений.** Это
  явная фиксация дизайн-решения, а не задача. Соблазн «давайте сделаем
  единый `Bind(program, firstTextureUnit, ctx)` для всего» нужно сразу
  отметать:
  - фильтры (`TextureMapFilter`, `ColorFilter`, `NormalMapFilter`) `ctx`
    не используют — расширение их API ради одного потребителя нарушает
    Interface Segregation;
  - единый контракт стирает разницу между «модулятор входа» и
    «модель освещения», то есть ломает ту самую семантическую границу,
    ради которой мы вводим `ILightingModel`;
  - если завтра появится фильтр, которому нужен `ctx` (например, фильтр
    «depth fog», читающий `ctx.camera`) — это сигнал, что он на самом
    деле не filter, а ещё одна верхнеуровневая абстракция вроде
    PostProcess. Не повод расширять `Filter3D::Bind`.

---

## Этап 2. Базовые реализации `ILightingModel`

- [x] **2.1. `PhongLightingModel`** — миграция существующей логики.
  - `GetDeclarations()` возвращает текущий блок:
    ```glsl
    struct Material { float shininess; };
    struct Light { vec3 position; vec3 ambient; vec3 diffuse; vec3 specular; };
    uniform vec3 viewPos;
    uniform Material material;
    uniform Light light;
    ```
  - `GetLightingCode()` возвращает то, что сейчас в `main` после маркеров
    фильтров — расчёт ambient/diffuse/specular и финальный `color = ...`.
  - `Bind(prog, ctx)` ставит `viewPos`, `light.position`/`ambient`/`diffuse`/
    `specular`, `material.shininess`. Это код, который сейчас в
    `Material3D::Bind` (lines ~110–135) — переедет сюда дословно.
  - Файлы: `src/materials/lighting/PhongLightingModel.h/.cpp`.

- [x] **2.2. `UnlitLightingModel`** — для источников света и UI-материалов.
  - `GetDeclarations()` — пусто.
  - `GetLightingCode()` — `color = vec4(BASE_COLOR + EMISSIVE, 1.0);`
    (игнорирует свет, использует базовый цвет и эмиссию).
  - `Bind(prog, ctx)` — пусто.
  - **Бонус:** позволит удалить из `assets/shaders/` отдельный
    `light_source_shader.vs/.fs` — лампа в `Scene3D::initLightView` будет
    использовать тот же темплейт `defaultColorLight.fs`, просто с unlit-
    моделью.
  - Файлы: `src/materials/lighting/UnlitLightingModel.h/.cpp`,
    `src/Scene3D.cpp`.

- [x] **2.3. Миграция потребителей.** Везде, где сейчас создаётся
  `Material3D`, добавить `SetLightingModel(make_unique<PhongLightingModel>())`
  (или сделать дефолтным в ctor, см. 1.3).
  - Файлы: `src/models/BoxModel.cpp`, `src/models/ExternalModel.cpp`,
    `src/Scene3D.cpp`.

- [x] **2.4. Чистка `defaultColorLight.fs`.** После 1.2 + 2.1 файл должен
  содержать только vertex-input'ы (`FragPos`, `Normal`, `TexCoords`, `TBN`),
  `uBaseColor`, и каркас `main()` со слот-маркерами. Никакого лайтинга,
  никаких структур `Light`/`Material`, никакого `viewPos`.
  - Этот шейдер становится «универсальным lit-темплейтом», его можно
    переименовать, например, в `default_material.fs` (но это уже стиль).

---

## Этап 3. Подготовка инфраструктуры под PBR

Цель: расширить filter-slots под PBR-входы, **не** трогая `ILightingModel`.
Phong по-прежнему работает; новые слоты он просто игнорирует.

- [ ] **3.1. Новые `FilterSlot`.** Добавить в `Filter3D::FilterSlot`:
  - `Metallic` — `float`-канал, мап в `M`-переменную в шейдере.
  - `Roughness` — `float`-канал, мап в `R`.
  - `AO` (ambient occlusion) — `float`-канал, мап в `AO`.
  - Шейдер `defaultColorLight.fs` объявляет дефолты:
    ```glsl
    float M  = 0.0;
    float R  = 0.5;
    float AO = 1.0;
    ```
    плюс новые маркеры:
    ```glsl
    // __APPLY_METALLIC_FILTERS__
    // __APPLY_ROUGHNESS_FILTERS__
    // __APPLY_AO_FILTERS__
    ```
  - `kSlotTarget` и `kSlotMarkers` в `Material3D::InjectFilters`
    дополняются новыми записями.
  - Файлы: `src/materials/filters/Filter3d.h`,
    `src/materials/Material3D.cpp`,
    `assets/shaders/defaultColorLight.fs`.

- [ ] **3.2. Фильтры под `float`-результат.** Сейчас все фильтры возвращают
  `vec3`/`vec4`. Для `Metallic`/`Roughness`/`AO` цель — `float`. Расширить
  `Filter3D::ResultType` значениями `FLOAT`, и в `InjectFilters` подобрать
  swizzle (`.r` для `float`-цели от `vec4`-фильтра).
  - Альтернатива: завести специализированный `TextureChannelFilter`,
    который сразу возвращает `float`.
  - Файлы: `src/materials/filters/Filter3d.h`, `src/materials/Material3D.cpp`.

- [ ] **3.3. Маппинг Assimp-типов на новые слоты.** В
  `ExternalModel::ProcessMesh` добавить считывание:
  - `aiTextureType_METALNESS` → `FilterSlot::Metallic`
  - `aiTextureType_DIFFUSE_ROUGHNESS` → `FilterSlot::Roughness`
  - `aiTextureType_AMBIENT_OCCLUSION` → `FilterSlot::AO`
  - С таким же fallback'ом на устаревшие типы, как мы сейчас делаем для
    `aiTextureType_NORMALS` ↔ `aiTextureType_HEIGHT` для obj.
  - У backpack эти карты есть (`roughness.jpg`, `ao.jpg`), но мапятся
    они через нестандартные имена — нужна проверка.
  - Файлы: `src/models/ExternalModel.cpp`,
    `assets/models/backpack/backpack.mtl` (возможно).

---

## Этап 4. `PBRLightingModel` (Cook-Torrance)

Цель: реализовать физически-корректную модель освещения, читающую новые
слоты `Metallic`/`Roughness`/`AO`.

- [ ] **4.1. PBR-математика.** `GetLightingCode()` возвращает классический
  Cook-Torrance BRDF:
  - GGX/Trowbridge-Reitz normal distribution function
  - Schlick fresnel
  - Smith geometry
  - F0 = mix(vec3(0.04), BASE_COLOR, M)
  - diffuse = (1 - F) * (1 - M) * BASE_COLOR / π
  - specular = (D * F * G) / (4 * NdotV * NdotL)
  - color = (kD * diffuse + specular) * radiance * NdotL + ambient * AO
  - Файл: `src/materials/lighting/PBRLightingModel.h/.cpp`.

- [ ] **4.2. Direct lights only.** На первом этапе считаем только один
  point light из `Scene3D` (то же, что у Phong сейчас). IBL — следующая
  задача.

- [ ] **4.3. Тестовая сцена.** В `main.cpp` добавить флаг (или второй
  объект), который использует `PBRLightingModel`. Сравнить с Phong-
  версией того же backpack визуально — на пряжках должна быть видна
  заметная разница в спекулярах (Phong даёт мягкий блик, PBR даёт
  более точечный с учётом roughness map).
  - Файл: `main.cpp`.

---

## Этап 5. IBL — image-based lighting (опционально)

Это переход от «технически PBR без окружения» к «PBR в студии».

- [ ] **5.1. `SceneEnvironment` — отдельная сущность сцены.**
  ```cpp
  class SceneEnvironment {
      std::shared_ptr<TextureCube> irradiance;        // diffuse IBL
      std::shared_ptr<TextureCube> prefilteredSpec;   // specular IBL
      std::shared_ptr<Texture2D>   brdfLUT;
  };
  ```
  Хранится в `Scene3D`, отдаётся через `ctx.scene3D->getEnvironment()`.
  - Файлы: `src/render/SceneEnvironment.h`, `src/Scene3D.h/.cpp`,
    `src/render/RenderContext.h`.

- [ ] **5.2. Класс `TextureCube`.** Сейчас есть только `Texture2D`.
  - Файлы: `src/resources/TextureCube.h/.cpp`.

- [ ] **5.3. Загрузчик HDR equirectangular → cubemap.** Один HDR-файл
  `.hdr`/`.exr` → cubemap через однократный render-pass.
  - Внешний loader (например, `stb_image` в HDR-режиме).
  - Файл: `src/resources/HDRLoader.cpp`.

- [ ] **5.4. Прекомпиляция irradiance + prefiltered specular + BRDF LUT.**
  Эти три cubemap'а считаются один раз при загрузке environment'а.
  - Алгоритм: см. [LearnOpenGL/PBR/IBL](https://learnopengl.com/PBR/IBL/Diffuse-irradiance).
  - Файлы: `src/render/IBLBaker.h/.cpp`.

- [ ] **5.5. `PBRLightingModel` использует IBL.** В `Bind()` биндит
  irradiance, prefilteredSpec, brdfLUT из `ctx.scene3D->getEnvironment()`
  на texture units, выделенные через `GetTextureUnitCount() == 3`.
  В `GetLightingCode()` добавляется ambient через IBL вместо плоского
  `light.ambient`.

- [ ] **5.6. Skybox.** Отрисовка cubemap'а как фона сцены — отдельный
  vertex+fragment шейдер. Это не часть `ILightingModel`, это отдельный
  pass в `Scene3D::Render`.
  - Файлы: `src/render/Skybox.h/.cpp`, `assets/shaders/skybox.vs/.fs`.

---

## Этап 6. Разделение `Mesh` на `Geometry` + `Mesh-instance`

**Независим от Этапов 1–5.** Можно делать параллельно (после
`REFACTORING_PLAN 5.9` — он предпосылка).

### Проблема

Сейчас `Mesh` одновременно играет две несовместимых роли:

1. **Узел сцены** — `is-a Pivot3D`, имеет position/rotation/scale, parent,
   children.
2. **GPU-ресурс** — владеет VAO/VBO/EBO.

Из-за этого:

- **Геометрия дублируется в VRAM.** 100 одинаковых box-моделей →
  100 копий box-VBO. Каждый `BoxModel::processMesh` делает свой
  `glGenBuffers` и `glBufferData`.
- **`shared_ptr<Mesh>` нельзя расшарить как «одинаковую геометрию».**
  Если положить один `shared_ptr<Mesh>` в двух местах сцены, у них
  будет один transform — то есть это «один объект, отрендеренный
  дважды в одной точке», а не «два инстанса в разных позициях».

Стандартное решение (Unity: `Mesh` + `MeshRenderer`; Unreal:
`UStaticMesh` + `UStaticMeshComponent`) — разделить роли на два типа.

### Подэтапы

- [ ] **6.1. Класс `Geometry`** — immutable owner GL-ресурсов, без
  transform, без иерархии. Шарится через `std::shared_ptr<Geometry>`.
  ```cpp
  // src/render/Geometry.h
  class Geometry {
      GLVertexArray _vao;
      GLBuffer _vbo;
      GLBuffer _ebo;
      std::vector<GLBuffer> _secondaryVbos;   // для skinning и пр.
      GLsizei _indicesCount = 0;
  public:
      // Конструируется один раз с layout + сырыми данными.
      template <typename V>
      Geometry(const VertexLayout& layout,
               const std::vector<V>& vertices,
               const std::vector<GLuint>& indices);

      void AddSecondaryBuffer(const VertexLayout& layout,
                              std::span<const std::byte> data);

      // Move-only автоматически (благодаря GLBuffer/GLVertexArray).
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
  - Извлекается из текущего `Mesh` — берёт всё, что про GL.
  - Файл: `src/render/Geometry.h/.cpp`.

- [ ] **6.2. `Mesh` становится лёгким узлом сцены.**
  ```cpp
  // src/models/Mesh.h
  class Mesh : public Pivot3D {
      std::shared_ptr<Geometry>    _geometry;  // ← шарится
      std::shared_ptr<MaterialBase> _material; // ← тоже может шариться
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
  - Размер `Mesh` минус Pivot3D — три указателя. На сцене из тысяч
    объектов это копейки RAM.
  - `SetupMesh` исчезает — его работа переезжает в `Geometry::Geometry`.
  - Файлы: `src/models/Mesh.h/.cpp`.

- [ ] **6.3. `GeometryRegistry` — кеш через `weak_ptr`.**
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
          _cache[std::string(key)] = geometry;  // weak копия
          return geometry;
      }

      void Cleanup() { _cache.clear(); }
  };
  ```
  - **`weak_ptr`, а не `shared_ptr`** — важно: если все инстансы Mesh,
    использовавшие данную Geometry, разрушены, и Registry хранит только
    `weak_ptr`, реальная Geometry удаляется автоматически. VRAM
    освобождается без явного вызова. Если инстансы есть — переиспользуем.
  - Хранится в `Engine` рядом с `TextureManager` и `ShaderFactory`.
  - `Engine::Cleanup()` вызывает `_geometryRegistry.Cleanup()` до
    `glfwTerminate`.
  - Файлы: `src/render/GeometryRegistry.h/.cpp`, `src/Engine.h/.cpp`.

- [ ] **6.4. Миграция `BoxModel`.** Box-геометрия становится singleton'ом
  через Registry.
  ```cpp
  std::shared_ptr<Mesh> BoxModel::processMesh()
  {
      auto mat = MakeBoxMaterial(_color);   // как сейчас

      auto geometry = Engine::GetInstance().GetGeometryRegistry().GetOrCreate(
          "primitive:box",
          [] { return Geometry(VertexLayouts::Standard(), boxVertices, boxIndices); });

      return std::make_shared<Mesh>(geometry, mat);
  }
  ```
  - 100 box-моделей → один box-VBO в VRAM, 100 материалов (разных цветов),
    100 лёгких Mesh-инстансов с разными transform'ами.
  - Файл: `src/models/BoxModel.cpp`.

- [ ] **6.5. Миграция `ExternalModel`.** Ключ кеша — путь к файлу +
  индекс mesh'а (в одном .obj/.fbx может быть несколько меш-частей).
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
  - Загрузка одного и того же `backpack.obj` дважды разными
    `ExternalModel` → Assimp всё равно парсит файл дважды (это можно
    тоже кешировать на уровне `aiScene` отдельной задачей), но **VBO
    в VRAM ровно один**.
  - Файл: `src/models/ExternalModel.cpp`.

### Что НЕ делается в этом этапе

- **GPU instancing** (`glDrawElementsInstanced` + per-instance VBO с
  transform'ами). Это отдельная оптимизация, экономящая **draw calls**,
  а не VRAM. Геометрия после 6.x уже отдельная сущность — instancing
  ляжет сверху без переделок.
- **Кеш `aiScene`** (одинаковые `.obj`-файлы парсятся повторно). Это
  отдельная задача уровня asset loading.
- **Material-кеш.** Material уже может шариться через
  `shared_ptr<MaterialBase>` (его не дублирует никакая структура), но
  сейчас каждый `BoxModel`/`ExternalModel` создаёт свой. Сделать
  `MaterialRegistry` — отдельная история, не блокер.

---

## Связь с `REFACTORING_PLAN.md`

- **3.4 (кэш `glGetUniformLocation`)** должен быть сделан **до** этапа 1.
  Иначе `ILightingModel::Bind` будет дёргать `glGetUniformLocation` 6–10
  раз на каждом draw — а нам наоборот хочется, чтобы новая абстракция
  не делала движок медленнее.

- **3.5 (`bindStandardUniforms`)** частично перекрывается с этим планом:
  то, что было «общим uniform-ом» (`viewPos`, `light.*`), уезжает в
  `ILightingModel`, а в `MaterialBase` остаётся только `model`/`view`/
  `projection`. После этапа 1 пункт 3.5 закрывается автоматически.

- **5.9 (Расширяемый `Vertex`)** становится критичным на двух этапах:
  - **Этап 5** (IBL/Skybox) — skybox использует свой layout без UV.
  - **Этап 6** (Geometry split) — `Geometry::Geometry(layout, vertices, ...)`
    типонезависим только если layout приходит снаружи. Без 5.9 Geometry
    жёстко зависел бы от `struct Vertex` и не смог бы хранить, например,
    `PBRVertex` или skybox-вершины.
  - Без 5.9 каждый новый формат вершин ломает существующий VAO в Mesh.

- **2.5 / 5.4 (RAII GL-ресурсы)** — предпосылка Этапа 6. `Geometry` =
  пачка `GLBuffer` + `GLVertexArray`, и владение через `shared_ptr<Geometry>`
  работает корректно ровно потому, что RAII-обёртки уничтожают
  GL-ресурсы детерминированно. Без них «лишний» Geometry в кеше
  означал бы утечку VBO в VRAM до конца программы.

---

## Антипаттерны, которых сознательно избегаем

1. **`mat->SetLightingModel("PBR")`** — строковое имя вместо типа.
   Скрывает зависимость, теряется автодополнение, ловится только в
   рантайме.
2. **`Material3D<TLightingModel>`** — шаблонный параметр.
   Раскручивает Material3D в N версий, ломает динамическую смену
   модели в рантайме.
3. **`LightingFilter : Filter3D`** — лайтинг как наследник Filter.
   Filter — modulator (пишет в slot), Lighting — synthesizer (читает
   все slot'ы, пишет финальный color). Это разные контракты; смешение
   приведёт к тому, что в `Filter3D` будут спец-кейсы «если это lighting,
   то делать иначе».
4. **Единый `Bind(program, unit, ctx)` для Filter и LightingModel.**
   Соблазн: «давайте сделаем универсальный интерфейс». Минус: фильтрам
   `ctx` не нужен, расширение API ради одного потребителя — нарушение
   Interface Segregation. Намеренно держим две разные сигнатуры — это
   фиксирует разницу контрактов на уровне типов, а не комментариев.
5. **Lighting-uniform'ы в Scene3D статикой.** Соблазн: «свет один на
   сцену, пусть Scene3D сам биндит». Минус: ломает контракт «материал
   владеет тем, что биндит». В нашей схеме `ILightingModel::Bind`
   читает Scene3D через `ctx`, но запись uniform-ов делает сама.
6. **Расшарить `shared_ptr<Mesh>` ради «одной геометрии».** Соблазн:
   «у меня уже есть `shared_ptr<Mesh>`, положу его в два места сцены —
   получу одну геометрию в VRAM». Минус: `Mesh : Pivot3D` имеет
   **один** transform, parent и набор детей. Два «инстанса» будут
   стоять в одной точке. Это **архитектурная неправильность роли**, а
   не оптимизация — решается Этапом 6 (разделение Geometry и
   Mesh-instance), а не shared_ptr-трюками.
