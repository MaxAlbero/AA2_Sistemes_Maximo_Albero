#pragma once
#include "DungeonMap.h"
#include "../InputSystem/InputSystem.h"
#include <mutex>

class Game
{
public:
    Game();
    ~Game();

    void Start();
    void Stop();

    void Lock();
    void Unlock();

private:
    DungeonMap* _dungeonMap;
    InputSystem* _inputHandler;
    int _currentRoomIndex;
    bool _running;
    std::mutex _classMutex;

    void InitializeDungeon();
    void DrawCurrentRoom();

    // Callbacks para input
    void OnMoveUp();
    void OnMoveDown();
    void OnMoveLeft();
    void OnMoveRight();
};