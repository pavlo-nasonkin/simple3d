#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

class Behaviour;

// Реестр сериализуемых компонентов: строковый тип → конструктор. Аналог FilterFactory.
// Программно-конфигурируемые компоненты (Camera/FlyCamera/Animator) НЕ регистрируются —
// их нельзя восстановить из одних данных, и фабрика их пропускает на загрузке.
class BehaviourFactory
{
public:
    using Creator = std::function<std::unique_ptr<Behaviour>()>;

    static void Register(const std::string& type, Creator creator);
    // Создаёт компонент по типу (nullptr, если тип не зарегистрирован).
    static std::unique_ptr<Behaviour> Create(const std::string& type);
    static bool IsRegistered(const std::string& type);

    // Список зарегистрированных типов (для меню «Add Behaviour» в инспекторе, 9.9).
    static const std::map<std::string, Creator>& Registry();

    // Регистрирует встроенные компоненты. Вызвать один раз при старте.
    static void RegisterBuiltins();

private:
    static std::map<std::string, Creator>& registry();
};
