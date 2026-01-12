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
#include <functional>
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

    // Thread de movimiento individual
    std::thread* _movementThread;
    std::atomic<bool> _isActive;
    std::atomic<bool> _shouldStop;

    // Callbacks para consultar al mundo (EntityManager/Game)
    std::function<bool(Enemy*, Vector2)> _canMoveToCallback;
    std::function<Vector2()> _getPlayerPositionCallback;
    std::function<void(Enemy*)> _onAttackPlayerCallback;

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
    void SetPosition(Vector2 newPos);

    bool CanPerformAction();
    void UpdateActionTime();

    void Attack(IDamageable* entity) const;
    void ReceiveDamage(int damageToReceive);

    bool IsAlive();
    int GetHP();

    Vector2 GetRandomDirection();

    // Gestión del thread individual
    void StartMovement();
    void StopMovement();
    bool IsActive() const { return _isActive; }

    // Configurar callbacks para que el enemigo pueda consultar el mundo
    void SetMovementCallbacks(
        std::function<bool(Enemy*, Vector2)> canMoveCallback,
        std::function<Vector2()> getPlayerPosCallback,
        std::function<void(Enemy*)> onAttackPlayerCallback);

    void Lock() { _enemyMutex.lock(); }
    void Unlock() { _enemyMutex.unlock(); }

    Json::Value Code();
    void Decode(Json::Value json);

private:
    void MovementLoop();
};