#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Behaviour.h"
#include "reflection/References.h"

// Демонстрационный компонент: показывает все поддерживаемые типы полей рефлексии
// (вариант A). Логики не несёт — нужен для проверки инспектора и drag-n-drop.
class DemoBehaviour : public Behaviour
{
public:
    std::string GetTypeName() const override { return "Demo"; }
    const std::vector<Property>& GetProperties() const override;

private:
    bool _enabledFlag = true;
    int _numBullets = 10;
    float _fireRate = 2.5f;
    std::string _label = "turret";
    glm::vec3 _tint = glm::vec3(1.0f, 0.5f, 0.2f);

    NodeRef _target;       // ссылка на узел (drag-n-drop из Hierarchy)
    BehRef _controller;    // ссылка на компонент другого узла

    std::vector<int> _ammoPerClip = { 30, 30, 15 };
    std::vector<float> _cooldowns = { 0.5f, 1.0f };
    std::vector<NodeRef> _patrolPoints; // Vector<T> со ссылочным элементом
};
