#include "Chest.h"

void Chest::Draw(Vector2 pos)
{
    CC::Lock();
    CC::SetColor(CC::YELLOW);
    CC::SetPosition(pos.X, pos.Y);
    std::cout << "C";
    CC::SetColor(CC::WHITE);
    CC::Unlock();
}


Vector2 Chest::GetPosition() {
    _chestMutex.lock();
    Vector2 pos = _position;
    _chestMutex.unlock();
    return pos;
}


void Chest::ReceiveDamage(int damageToReceive) {
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

bool Chest::IsBroken() {
    _chestMutex.lock();
    bool isBroken = _broken;
    _chestMutex.unlock();
    return isBroken;
}


Json::Value Chest::Code() {
    Json::Value json;
    CodeSubClassType<Chest>(json);
    json["posX"] = _position.X;
    json["posY"] = _position.Y;
    json["hp"] = _hp;
    json["broken"] = _broken;
    return json;
}

void Chest::Decode(Json::Value json) {
    _position.X = json["posX"].asInt();
    _position.Y = json["posY"].asInt();
    _hp = json["hp"].asInt();
    _broken = json["broken"].asBool();
}
