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

    std::mutex _playerMutex;

public:
    Player(Vector2 startPosition)
        : _position(startPosition), _hp(50), _coins(0), _potionCount(1), _weapon(0)
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
        _playerMutex.lock();
        auto currentTime = std::chrono::steady_clock::now();
        auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - _lastActionTime
        ).count();
        bool canAct = timeSinceLastAction >= _actionCooldownMs;
        _playerMutex.unlock();
        return canAct;
    }

    void UpdateActionTime()
    {
        _playerMutex.lock();
        _lastActionTime = std::chrono::steady_clock::now();
        _playerMutex.unlock();
    }

    void SetActionCooldown(int milliseconds)
    {
        _playerMutex.lock();
        _actionCooldownMs = milliseconds;
        _playerMutex.unlock();
    }

    void Attack(IDamageable* entity) const override {
        if(entity != nullptr)
            entity->ReceiveDamage(20);
    }

    void ReceiveDamage(int damageToReceive) override {
        _playerMutex.lock();
        _hp -= damageToReceive;

        std::cout << "¡El jugador recibe " << damageToReceive << " de daño! HP: " << _hp << std::endl;

        if (_hp <= 0)
        {
            _hp = 0;
            std::cout << "¡GAME OVER!" << std::endl;
        }

        _playerMutex.unlock();
    }

    int GetHP()
    {
        _playerMutex.lock();
        int hp = _hp;
        _playerMutex.unlock();
        return hp;
    }

    bool IsAlive()
    {
        _playerMutex.lock();
        bool alive = _hp > 0;
        _playerMutex.unlock();
        return alive;
    }
};