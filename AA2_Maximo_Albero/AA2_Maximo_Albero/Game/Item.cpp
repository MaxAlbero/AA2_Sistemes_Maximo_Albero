#include "Item.h"

void Item::Draw(Vector2 pos)
{
    CC::Lock();
    CC::SetPosition(pos.X, pos.Y);

    // Dibujar según el tipo
    switch (_type)
    {
    case ItemType::COIN:
        std::cout << "k";  // 'c' para coin
        break;
    case ItemType::POTION:
        std::cout << "o";  // 'o' para potion
        break;
    case ItemType::WEAPON:
        std::cout << "w";  // 'w' para weapon
        break;
    }

    CC::Unlock();
}

Vector2 Item::GetPosition() {
    _itemMutex.lock();
    Vector2 pos = _position;
    _itemMutex.unlock();
    return pos;
}

ItemType Item::GetType() {
    _itemMutex.lock();
    ItemType type = _type;
    _itemMutex.unlock();
    return type;
}

Json::Value Item::Code() {
    Json::Value json;
    CodeSubClassType<Item>(json);
    json["posX"] = _position.X;
    json["posY"] = _position.Y;
    json["type"] = static_cast<int>(_type);
    return json;
}

void Item::Decode(Json::Value json) {
    _position.X = json["posX"].asInt();
    _position.Y = json["posY"].asInt();
    _type = static_cast<ItemType>(json["type"].asInt());
}