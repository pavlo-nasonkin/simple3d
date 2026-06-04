#pragma once

class Scene3D;

// Панель «Hierarchy»: дерево узлов сцены, выбор синхронизирован с ObjectSelector.
class HierarchyPanel
{
public:
    void Draw(Scene3D* scene, bool* open);
};
