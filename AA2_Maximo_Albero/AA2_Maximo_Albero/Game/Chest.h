#pragma once
#include "../NodeMap/INodeContent.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/IDamageable.h"
#include <mutex>
#include <thread>

class Chest : public INodeContent, public IDamageable {
private:
    Vector2 _position;
    int _hp;

    std::mutex _chestMutex;
    bool _broken; //necesita empezar como "= false"

public:
    Chest(Vector2 position, int hp = 20)
        : _position(position), _hp(hp), _broken(false) {}

    ~Chest() {}

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "C";
        CC::Unlock();
    }

    Vector2 GetPosition() {
        _chestMutex.lock();
        Vector2 pos = _position;
        _chestMutex.unlock();
        return pos;
    }


    void ReceiveDamage(int damageToReceive) override {
        _chestMutex.lock();
        _hp -= damageToReceive;

        if (_hp <= 0)
        {
            _hp = 0;
            _broken = true;
            //std::cout << "¡Cofre destruido!" << std::endl;
        }  

        _chestMutex.unlock();
    }

    bool IsBroken() {
        _chestMutex.lock();
        bool isBroken = _broken;
        _chestMutex.unlock();
        return isBroken;
    }

};