#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"

class FloorTile : public INodeContent
{
public:
    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "#";
        CC::Unlock();
    }
};
