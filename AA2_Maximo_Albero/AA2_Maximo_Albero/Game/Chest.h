#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/IDamageable.h"
#include <mutex>
#include <thread>

#include "../Json/ICodable.h"

class Chest : public INodeContent, public IDamageable, public ICodable {
private:
    Vector2 _position;
    int _hp;

    std::mutex _chestMutex;
    bool _broken; //necesita empezar como "= false"

public:
    Chest() : Chest(Vector2(0, 0)) {}

    Chest(Vector2 position, int hp = 20)
        : _position(position), _hp(hp), _broken(false) {}

    ~Chest() {}

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetColor(CC::YELLOW);
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "C";
        CC::SetColor(CC::WHITE);
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


    Json::Value Code() override {
        Json::Value json;
        CodeSubClassType<Chest>(json);
        json["posX"] = _position.X;
        json["posY"] = _position.Y;
        json["hp"] = _hp;
        json["broken"] = _broken;
        return json;
    }

    void Decode(Json::Value json) override {
        _position.X = json["posX"].asInt();
        _position.Y = json["posY"].asInt();
        _hp = json["hp"].asInt();
        _broken = json["broken"].asBool();
    }
};