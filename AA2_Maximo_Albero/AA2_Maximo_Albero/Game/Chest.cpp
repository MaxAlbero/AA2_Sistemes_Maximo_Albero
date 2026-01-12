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
    Lock();
    Vector2 pos = _position;
    Unlock();
    return pos;
}


void Chest::ReceiveDamage(int damageToReceive) {
    Lock();
    _hp -= damageToReceive;

    if (_hp <= 0)
    {
        _hp = 0;
        _broken = true;
        //std::cout << "¡Cofre destruido!" << std::endl;
    }

    Unlock();
}

bool Chest::IsBroken() {
    Lock();
    bool isBroken = _broken;
    Unlock();
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
