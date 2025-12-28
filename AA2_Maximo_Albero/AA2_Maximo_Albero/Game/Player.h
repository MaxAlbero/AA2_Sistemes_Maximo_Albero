#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"

class Player : public INodeContent
{
private:
    Vector2 _position;

public:
    Player(Vector2 startPosition) : _position(startPosition) {}

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "P";
        CC::Unlock();
    }

    Vector2 GetPosition() const { return _position; }
    void SetPosition(Vector2 newPos) { _position = newPos; }
};