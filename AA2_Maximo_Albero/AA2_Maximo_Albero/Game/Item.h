#pragma once
#include "../NodeMap/INodeContent.h"
#include "../NodeMap/Vector2.h"
#include "../Utils/ConsoleControl.h"
#include <mutex>

enum class ItemType {
    COIN,
    POTION,
    WEAPON
};

class Item : public INodeContent {
private:
    Vector2 _position;
    ItemType _type;
    std::mutex _itemMutex;

public:
    Item(Vector2 position, ItemType type)
        : _position(position), _type(type) {
    }

    ~Item() {}

    void Draw(Vector2 pos) override {
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

    Vector2 GetPosition() {
        _itemMutex.lock();
        Vector2 pos = _position;
        _itemMutex.unlock();
        return pos;
    }

    ItemType GetType() {
        _itemMutex.lock();
        ItemType type = _type;
        _itemMutex.unlock();
        return type;
    }
};