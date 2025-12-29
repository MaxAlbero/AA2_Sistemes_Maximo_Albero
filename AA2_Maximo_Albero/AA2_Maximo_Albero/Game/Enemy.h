#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/IAttacker.h"
#include "../Utils/IDamageable.h"
#include <chrono>
#include <mutex>

class Enemy : public INodeContent, public IAttacker, public IDamageable
{
private:
    Vector2 _position;
    int _hp;
    int _damage;
    std::chrono::steady_clock::time_point _lastActionTime;
    int _actionCooldownMs = 500; // Milisegundos entre acciones
    std::mutex _enemyMutex;

public:
    Enemy(Vector2 startPosition, int hp = 30, int damage = 10)
        : _position(startPosition), _hp(hp), _damage(damage)
    {
        _lastActionTime = std::chrono::steady_clock::now();
    }

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "E";
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

    void Attack(IDamageable* entity) const override {
        if (entity != nullptr)
        {
            entity->ReceiveDamage(_damage);
        }
    }

    void ReceiveDamage(int damageToReceive) override {
        _enemyMutex.lock();

        _hp -= damageToReceive;

        //std::cout << "Enemigo Recibe: " << damageToReceive << "puntos de daño! HP restante: " << _hp << std::endl;

        if (_hp <= 0)
        {
            _hp = 0;
            //std::cout << "Enemigo Eliminado" << std::endl;
            // Aquí puedes añadir lógica de muerte más adelante
        }

        _enemyMutex.unlock();
    }

    bool IsAlive() {
        _enemyMutex.lock();
        bool alive = _hp > 0;
        _enemyMutex.unlock();
        return alive;
    }

    int GetHP() {
        _enemyMutex.lock();
        int hp = _hp;
        _enemyMutex.unlock();
        return hp;
    }

    void Lock() { _enemyMutex.lock(); }
    void Unlock() { _enemyMutex.unlock(); }
};