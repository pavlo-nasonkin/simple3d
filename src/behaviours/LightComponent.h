#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Behaviour.h"

// Источник света — компонент-ДАННЫЕ на узле. Логики не несёт (OnUpdate не
// переопределяет): рендер собирает свет из графа и читает его параметры. Трансформ
// света (направление/позиция) берётся из мировой матрицы узла-владельца.
class LightComponent : public Behaviour
{
public:
    const glm::vec3& GetColor() const { return _color; }
    void SetColor(const glm::vec3& color) { _color = color; }
    float GetIntensity() const { return _intensity; }
    void SetIntensity(float intensity) { _intensity = intensity; }

protected:
    glm::vec3 _color { 1.0f, 1.0f, 1.0f };
    float _intensity = 1.0f;
};

// Направленный свет (солнце): направление = forward узла (−Z мировой матрицы).
class DirectionalLightComponent : public LightComponent
{
public:
    DirectionalLightComponent();

    std::string GetTypeName() const override { return "DirectionalLight"; }
    const std::vector<Property>& GetProperties() const override;

    glm::vec3 GetWorldDirection() const; // куда светит (нормализовано)
    const glm::vec3& GetAmbient() const { return _ambient; }
    void SetAmbient(const glm::vec3& ambient) { _ambient = ambient; }

private:
    glm::vec3 _ambient { 0.03f, 0.03f, 0.03f }; // плоский заполняющий свет (до IBL)
};

// Точечный свет: позиция = translation узла. Пока ДАННЫЕ (шейдер однопроходный,
// одного направленного света) — задел под мультисвет/аккумуляцию (см. план).
class PointLightComponent : public LightComponent
{
public:
    std::string GetTypeName() const override { return "PointLight"; }
    const std::vector<Property>& GetProperties() const override;

    glm::vec3 GetWorldPosition() const;
    float GetRange() const { return _range; }
    void SetRange(float range) { _range = range; }

private:
    float _range = 10.0f;
};
