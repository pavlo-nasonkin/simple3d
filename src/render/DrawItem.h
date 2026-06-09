#pragma once

#include <glm/glm.hpp>

class MeshRenderer;
class Pivot3D;

// Один элемент рендер-очереди: что и где рисовать. Собирается обходом графа, затем
// отсеивается фрустумом и сортируется по материалу/геометрии в Renderer.
struct DrawItem
{
    const MeshRenderer* renderer = nullptr;
    const Pivot3D* owner = nullptr;
    glm::mat4 world { 1.0f }; // world-матрица узла (без nodeMatrix — её добавляет Draw)
};
