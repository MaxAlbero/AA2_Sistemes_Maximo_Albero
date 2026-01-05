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

    void Draw(Vector2 pos) override;
    Vector2 GetPosition();
    void ReceiveDamage(int damageToReceive) override;
    bool IsBroken();

    Json::Value Code() override;
    void Decode(Json::Value json) override;
};