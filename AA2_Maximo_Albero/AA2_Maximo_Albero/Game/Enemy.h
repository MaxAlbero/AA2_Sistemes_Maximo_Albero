#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/IAttacker.h"
#include "../Utils/IDamageable.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>

#include <random>

#include "../Json/ICodable.h"


class Enemy : public INodeContent, public IAttacker, public IDamageable, public ICodable
{
private:
    Vector2 _position;
    int _hp;
    int _damage;
    std::chrono::steady_clock::time_point _lastActionTime;
    int _actionCooldownMs = 1000; // Milisegundos entre acciones
    std::mutex _enemyMutex;

    // Thread de movimiento
    std::thread* _movementThread;
    std::atomic<bool> _isActive;
    std::atomic<bool> _shouldStop;

public:
    Enemy() : Enemy(Vector2(0, 0)) {}

    Enemy(Vector2 startPosition, int hp = 30, int damage = 10)
        : _position(startPosition), _hp(hp), _damage(damage),
        _movementThread(nullptr), _isActive(false), _shouldStop(false)
    {
        _lastActionTime = std::chrono::steady_clock::now();
    }

    ~Enemy()
    {
        StopMovement();
    }

    void Draw(Vector2 pos) override;

    Vector2 GetPosition();

    void SetPosition(Vector2 newPos) {
        _enemyMutex.lock();
        _position = newPos;
        _enemyMutex.unlock();
    }

    bool CanPerformAction()
    {
        _enemyMutex.lock();
        auto currentTime = std::chrono::steady_clock::now();
        auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - _lastActionTime
        ).count();
        bool canAct = timeSinceLastAction >= _actionCooldownMs;
        _enemyMutex.unlock();
        return canAct;
    }

    void UpdateActionTime()
    {
        _enemyMutex.lock();
        _lastActionTime = std::chrono::steady_clock::now();
        _enemyMutex.unlock();
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

        if (_hp <= 0)
        {
            _hp = 0;
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

    Vector2 GetRandomDirection()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 3);

        int direction = dis(gen);

        switch (direction)
        {
        case 0: return Vector2(0, -1);  // Arriba
        case 1: return Vector2(0, 1);   // Abajo
        case 2: return Vector2(-1, 0);  // Izquierda
        case 3: return Vector2(1, 0);   // Derecha
        default: return Vector2(0, 0);
        }
    }

    // Sistema de threading para movimiento
    void StartMovement()
    {
        if (_isActive)
            return;

        _shouldStop = false;
        _isActive = true;
        _movementThread = new std::thread(&Enemy::MovementLoop, this);
    }

    void StopMovement()
    {
        if (!_isActive)
            return;

        _shouldStop = true;

        if (_movementThread != nullptr)
        {
            if (_movementThread->joinable())
                _movementThread->join();

            delete _movementThread;
            _movementThread = nullptr;
        }

        _isActive = false;
    }

    bool IsActive() const { return _isActive; }

    // Esta función será llamada desde el EntityManager
    // Para solicitar un movimiento al enemigo
    void RequestMove(std::function<bool(Vector2, Vector2&)> canMoveCallback)
    {
        if (!CanPerformAction())
            return;

        Vector2 direction = GetRandomDirection();
        Vector2 currentPos = GetPosition();
        Vector2 newPos = currentPos + direction;
        Vector2 confirmedNewPos;

        // Preguntar al EntityManager si podemos movernos
        if (canMoveCallback(newPos, confirmedNewPos))
        {
            SetPosition(confirmedNewPos);
            UpdateActionTime();
        }
    }

    void Lock() { _enemyMutex.lock(); }
    void Unlock() { _enemyMutex.unlock(); }

    Json::Value Code() override {
        Json::Value json;
        CodeSubClassType<Enemy>(json);
        json["posX"] = _position.X;
        json["posY"] = _position.Y;
        json["hp"] = _hp;
        json["damage"] = _damage;
        return json;
    }

    void Decode(Json::Value json) override {
        _position.X = json["posX"].asInt();
        _position.Y = json["posY"].asInt();
        _hp = json["hp"].asInt();
        _damage = json["damage"].asInt();
        _lastActionTime = std::chrono::steady_clock::now();
    }

private:
    void MovementLoop()
    {
        while (!_shouldStop)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (_shouldStop)
                break;

            // El movimiento real se gestiona desde EntityManager
            // Este loop solo mantiene el thread activo
        }
    }


};