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

    int _hp;
    int _maxHp;
    int _coins;
    int _potionCount;
    int _weapon;

    std::mutex _playerMutex;

public:
    Player(Vector2 startPosition)
        : _position(startPosition), _hp(50), _maxHp(50), _coins(0), _potionCount(1), _weapon(0)
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

        //std::cout << "¡El jugador recibe " << damageToReceive << " de daño! HP: " << _hp << std::endl;

        if (_hp <= 0)
        {
            _hp = 0;
            //std::cout << "¡GAME OVER!" << std::endl;
        }

        _playerMutex.unlock();
    }

    void AddCoin()
    {
        _playerMutex.lock();
        _coins+=10;
        _playerMutex.unlock();
    }

    void AddPotion()
    {
        _playerMutex.lock();
        _potionCount++;
        _playerMutex.unlock();
    }

    void ChangeWeapon()
    {
        _playerMutex.lock();

        switch (_weapon) {
        case 0:
            _weapon = 1;
            break;
        case 1:
            _weapon = 0;
            break;
        default:
            _weapon = 0;
        }

        _playerMutex.unlock();
    }

    void UsePotion()
    {
        _playerMutex.lock();

        if (_potionCount > 0)
        {
            _potionCount--;
            _hp += 20; // Recupera 20 HP

            if (_hp > _maxHp)
                _hp = _maxHp;

            std::cout << "¡Poción usada! HP: " << _hp << std::endl;
        }
        else
        {
            std::cout << "No tienes pociones" << std::endl;
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

    int GetCoins()
    {
        _playerMutex.lock();
        int coins = _coins;
        _playerMutex.unlock();
        return coins;
    }

    int GetPotionCount()
    {
        _playerMutex.lock();
        int potions = _potionCount;
        _playerMutex.unlock();
        return potions;
    }

    int GetWeapon()
    {
        _playerMutex.lock();
        int weapon = _weapon;
        _playerMutex.unlock();
        return weapon;
    }

    bool IsAlive()
    {
        _playerMutex.lock();
        bool alive = _hp > 0;
        _playerMutex.unlock();
        return alive;
    }


};