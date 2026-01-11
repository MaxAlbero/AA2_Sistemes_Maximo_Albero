#pragma once
#include "../NodeMap/Vector2.h"

class Entity {
public:
    virtual ~Entity() = default;

    virtual Vector2 GetPosition() const = 0;
    virtual void SetPosition(Vector2 pos) = 0;

    virtual bool IsAlive() const { return true; }
    virtual void ReceiveDamage(int dmg) {}
};