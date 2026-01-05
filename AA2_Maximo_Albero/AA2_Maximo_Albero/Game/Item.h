#pragma once
#include "../NodeMap/INodeContent.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/ConsoleControl.h"
#include <mutex>

#include "../Json/ICodable.h"

enum class ItemType {
    COIN,
    POTION,
    WEAPON
};

class Item : public INodeContent, public ICodable {
private:
    Vector2 _position;
    ItemType _type;     
    std::mutex _itemMutex;

public:
    Item() : _position(0, 0), _type(ItemType::COIN) {}

    Item(Vector2 position, ItemType type)
        : _position(position), _type(type) {
    }

    ~Item() {}

    void Draw(Vector2 pos) override;

    Vector2 GetPosition();

    ItemType GetType();

    Json::Value Code() override;

    void Decode(Json::Value json) override;
};