#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"
#include <chrono>

class Player : public INodeContent
{
private:
    Vector2 _position;
    std::chrono::steady_clock::time_point _lastActionTime;
    int _actionCooldownMs = 400; // Milisegundos entre acciones

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
};