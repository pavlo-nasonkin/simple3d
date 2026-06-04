#pragma once

class Scene3D;
class Camera;

// Верхнее меню редактора: File / Edit / View / Import / Help.
class MenuBarPanel
{
public:
    // showHierarchy/showInspector/showMaterials — флаги видимости панелей (тумблеры в View → Panels).
    void Draw(Scene3D* scene, Camera* camera, bool* showHierarchy, bool* showInspector, bool* showMaterials);

private:
    void ImportModel(Scene3D* scene);
    void ImportPrefab(Scene3D* scene);
    void OpenScene(Scene3D* scene, Camera* camera);
    void SaveSceneAs(Scene3D* scene, Camera* camera);
};
