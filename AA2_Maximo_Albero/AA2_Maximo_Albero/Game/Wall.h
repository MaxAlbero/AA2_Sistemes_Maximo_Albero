#pragma once
#include "../NodeMap/INodeContent.h"
#include "../Utils/ConsoleControl.h"

class Wall : public INodeContent
{
public:
    void Draw(Vector2 pos) override {
        CC::Lock();
        CC::SetColor(CC::CYAN);
        CC::SetPosition(pos.X, pos.Y);
        std::cout << "#";
        CC::SetColor(CC::WHITE);
        CC::Unlock();
    }
};
