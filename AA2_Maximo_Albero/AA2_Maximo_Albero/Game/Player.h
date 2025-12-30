#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"

#include "../Utils/IAttacker.h"
#include "../Utils/IDamageable.h"
#include <chrono>

class Player : public INodeContent, public IAttacker, public IDamageable
{
private:
    Vector2 _position;
    std::chrono::steady_clock::time_point _lastActionTime;
    int _actionCooldownMs = 400; // Milisegundos entre acciones

    int _hp = 50;
    int _coins = 0;
    int _potionCount = 1;
    int _weapon = 0;

public:
    Player(Vector2 startPosition)
        : _position(startPosition)
    {
        _lastActionTime = std::chrono::steady_clock::now();
    }

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "P";
        CC::Unlock();
    }

    Vector2 GetPosition() const { return _position; }
    void SetPosition(Vector2 newPos) { _position = newPos; }

    bool CanPerformAction()
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - _lastActionTime
        ).count();

        return timeSinceLastAction >= _actionCooldownMs;
    }

    void UpdateActionTime()
    {
        _lastActionTime = std::chrono::steady_clock::now();
    }

    void SetActionCooldown(int milliseconds)
    {
        _actionCooldownMs = milliseconds;
    }

    void Attack(IDamageable* entity) const override {
        if(entity != nullptr)
            entity->ReceiveDamage(20);
    }

    void ReceiveDamage(int damageToAdd) override {
        std::cout << "El player pierde 10 de vida" << std::endl;
    }
};