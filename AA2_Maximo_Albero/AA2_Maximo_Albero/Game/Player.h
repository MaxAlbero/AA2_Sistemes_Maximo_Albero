#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"

#include "../Json/ICodable.h"

#include "../Utils/IAttacker.h"
#include "../Utils/IDamageable.h"
#include <chrono>

class Player : public INodeContent, public IAttacker, public IDamageable, public ICodable
{
private:
    Vector2 _position;
    std::chrono::steady_clock::time_point _lastActionTime;
    int _actionCooldownMs = 400; // Milisegundos entre acciones

    int _hp;
    int _maxHp;
    int _coins;
    int _potionCount;
    int _weapon;

    std::mutex _playerMutex;

public:
    Player() : Player(Vector2(0, 0)) {}

    Player(Vector2 startPosition)
        : _position(startPosition), _hp(50), _maxHp(50), _coins(0), _potionCount(1), _weapon(0)
    {
        _lastActionTime = std::chrono::steady_clock::now();
    }

    void Draw(Vector2 pos) override;

    Vector2 GetPosition();

    void SetPosition(Vector2 newPos);

    bool CanPerformAction();

    void UpdateActionTime();

    void SetActionCooldown(int milliseconds);

    void Attack(IDamageable* entity) const override;

    void ReceiveDamage(int damageToReceive) override;

    void AddCoin();

    void AddPotion();

    void ChangeWeapon();

    void UsePotion();


    int GetAttackRange();

    int GetHP();

    int GetCoins();

    int GetPotionCount();

    int GetWeapon();
    bool IsAlive();

    Json::Value Code() override;

    void Decode(Json::Value json) override;

};