#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"
#include "../NodeMap/Vector2.h"

enum class PortalDir { Left, Right, Up, Down };

class Portal : public INodeContent
{
private:
    PortalDir _direction;

public:
    Portal(PortalDir dir) : _direction(dir) {}

    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetColor(CC::LIGHTGREY);
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "P";
        CC::SetColor(CC::WHITE);
        CC::Unlock();
    }

    PortalDir GetDirection() const { return _direction; }
};